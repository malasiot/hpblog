add_executable(app
    app.cpp
    auth.cpp
    auth.hpp
    page_view.hpp
    page_view.cpp
    page_controller.cpp
    page_controller.hpp
    users_controller.cpp
    users_controller.hpp
    login.cpp
    login.hpp
    forms.hpp
    forms.cpp
    table.cpp
    table.hpp
    validators.hpp
    validators.cpp
)
target_link_libraries(app ws_http_server twig xdb)




