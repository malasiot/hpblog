#ifndef APP_LOGIN_CONTROLLER_HPP
#define APP_LOGIN_CONTROLLER_HPP

#include "app_context.hpp"
#include "auth.hpp"

#include <ws/mailer.hpp>

class LoginController {
public:
    LoginController(PageContext &ctx):  ctx_(ctx) {}

    bool dispatch() ;
    void login() ;
    void logout() ;
    void reg();
    void activate();
    void password() ;
    void reset() ;
private:
    PageContext &ctx_ ;
};

#endif
