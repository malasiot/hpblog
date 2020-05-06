#ifndef APP_MODELS_USER_HPP
#define APP_MODELS_USER_HPP

#include "app_context.hpp"

// a user model that handles authentication via username and password stored in database and authroization via role/permission models

/*
 * CREATE TABLE users ( id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE NOT NULL, password TEXT NOT NULL );
 * CREATE TABLE auth_tokens ( id INTEGER PRIMARY KEY AUTOINCREMENT, selector TEXT, token TEXT, user_id INTEGER NOT NULL, expires INTEGER );
 * CREATE TABLE user_roles ( id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER NOT NULL, role_id TEXT );
 */

class AuthorizationModel ;

struct User {
    std::string name_, password_ ;
    std::string role_ ;
};

class UserRepository {
public:
    UserRepository(xdb::Connection &con, const std::string &table_prefix = std::string()): con_(con), prefix_(table_prefix) {
        create() ;
    } ;

    void createUser(const std::string &username, const std::string &password, const std::string &role_str) ;

    // check database for username
    bool userNameExists(const std::string &username) ;

    User fetch(const std::string &name) ;

    void updatePassword(const std::string &username, const std::string &password) ;
    void updateRole(const std::string &username, const std::string &role) ;

private:

    void create() ;

private:

    xdb::Connection &con_ ;
    std::string prefix_ ;
};

class UserModel {
public:
    UserModel(AppContext &ctx, AuthorizationModel &auth):
        ctx_(ctx), auth_(auth) {
    }

    void persist(const std::string &username, const std::string &id, const std::string &role, bool remember_me = false) ;
    void forget() ;

    bool check() const ;

    std::string userName() const ;
    std::string userId() const ;
    std::string userRole() const ;
    std::string token() const ;

    // check database for username
    bool userNameExists(const std::string &username) ;

    // verify query password against the one stored in database
    bool verifyPassword(const std::string &query, const std::string &stored) ;

    // fetch user from database
    void load(const std::string &username, std::string &id, std::string &password, std::string &role) ;

    // test if the user has permission to perform the action
    bool can(const std::string &action) const ;

    AuthorizationModel &auth() const { return auth_ ; }

    static std::string sanitizeUserName(const std::string &username);
    static std::string sanitizePassword(const std::string &password);

    void create(const std::string &username, const std::string &password, const std::string &role) ;
    void update(const std::string &id, const std::string &password, const std::string &role) ;

protected:

    AppContext &ctx_ ;
    AuthorizationModel &auth_ ;
} ;

class AuthorizationModel {
public:
    AuthorizationModel() {}

    virtual Dictionary getRoles() const = 0 ;
    virtual std::vector<std::string> getPermissions(const std::string &role) const = 0 ;
};

class DefaultAuthorizationModel: public AuthorizationModel {
public:
    // In memory access control model
    // roles is an array of roles of the form { "role.1": { "name": "Administrator", "permissions": [ "permision.1", "permision.2", permision.3" ]}, ...}

    DefaultAuthorizationModel(twig::Variant role_map) ;

    virtual Dictionary getRoles() const ;
    std::vector<std::string> getPermissions(const std::string &role) const override ;

private:
    struct Role {
        Role(const std::string name, const std::vector<std::string> &permissions):
            name_(name), permissions_(permissions) {}

        std::string name_ ;
        std::vector<std::string> permissions_ ;
    };

    std::map<std::string, Role> role_map_ ;
};
#endif
