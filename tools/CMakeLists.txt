add_executable(mailer
    mailer.cpp
    mailer.hpp
    base64.cpp
    base64.hpp

)

target_link_libraries(mailer pthread crypto ssl)
