#ifndef BLOG_PAGE_CONTROLLER_HPP
#define BLOG_PAGE_CONTROLLER_HPP

#include "app_context.hpp"

#include "forms.hpp"

#include "login.hpp"
#include "page_view.hpp"

class PageCreateForm: public FormHandler {
public:
    PageCreateForm(xdb::Connection &con) ;

    void onSuccess(const ws::Request &request) override ;

private:
    xdb::Connection &con_ ;
};

class PageUpdateForm: public FormHandler {
public:
    PageUpdateForm(xdb::Connection &con, const std::string &id) ;

    void onSuccess(const ws::Request &request) override ;
    void onGet(const ws::Request &request, ws::Response &) override ;

private:
    xdb::Connection &con_ ;
    std::string id_ ;
};

class PageController: public PageContext {
public:
    PageController(PageContext &ctx): PageContext(ctx) {}

    bool dispatch() ;

    void show(const string &page_id) ;

    void create() ;
    void publish() ;
    void edit() ;
    void edit(const string &page_id) ;
    void remove() ;
    void fetch();
    void update();
};

#endif
