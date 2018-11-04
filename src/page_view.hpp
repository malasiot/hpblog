#ifndef APP_PAGE_VIEW_HPP
#define APP_PAGE_VIEW_HPP

#include <twig/variant.hpp>

#include "auth.hpp"

// Helper for global page layout

class PageView {
 public:

    PageView(const User &user, twig::Variant menu);

    twig::Variant data(const std::string &page_id, const std::string &title) const;

    twig::Variant menu_ ;
    const User &auth_ ;
} ;






#endif
