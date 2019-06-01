#ifndef APP_LOGIN_CONTROLLER_HPP
#define APP_LOGIN_CONTROLLER_HPP

#include "app_context.hpp"
#include "auth.hpp"

class LoginController {
public:
    LoginController(PageContext &ctx):  ctx_(ctx) {}

    bool dispatch() ;
    void login() ;
    void logout() ;

private:
    PageContext &ctx_ ;
};

#endif
