#include "login.hpp"

#include <ws/crypto.hpp>

#include "forms.hpp"

using namespace std ;
using namespace ws ;

class LoginForm: public FormHandler {
public:
    LoginForm(Authenticator &auth) ;

    bool validate(const ws::Request &vals) override ;

    void onSuccess(const ws::Request &request) override;

private:
    Authenticator &auth_ ;

};

LoginForm::LoginForm(Authenticator &auth): auth_(auth) {

    field("email").alias("E-mail")
        .setNormalizer([&] (const string &val) {
            return Authenticator::sanitizeUserName(val) ;
        })
        .addValidator<NonEmptyValidator>();

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

  //  string pass = encodeBase64(passwordHash("xx") ;

    if ( !hashCompare(getValue("csrf_token"), auth_.token()) )
        throw std::runtime_error("Security exception" ) ;

    string email = getValue("email") ;
    string password = getValue("password") ;

    if ( !auth_.userEmailExists(email) ) {
        errors_.push_back("Username does not exist") ;
        return false ;
    }

    string stored_password, user_id, role, user_name ;
    auth_.load(email, user_id, user_name, stored_password, role) ;

    if ( !auth_.verifyPassword(password, stored_password) ) {
        errors_.push_back("Password mismatch") ;
        return false ;
    }

    return true ;
}

void LoginForm::onSuccess(const ws::Request &request) {
    string username = getValue("username") ;
    bool remember_me = getValue("remember-me") == "on" ;

    string stored_password, user_id, role, user_name ;
    auth_.load(username, user_id, user_name, stored_password, role) ;
    auth_.persist(username, user_id, role, remember_me) ;
}

bool LoginController::dispatch()
{
    if ( ctx_.request_.matches("GET|POST", "/user/login/") ) login() ;
    else if ( ctx_.request_.matches("POST", "/user/logout/") ) logout() ;
    else return false ;
    return true ;
}

void LoginController::login()
{
    LoginForm form(ctx_.user_) ;

    form.handle(ctx_.request_, ctx_.response_, ctx_.engine_) ;
}

void LoginController::logout()
{
    ctx_.user_.forget() ;
    ctx_.response_.writeJSON("{}");
}
