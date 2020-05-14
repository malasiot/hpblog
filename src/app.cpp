#include <ws/server.hpp>
#include <ws/request_handler.hpp>
#include <ws/session.hpp>
#include <ws/exceptions.hpp>
#include <ws/sqlite3_session_manager.hpp>

#include <twig/renderer.hpp>
#include <twig/functions.hpp>

#include <xdb/connection.hpp>
#include <mutex>
#include <iostream>

#include "app_context.hpp"
#include "auth.hpp"
#include "page_view.hpp"
#include "page_controller.hpp"
#include "users_controller.hpp"
#include "login.hpp"

using namespace ws ;
using namespace twig ;
using namespace xdb ;
using namespace std ;

// You may implement a filter like handler like this. Here we put the task in the destructor so that it is performed after the app
// has the chance to fill the response

class RequestLogger {
public:
    RequestLogger(const Request &req, const Response &res): req_(req), res_(res) {}
    ~RequestLogger() {
        unique_lock<mutex> lock(lock_) ;

        if ( res_.getStatus() == Response::ok ) {
            cout << req_.toString() << " " << res_.toString() << endl ;
        } else {
            cerr << req_.toString() << " " << res_.toString() << endl ;
        }
    }

    const Request &req_ ;
    const Response &res_ ;
    mutex lock_ ;
};

class GZipFilter {
public:
    GZipFilter(const Request &req, Response &res): req_(req), res_(res) {}
    ~GZipFilter() {
        if ( req_.supportsGzip() && res_.contentBenefitsFromCompression() )
            res_.compress();
    }

    const Request &req_ ;
    Response &res_ ;

};

class App: public RequestHandler {
    using Dictionary = map<string, string>;
public:

    App(const Variant &config): config_(config) {
        root_ = config["server"]["root"].toString() ;

        engine_.reset(
                    new TemplateRenderer(std::shared_ptr<TemplateLoader>(new FileSystemTemplateLoader({{root_ + "/templates/"}, {root_ + "/templates/bootstrap-partials/"}}))));
        engine_->setCaching(false) ;
    }

    void handle(const Request &req, Response &resp) override {

        RequestLogger logger(req, resp) ;
        GZipFilter gzip(req, resp) ;

        try {
            Session session(*session_manager_, req, resp) ;

            Connection con("sqlite:db=" + root_ + "/blog.sqlite") ; // establish connection with database

            AppContext ctx(con, session, req, resp, *engine_, config_) ;

            DefaultAuthorizationModel auth(Variant::fromJSONFile(root_ + "templates/acm.json")) ;


            Authenticator user(con, session, req, resp) ; // setup authentication

            PageView page(user, Variant::fromJSONFile(root_ + "templates/menu.json")) ; // global page data

            PageContext page_ctx(ctx, user, &auth, page) ;

            Dictionary attrs ;

            if ( PageController(page_ctx).dispatch() ) return ;
            if ( UsersController(page_ctx).dispatch() ) return ;
            if ( LoginController(page_ctx).dispatch() ) return ;

            if ( resp.serveStaticFile(root_, req.getPath()) ) {
                return ;
            } else {
                resp.stockReply(Response::not_found) ;
            }
        }
        catch ( HttpResponseException &e ) {

            resp.stockReply(e.code_) ;
        }
        catch ( twig::TemplateCompileException &e ) {
            cout << e.what() << endl ;
            resp.stockReply(Response::internal_server_error) ;
        }
        catch ( std::runtime_error &e ) {
            cout << e.what() << endl ;
            resp.stockReply(Response::internal_server_error) ;
        }
    }

    void setSessionManager(SessionManager *sm) {
        session_manager_.reset(sm) ;
    }

private:

    const Variant &config_ ;
    string root_ ;
    std::unique_ptr<TemplateRenderer> engine_ ;
    std::unique_ptr<SessionManager> session_manager_ ;
};


int main(int argc, char *argv[]) {

    if ( argc < 2 ) {
        cout << "Usage: app <config_file>" << endl ;
        exit(1) ;
    }

    Variant config = Variant::fromJSONFile(argv[1]) ;

    if ( config.isUndefined() ) {
        cerr << "Cannot read configuration file" << endl ;
        exit(1) ;
    }

    HttpServer server(config["server"]["listen"].toString()) ;

    const string root = config["server"]["root"].toString() ;

    App *app = new App(config) ;

    app->setSessionManager(new SQLite3SessionManager("/tmp/session.sqlite")) ;

    server.setHandler(app) ;
    server.run() ;
}
