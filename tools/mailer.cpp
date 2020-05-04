#include "mailer.hpp"
#include "base64.hpp"

#include <iostream>

using namespace std ;
using namespace asio ;

// https://github.com/karastojko/mailio/blob/master/src/dialog.cpp

SMTPS::SMTPS(const std::string &hostname, unsigned port): socket_(new Socket(hostname, port))

{

}

Socket::Socket(const string &hostname, unsigned port):
    hostname_(hostname), port_(port), ios_(new io_context()), socket_(*ios_), strmbuf_(new asio::streambuf()), istrm_(new istream(strmbuf_.get())) {

    try {
        ip::tcp::resolver res(*ios_);
        asio::connect(socket_, res.resolve(hostname_, to_string(port_)));
    } catch ( system_error &e ) {

    }
}

Socket::Socket(Socket&& other) :
    hostname_(move(other.hostname_)), port_(other.port_), socket_(move(other.socket_))
{
    ios_.reset(other.ios_.release());
    strmbuf_.reset(other.strmbuf_.release());
    istrm_.reset(other.istrm_.release());
}


Socket::~Socket()
{
    try
    {
        socket_.close();
    }
    catch (...)
    {
    }
}

void SMTPS::connect()
{
    string line = socket_->receive();
    tuple<int, bool, string> tokens = parse_line(line);
    if (std::get<0>(tokens) != 220)
        throw SMTPError("Connection rejection.");
}


inline bool SMTPS::positive_intermediate(int status)
{
    return status / 100 == smtp_status_t::POSITIVE_INTERMEDIATE;
}


inline bool SMTPS::transient_negative(int status)
{
    return status / 100 == smtp_status_t::TRANSIENT_NEGATIVE;
}


inline bool SMTPS::permanent_negative(int status)
{
    return status / 100 == smtp_status_t::PERMANENT_NEGATIVE;
}

inline bool SMTPS::positive_completion(int status)
{
    return status / 100 == smtp_status_t::POSITIVE_COMPLETION;
}

void SMTPS::auth_login(const string& username, const string& password)
{
    socket_->send("AUTH LOGIN");
    string line = socket_->receive();
    tuple<int, bool, string> tokens = parse_line(line);
    if (std::get<1>(tokens) && !positive_intermediate(std::get<0>(tokens)))
        throw SMTPError("Authentication rejection.");

    // TODO: use static encode from base64

    socket_->send(base64_encode(username));
    line = socket_->receive();
    tokens = parse_line(line);
    if (std::get<1>(tokens) && !positive_intermediate(std::get<0>(tokens)))
        throw SMTPError("Username rejection.");

    socket_->send(base64_encode(password));
    line = socket_->receive();
    tokens = parse_line(line);
    if (std::get<1>(tokens) && !positive_completion(std::get<0>(tokens)))
        throw SMTPError("Password rejection.");
}

tuple<int, bool, string> SMTPS::parse_line(const string& line)
{
    try
    {
        return make_tuple(stoi(line.substr(0, 3)), (line.at(3) == '-' ? false : true), line.substr(4));
    }
    catch (out_of_range&)
    {
        throw SMTPError("Parsing server failure.");
    }
    catch (invalid_argument&)
    {
        throw SMTPError("Parsing server failure.");
    }
}



void SMTPS::ehlo()
{
    socket_->send("EHLO hostname_");
    string line = socket_->receive();
    tuple<int, bool, string> tokens = parse_line(line);
    while (!std::get<1>(tokens))
    {
        line = socket_->receive();
        tokens = parse_line(line);
    }

    if (!positive_completion(std::get<0>(tokens)))
    {
        socket_->send("HELO hostname_");

        line = socket_->receive();
        tokens = parse_line(line);
        while (!std::get<1>(tokens))
        {
            line = socket_->receive();
            tokens = parse_line(line);
        }
        if (!positive_completion(std::get<0>(tokens)))
            throw SMTPError("Initial message rejection.");
    }
}


void SMTPS::authenticate(const string &username, const string &password, SMTPS::auth_method_t method)
{
    if (method == auth_method_t::NONE)
       {
           switch_to_ssl();
           connect();
           ehlo();
       }
       else if (method == auth_method_t::LOGIN)
       {
           switch_to_ssl();
           connect();
           ehlo();
           auth_login(username, password);
       }
       else if (method == auth_method_t::START_TLS)
       {
           connect();
           ehlo();
           start_tls();
           auth_login(username, password);
       }

}

void SMTPS::start_tls()
{
    socket_->send("STARTTLS");
    string line = socket_->receive();
    tuple<int, bool, string> tokens = parse_line(line);
    if (std::get<1>(tokens) && std::get<0>(tokens) != 220)
        throw SMTPError("Start tls refused by server.");

    switch_to_ssl();
    ehlo();
}

void SMTPS::switch_to_ssl()
{
    socket_.reset(new SSLSocket(std::move(*socket_.get()))) ;


}


SSLSocket::SSLSocket(Socket &&s): Socket(move(s)), context_(asio::ssl::context::sslv23), ssl_socket_(Socket::socket_, context_) {
    try
       {
           ssl_socket_.set_verify_mode(asio::ssl::verify_none);
           ssl_socket_.handshake(asio::ssl::stream_base::client);

       }
       catch (system_error&e)
       {
         std::cout << e.what() << endl ;
           // TODO: perhaps the message is confusing
           throw SMTPError("Switching to SSL failed.");
       }
}

int main(int argc, char *argv[]) {

    SMTPS c("smtp.gmail.com", 587) ;
    c.authenticate("malasiot@gmail.com", "partita96", SMTPS::auth_method_t::START_TLS) ;
}

