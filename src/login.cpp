#include "login.hpp"

#include <ws/crypto.hpp>
#include <ws/mailer.hpp>
#include <ws/client.hpp>

#include "forms.hpp"

using namespace std ;
using namespace ws ;
using namespace twig ;

class LoginForm: public FormHandler {
public:
    LoginForm(Authenticator &auth, TemplateRenderer &rdr) ;

    bool validate(const ws::Request &vals) override ;

    void onSuccess(const ws::Request &request) override;

      void onGet(const ws::Request &request, ws::Response &response) override;

private:
    Authenticator &auth_ ;
    TemplateRenderer &rdr_ ;
};

LoginForm::LoginForm(Authenticator &auth, TemplateRenderer &rdr): auth_(auth), rdr_(rdr) {

    field("username").alias("Username")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizeUserName(val) ;
        })
        .addValidator<NonEmptyValidator>();

    field("password").alias("Password")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizePassword(val) ;
        })
        .addValidator<NonEmptyValidator>() ;

    field("csrf_token").initial(auth_.token()) ;

    field("remember-me").alias("Remember Me:") ;
}

bool LoginForm::validate(const ws::Request &vals) {
    if ( !FormHandler::validate(vals) ) return false ;

 //   string pass = encodeBase64(passwordHash("xx")) ;

    if ( !hashCompare(getValue("csrf_token"), auth_.token()) )
        throw std::runtime_error("Security exception" ) ;

    string name = getValue("username") ;
    string password = getValue("password") ;

    auto res = auth_.login(name, password) ;

    switch ( res ) {
    case Authenticator::OK:
        return true ;
    case Authenticator::USER_NAME_NOT_FOUND:
        errors_.push_back("Username does not exist") ;
        return false ;
    case Authenticator::PASSWORD_MISMATCH:
        errors_.push_back("Password mismatch") ;
        return false ;
    }
}

void LoginForm::onSuccess(const ws::Request &request) {
    string username = getValue("username") ;
    bool remember_me = getValue("remember-me") == "on" ;

    auth_.persist(username, remember_me) ;

}

void LoginForm::onGet(const Request &request, Response &response) {
    response.write(rdr_.render("login", {})) ;
}


class RegisterForm: public FormHandler {
public:
    RegisterForm(Authenticator &auth) ;

    bool validate(const ws::Request &vals) override ;

    void onSuccess(const ws::Request &request) override;

private:
    Authenticator &auth_ ;
};

RegisterForm::RegisterForm(Authenticator &auth): auth_(auth) {

    field("username").alias("Username")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizeUserName(val) ;
        })
        .addValidator<NonEmptyValidator>();

    field("email").alias("E-mail")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizeUserName(val) ;
        })
        .addValidator<NonEmptyValidator>();

    field("password").alias("Password")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizePassword(val) ;
        })
        .addValidator<NonEmptyValidator>() ;

    field("password_ver").alias("Verify Password")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizePassword(val) ;
        })
        .addValidator<NonEmptyValidator>() ;

    field("csrf_token").initial(auth_.token()) ;
}

bool RegisterForm::validate(const ws::Request &vals) {
    if ( !FormHandler::validate(vals) ) return false ;

    if ( !hashCompare(getValue("csrf_token"), auth_.token()) )
        throw std::runtime_error("Security exception" ) ;

    string name = getValue("username") ;
    string email = getValue("email") ;
    string password = getValue("password") ;
    string password_ver = getValue("password_ver") ;

    if ( password != password_ver ) {
        errors_.push_back("Passwords don't match") ;
        return false ;
    }

    if ( auth_.userNameExists(name) ) {
        errors_.push_back("User Name exists") ;
        return false ;
    }

    return true ;
}

void RegisterForm::onSuccess(const ws::Request &request) {
    string username = getValue("username") ;
    string password = getValue("password") ;
    string email = getValue("email") ;
    string url ;

    auth_.create(username, email, password, "user", url) ;
}

class PasswordForm: public FormHandler {
public:
    PasswordForm(Authenticator &auth, TemplateRenderer &rdr, SMTPMailer &mailer) ;

    bool validate(const ws::Request &vals) override ;

    void onSuccess(const ws::Request &request) override;

      void onGet(const ws::Request &request, ws::Response &response) override;

private:
    Authenticator &auth_ ;
    TemplateRenderer &rdr_ ;
    SMTPMailer &mailer_ ;
};

PasswordForm::PasswordForm(Authenticator &auth, TemplateRenderer &rdr, SMTPMailer &mailer): auth_(auth), rdr_(rdr), mailer_(mailer) {

    field("email").alias("email")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizeUserName(val) ;
        })
        .addValidator<NonEmptyValidator>();

    field("csrf_token").initial(auth_.token()) ;
}

