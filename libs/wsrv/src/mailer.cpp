#include <ws/mailer.hpp>

#include "smtp_socket.hpp"
#include "base64.hpp"

#include <iostream>

using namespace std ;
using namespace asio ;

// https://github.com/karastojko/mailio/blob/master/src/dialog.cpp

namespace ws {

SMTPMailer::SMTPMailer(const std::string &hostname, unsigned port): socket_(new detail::Socket(hostname, port)) {
}

SMTPMailer::~SMTPMailer() {
    try
    {
        socket_->send("QUIT");
    }
    catch (...)
    {
    }
}

void SMTPMailer::connect() {
    auto r = waitForResponse() ;

    if ( r.status() != 220 )
        throw SMTPError("Connection rejection.");
}


inline bool SMTPMailer::positive_intermediate(int status) {
    return status / 100 == smtp_status_t::POSITIVE_INTERMEDIATE;
}


inline bool SMTPMailer::transient_negative(int status) {
    return status / 100 == smtp_status_t::TRANSIENT_NEGATIVE;
}


inline bool SMTPMailer::permanent_negative(int status) {
    return status / 100 == smtp_status_t::PERMANENT_NEGATIVE;
}

inline bool SMTPMailer::positive_completion(int status) {
    return status / 100 == smtp_status_t::POSITIVE_COMPLETION;
}

SMTPMailer::ResponseLine SMTPMailer::waitForResponse()
{
    string line = socket_->receive();

    try {
        if ( line.size() < 4 ) throw SMTPError("Response message too short") ;

        return { stoi(line.substr(0, 3)), line.at(3) != '-',  line.substr(4) } ;
    }
    catch ( out_of_range & ) {
        throw SMTPError("Error parsing response message") ;
    }
    catch (invalid_argument&) {
        throw SMTPError("Error parsing response message") ;
    }
}

void SMTPMailer::auth_login(const string& username, const string& password) {
    socket_->send("AUTH LOGIN");
    auto r = waitForResponse() ;
    if ( r.isLast() && !positive_intermediate(r.status()) )
        throw SMTPError("Authentication rejection.");

    socket_->send(base64_encode(username));
    r = waitForResponse() ;
    if ( r.isLast() && !positive_intermediate(r.status()) )
        throw SMTPError("Username rejection.");

    socket_->send(base64_encode(password));
    r = waitForResponse() ;
    if ( r.isLast() && !positive_completion(r.status()) )
        throw SMTPError("Password rejection.");
}

void SMTPMailer::ehlo()
{
    socket_->send("EHLO " + src_hostname_);

    ResponseLine r ;

    do  {
        r = waitForResponse() ;
    } while ( !r.isLast() ) ;

    if ( !positive_completion(r.status()) ) {
        socket_->send("HELO " + src_hostname_);

        do  {
            r = waitForResponse() ;
        } while ( !r.isLast() ) ;

        if ( !positive_completion( r.status() ) )
            throw SMTPError("Initial message rejection.");
    }
}


void SMTPMailer::authenticate(const string &username, const string &password, SMTPMailer::auth_method_t method)
{
    if (method == auth_method_t::NONE) {
        switch_to_ssl();
        connect();
        ehlo();
    }
    else if (method == auth_method_t::LOGIN) {
        switch_to_ssl();
        connect();
        ehlo();
        auth_login(username, password);
    }
    else if (method == auth_method_t::START_TLS) {
        connect();
        ehlo();
        start_tls();
        auth_login(username, password);
    }

}

void SMTPMailer::start_tls()
{
    socket_->send("STARTTLS");

    auto r = waitForResponse() ;

    if ( r.isLast() && r.status() != 220)
        throw SMTPError("Start tls refused by server.");

    switch_to_ssl();
    ehlo();
}

void SMTPMailer::switch_to_ssl() {
    socket_.reset(new detail::SSLSocket(std::move(*socket_.get()))) ;
}

void SMTPMailer::submit(const SMTPMessage& msg)
{
    socket_->send("MAIL FROM: <" + msg.sender().address() + ">");
    auto r = waitForResponse() ;

    if ( r.isLast() && !positive_completion(r.status()))
        throw SMTPError("Mail sender rejection.");

    for (const auto& rcpt : msg.recipients() ) {
        socket_->send("RCPT TO: <" + rcpt.address() + ">");
        r = waitForResponse() ;

        if ( !positive_completion(r.status()) )
            throw SMTPError("Mail recipient rejection.");
    }

    for (const auto& rcpt : msg.cc_recipients()) {
        socket_->send("RCPT TO: <" + rcpt.address() + ">");
        r = waitForResponse() ;

        if ( !positive_completion(r.status()) )
            throw SMTPError("Mail cc recipient rejection.");
    }

    for (const auto& rcpt : msg.bcc_recipients()) {
        socket_->send("RCPT TO: <" + rcpt.address() + ">");
        r = waitForResponse() ;

        if ( !positive_completion(r.status()) )
            throw SMTPError("Mail bcc recipient rejection.");
    }

    socket_->send("DATA");
    r = waitForResponse() ;

    if (!positive_intermediate(r.status()))
        throw SMTPError("Mail message rejection.");

    string msg_str;
    msg.format(msg_str, true);
    socket_->send(msg_str + "\r\n.");
    r = waitForResponse() ;

    if ( !positive_completion(r.status()) )
        throw SMTPError("Mail message rejection.");
}

void SMTPMailer::setSourceHostname(const string hostname) {
    src_hostname_ = hostname ;
}

// assume properly formatted addresses

string MailAddress::format() const
{
    if ( name_.empty() && address_.empty()) return string() ;

    string name_formatted, addr ;

    if ( !name_.empty() )
        name_formatted += "\"" + name_ + "\"" ;

    if ( !address_.empty() )
        addr = "<" + address_ + ">" ;

    string addr_name = ( name_formatted.empty() ? addr : name_formatted + (addr.empty() ? "" : " " + addr));

    return addr_name;
}

void SMTPMessage::format(string &msg, bool dot_escape) const
{
    msg += format_header();
    msg += format_content() ;
}

string SMTPMessage::format_header() const
{
    string header;

    header += "From:" + from_.format() + "\r\n";
    header += sender_.address().empty() ? "" : "Sender:" + sender_.format() + "\r\n";
    header += reply_to_.address().empty() ? "" : "Reply-To:" + reply_to_.format() + "\r\n" ;
    header += "To:" + format_address_list(recipients_) + "\r\n";
    header += cc_recipients_.empty() ? "" : "Cc:" + format_address_list(cc_recipients_) + "\r\n";
    header += bcc_recipients_.empty() ? "" : "Bcc:" + format_address_list(recipients_) + "\r\n";
#if 0
    // TODO: move formatting datetime to a separate method
    if (!_date_time->is_not_a_date_time())
    {
        stringstream ss;
        ss.exceptions(std::ios_base::failbit);
        local_time_facet* facet = new local_time_facet("%a, %d %b %Y %H:%M:%S %q");
        ss.imbue(locale(ss.getloc(), facet));
        ss << *_date_time;
        header += DATE_HEADER + COLON + ss.str() + codec::CRLF;
    }

    if (!_parts.empty())
        header += MIME_VERSION_HEADER + COLON + _version + codec::CRLF;
    header += mime::format_header();
#endif
    header += "Subject:" + format_subject() + "\r\n";

    return header;
}

string SMTPMessage::format_address_list(const std::vector<MailAddress> &a) const
{
    string res ;

    for( const auto &address: a ) {
        if ( !res.empty() )
            res += ' ' ;
        res += address.format() ;
    }

    return res ;
}

string SMTPMessage::format_subject() const
{
    return subject_ ;
}

string SMTPMessage::format_content() const
{
    return content_->format() ;
}


} //namespace ws

