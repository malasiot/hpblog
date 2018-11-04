#ifndef APP_CONTEXT_HPP
#define APP_CONTEXT_HPP

#include <xdb/connection.hpp>
#include <ws/session.hpp>
#include <twig/renderer.hpp>

class User ;
class PageView ;

using Dictionary = std::map<std::string, std::string>;

struct AppContext {
    AppContext(xdb::Connection &con, ws::Session &session,
               const ws::Request &request, ws::Response &response,
               twig::TemplateRenderer &engine):
        con_(con), session_(session), request_(request), response_(response), engine_(engine) {}

    xdb::Connection &con_ ;
    ws::Session &session_ ;
    const ws::Request &request_ ;
    ws::Response &response_ ;
    twig::TemplateRenderer &engine_ ;
};

struct PageContext: public AppContext {
    PageContext(AppContext &actx, User &user, PageView &page): AppContext(actx),
        user_(user), page_(page) {}

    User &user_ ;
    PageView &page_ ;
};







#endif
