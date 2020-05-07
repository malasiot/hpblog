#include "page_view.hpp"

using namespace std ;
using namespace twig ;

PageView::PageView(const Authenticator &user, Variant menu): auth_(user), menu_(menu) {
}

Variant PageView::data(const std::string &page_id, const std::string &title) const {

    Variant::Object nav{{"menu", menu_}} ;
    if ( auth_.check() ) nav.insert({"username", auth_.userName()}) ;

    Variant page(
                Variant::Object{
                    { "nav", nav },
                    { "title", title }
                }
            ) ;

    return page ;
}
