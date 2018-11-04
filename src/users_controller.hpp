#ifndef APP_USERS_CONTROLLER_HPP
#define APP_USERS_CONTROLLER_HPP

#include "app_context.hpp"
#include "forms.hpp"

#include "auth.hpp"
#include "page_view.hpp"

class UsersController: public PageContext {
public:
    UsersController(PageContext &ctx): PageContext(ctx) {}

    bool dispatch() ;

    void create() ;

    void edit() ;
    void edit(const string &user_id) ;
    void remove() ;
    void fetch();
    void update();

};

#endif






