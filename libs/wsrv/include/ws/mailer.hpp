#ifndef MAILER_HPP
#define MAILER_HPP

#include <string>
#include <memory>
#include <vector>

#include <ws/mime.hpp>

namespace ws {

namespace detail {
class Socket ;
}

class MailAddress {
public:

    MailAddress() = default ;
    MailAddress(const std::string &address, const std::string name = std::string()): address_(address), name_(name) {}

    std::string format() const ;
    const std::string &address() const { return address_ ; }
    const std::string &name() const { return name_ ; }

private:
    std::string name_, address_ ;
};

class SMTPMessage {
  public:

    SMTPMessage() = default ;

    SMTPMessage &setSender(const MailAddress &sender) {
        sender_ = sender ;
        return *this ;
    }

    SMTPMessage &setFrom(const MailAddress &from) {
        from_ = from ;
        return *this ;
    }


    SMTPMessage &setReplyTo(const MailAddress &reply_to) {
        reply_to_ = reply_to ;
        return *this ;
    }

    SMTPMessage &addRecipient(const MailAddress &re) {
        recipients_.push_back(re) ;
        return *this ;
    }

    SMTPMessage &setRecipients(const std::vector<MailAddress> &rel) {
        recipients_ = rel ;
        return *this ;
    }

    SMTPMessage &addCCRecipient(const MailAddress &re) {
        cc_recipients_.push_back(re) ;
        return *this ;
    }

    SMTPMessage &setCCRecipients(const std::vector<MailAddress> &rel) {
        cc_recipients_ = rel ;
        return *this ;
    }

    SMTPMessage &addBCCRecipient(const MailAddress &re) {
        bcc_recipients_.push_back(re) ;
        return *this ;
    }

    SMTPMessage &setBCCRecipients(const std::vector<MailAddress> &rel) {
        bcc_recipients_ = rel ;
        return *this ;
    }

    SMTPMessage &setSubject(const std::string &sub) {
        subject_ = sub ;
        return *this ;
    }

    SMTPMessage &setContent(const MimePart *content) {
        content_ = content ;
        return *this ;
    }


    const MailAddress &sender() const { return sender_ ; }
    const MailAddress &from() const { return from_ ; }
    const MailAddress &reply_to() const { return reply_to_ ; }
    const std::vector<MailAddress> &recipients() const { return recipients_ ; }
    const std::vector<MailAddress> &cc_recipients() const { return cc_recipients_ ; }
    const std::vector<MailAddress> &bcc_recipients() const { return bcc_recipients_ ; }

    void format(std::string &msg, bool) const ;


private:
    std::string format_header() const;
    std::string format_address_list(const std::vector<MailAddress> &a) const ;
    std::string format_subject() const;
    std::string format_content() const;

    std::string subject_ ;
    MailAddress sender_, from_, reply_to_ ;
    std::vector<MailAddress> recipients_, cc_recipients_, bcc_recipients_ ;

    const MimePart *content_ ;

};

class SMTPMailer
{
public:

    enum class auth_method_t {NONE, LOGIN, START_TLS};

    SMTPMailer(const std::string& hostname, unsigned port);
    ~SMTPMailer() ;

    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

    void submit(const SMTPMessage& msg) ;

    void setSourceHostname(const std::string str_hostname) ;

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

    struct ResponseLine {
        int status() const { return status_ ; }
        bool isLast() const { return is_last_ ; }

        int status_ ;
        bool is_last_ ;
        std::string msg_ ;
    };

    ResponseLine waitForResponse() ;

private:


    std::unique_ptr<detail::Socket> socket_ ;
    std::string src_hostname_ = "mail.client.org" ;
};

class SMTPError: public std::runtime_error {
public:

    SMTPError(const std::string &msg): std::runtime_error(msg) {}
};

}

#endif
