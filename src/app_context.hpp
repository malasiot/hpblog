#ifndef APP_CONTEXT_HPP
#define APP_CONTEXT_HPP

#include <xdb/connection.hpp>
#include <ws/session.hpp>
#include <twig/renderer.hpp>

class Authenticator ;
class AuthorizationModel ;
class PageView ;

using Dictionary = std::map<std::string, std::string>;

struct AppContext {
    AppContext(xdb::Connection &con, ws::Session &session,
               const ws::Request &request, ws::Response &response,
               twig::TemplateRenderer &engine, const twig::Variant &config):
        con_(con), session_(session), request_(request), response_(response), engine_(engine), config_(config) {}

    xdb::Connection &con_ ;
    ws::Session &session_ ;
    const ws::Request &request_ ;
    ws::Response &response_ ;
    twig::TemplateRenderer &engine_ ;
    const twig::Variant &config_ ;

};

struct PageContext: public AppContext {
    PageContext(AppContext &actx, Authenticator &user, AuthorizationModel *auth, PageView &page): AppContext(actx),
        user_(user), auth_(auth), page_(page) {}

    Authenticator &user_ ;
    AuthorizationModel *auth_ ;
    PageView &page_ ;
};







#endif