static bool verify_captcha(const string &response) {
    HttpClient c ;

    auto r = c.post("https://www.google.com/recaptcha/api/siteverify",
    {{"response", response}, {"secret", "6LeIxAcTAAAAAGG-vFI1TnRWxMZNFuojJ4WifJWe"}}) ;

    if ( r.getStatus() == Response::ok ) {
        string content = r.content() ;
        twig::Variant v = twig::Variant::fromJSONString(content) ;
        return  ( v["success"].toBoolean() ) ;
    }

    return false ;
}

bool PasswordForm::validate(const ws::Request &vals) {
    if ( !FormHandler::validate(vals) ) return false ;

    if ( !hashCompare(getValue("csrf_token"), auth_.token()) )
        throw std::runtime_error("Security exception" ) ;

    string name = getValue("email") ;

    string captcha = vals.getPostAttribute("g-recaptcha-response") ;

    if ( !auth_.emailExists(name) ) {
        errors_.push_back("There is no account with this email address") ;
        return false ;
    }

    if ( !verify_captcha(captcha) ) {
        errors_.push_back("reCAPTCHA validation failed") ;
        return false ;
    }


    return true ;
}

void PasswordForm::onSuccess(const ws::Request &request) {
   string email = getValue("email") ;
   string url = auth_.makePasswordResetUrl(email) ;

   SMTPMessage msg ;
   msg.setFrom(MailAddress("malasiot@iti.gr")) ;
   msg.addRecipient(MailAddress(email)) ;
   msg.setSubject("Change password request") ;
   msg.setBody("To change your password please click on the following link:\r\n: http://127.0.0.1:5000/user/reset?" + url) ;

   mailer_.submit(msg) ;
}

void PasswordForm::onGet(const Request &request, Response &response) {
    response.write(rdr_.render("password", {})) ;
}


class ResetPasswordForm: public FormHandler {
public:
    ResetPasswordForm(Authenticator &auth, TemplateRenderer &rdr) ;

    bool validate(const ws::Request &vals) override ;

    void onSuccess(const ws::Request &request) override;

      void onGet(const ws::Request &request, ws::Response &response) override;

private:
    Authenticator &auth_ ;
    TemplateRenderer &rdr_ ;

};

ResetPasswordForm::ResetPasswordForm(Authenticator &auth, TemplateRenderer &rdr): auth_(auth), rdr_(rdr) {
    field("password").alias("password")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizePassword(val) ;
        })
        .addValidator<NonEmptyValidator>();

    field("password_ver").alias("password_ver")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizePassword(val) ;
        })
        .addValidator<NonEmptyValidator>();

    field("csrf_token").initial(auth_.token()) ;
}

bool ResetPasswordForm::validate(const ws::Request &vals) {
    if ( !FormHandler::validate(vals) ) return false ;

    if ( !hashCompare(getValue("csrf_token"), auth_.token()) )
        throw std::runtime_error("Security exception" ) ;

    string pass = getValue("password") ;
    string pass_ver = getValue("password_ver") ;

    if ( pass != pass_ver ) {
        errors_.push_back("Passwords do not match") ;
        return false ;
    }

    return true ;
}

void ResetPasswordForm::onSuccess(const ws::Request &request) {
   string pass = getValue("password") ;

   string id = request.getQueryAttribute("id") ;
   string selector = request.getQueryAttribute("s") ;
   string token = request.getQueryAttribute("t") ;

   auth_.resetPassword(id, selector, token, pass) ;

}

void ResetPasswordForm::onGet(const Request &request, Response &response) {
    response.write(rdr_.render("reset", {})) ;
}

bool LoginController::dispatch()
{
    if ( ctx_.request_.matches("GET|POST", "/user/login/") ) login() ;
    else if ( ctx_.request_.matches("GET|POST", "/user/password/") ) password() ;
    else if ( ctx_.request_.matches("GET|POST", "/user/reset/") ) reset() ;
    else if ( ctx_.request_.matches("GET", "/user/activate/")) activate() ;
    else if ( ctx_.request_.matches("GET|POST", "/user/register/") ) reg() ;
    else if ( ctx_.request_.matches("POST", "/user/logout/") ) logout() ;
    else return false ;
    return true ;
}



void LoginController::login()
{
    LoginForm form(ctx_.user_, ctx_.engine_) ;

    form.handle(ctx_.request_, ctx_.response_) ;
}

void LoginController::reg()
{
    RegisterForm form(ctx_.user_) ;

    form.handle(ctx_.request_, ctx_.response_) ;
}

void LoginController::activate()
{
    ctx_.user_.activate(ctx_.request_.getQueryAttribute("id"), ctx_.request_.getQueryAttribute("s"), ctx_.request_.getQueryAttribute("t")) ;
}

void LoginController::password()
{
    PasswordForm form(ctx_.user_, ctx_.engine_, mailer_) ;

    form.handle(ctx_.request_, ctx_.response_) ;

}

void LoginController::reset()
{
    ResetPasswordForm form(ctx_.user_, ctx_.engine_) ;

    form.handle(ctx_.request_, ctx_.response_) ;

}


void LoginController::logout()
{
    ctx_.user_.forget() ;
    ctx_.response_.writeJSON("{}");
}
