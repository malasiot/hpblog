#include "login.hpp"

#include <ws/crypto.hpp>

#include "forms.hpp"

using namespace std ;
using namespace ws ;

class LoginForm: public FormHandler {
public:
    LoginForm(UserModel &auth) ;

    bool validate(const ws::Request &vals) override ;

    void onSuccess(const ws::Request &request) override;

private:
    UserModel &auth_ ;

};

LoginForm::LoginForm(UserModel &auth): auth_(auth) {

    field("username").alias("Username")
        .setNormalizer([&] (const string &val) {
            return UserModel::sanitizeUserName(val) ;
        })
        .addValidator<NonEmptyValidator>();

    field("password").alias("Password")
        .setNormalizer([&] (const string &val) {
            return UserModel::sanitizePassword(val) ;
        })
        .addValidator<NonEmptyValidator>() ;

    field("csrf_token").initial(auth_.token()) ;

    field("remember-me").alias("Remember Me:") ;
}

bool LoginForm::validate(const ws::Request &vals) {
    if ( !FormHandler::validate(vals) ) return false ;

    if ( !hashCompare(getValue("csrf_token"), auth_.token()) )
        throw std::runtime_error("Security exception" ) ;

    string username = getValue("username") ;
    string password = getValue("password") ;

    if ( !auth_.userNameExists(username) ) {
        errors_.push_back("Username does not exist") ;
        return false ;
    }

    string stored_password, user_id, role ;
    auth_.load(username, user_id, stored_password, role) ;

    if ( !auth_.verifyPassword(password, stored_password) ) {
        errors_.push_back("Password mismatch") ;
        return false ;
    }

    return true ;
}

void LoginForm::onSuccess(const ws::Request &request) {
    string username = getValue("username") ;
    bool remember_me = getValue("remember-me") == "on" ;

    string stored_password, user_id, role ;
    auth_.load(username, user_id, stored_password, role) ;
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
