#ifndef MAILER_HPP
#define MAILER_HPP

#include <string>
#include <chrono>

#include <asio.hpp>
#include <asio/connect.hpp>
#include <asio/ssl.hpp>
#include <asio/streambuf.hpp>
#include <asio/deadline_timer.hpp>


class Socket {
public:
    Socket(const std::string& hostname, unsigned port) ;



    template<typename Socket>
    void send(Socket& socket, const std::string& line)
    {
        try
        {
            std::string l = line + "\r\n";
            write(socket, asio::buffer(l, l.size()));
        }
        catch (asio::system_error&)
        {

        }
    }


    template<typename Socket>
    std::string receive(Socket& socket, bool raw)
    {
        try
        {
            asio::read_until(socket, *strmbuf_, "\n");
            std::string line;
            getline(*istrm_, line, '\n');
      //     if (!raw)
      //          trim_if(line, is_any_of("\r\n"));
            return line;
        }
        catch (asio::system_error&)
        {
 //           throw dialog_error("Network receiving error.");
        }
    }

    virtual void send(const std::string &line) {
        send(socket_, line) ;
    }

    virtual std::string receive(bool raw = false) {
        return receive(socket_, raw) ;
    }


    Socket(Socket &&other);
    ~Socket() ;

protected:
    /**
        Server hostname.
        **/
        const std::string hostname_;

        /**
        Server port.
        **/
        const unsigned int port_ ;

        /**
        Asio input/output service.
        **/
        std::unique_ptr<asio::io_context> ios_;

        /**
        Socket connection.
        **/

        asio::ip::tcp::socket socket_;

        /**
        Stream buffer associated to the socket.
        **/
        std::unique_ptr<asio::streambuf> strmbuf_;

        /**
        Input stream associated to the buffer.
        **/
        std::unique_ptr<std::istream> istrm_;

};


class SSLSocket: public Socket {
public:
    SSLSocket(Socket &&s) ;

    void send(const std::string &line) {
        Socket::send(ssl_socket_, line) ;
    }

    std::string receive(bool raw = false) {
        return Socket::receive(ssl_socket_, raw) ;
    }
private:

    asio::ssl::context context_;

    asio::ssl::stream<asio::ip::tcp::socket&> ssl_socket_;

};

/**
Secure version of SMTP client.
**/
class SMTPS
{
public:

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {NONE, LOGIN, START_TLS};

    /**
    Making a connection to the server.

    Parent constructor is called to do all the work.
    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `smtp::smtp(const string&, unsigned)`.
    **/
    SMTPS(const std::string& hostname, unsigned port);
    ~SMTPS() = default;

    /**
    Authenticating with the given credentials.
    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `start_tls()`, `switch_to_ssl()`, `ehlo()`, `auth_login(const string&, const string&)`, `connect()`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);



protected:
  void connect();
    /**
    Switching to TLS layer.

    @throw smtp_error Start TLS refused by server.
    @throw *          `parse_line(const string&)`, `ehlo()`, `dialog::send(const string&)`, `dialog::receive()`, `switch_to_ssl()`.
    **/
    void start_tls();

    /**
    Replaces TCP socket with SSL socket.
    @throw * `dialog_ssl::dialog_ssl(dialog_ssl&&)`.
    **/
    void switch_to_ssl();



    void ehlo();
    void auth_login(const std::string &username, const std::string &password);
    std::tuple<int, bool, std::string> parse_line(const std::string &line);

    enum smtp_status_t {POSITIVE_COMPLETION = 2, POSITIVE_INTERMEDIATE = 3, TRANSIENT_NEGATIVE = 4, PERMANENT_NEGATIVE = 5};

    bool positive_intermediate(int status);
    bool transient_negative(int status);
    bool permanent_negative(int status);
    bool positive_completion(int status);


private:
    std::unique_ptr<Socket> socket_ ;
};

class SMTPError: public std::runtime_error {
public:

    SMTPError(const std::string &msg): std::runtime_error(msg) {}
};

#endif
