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
    int64_t id_ ;
    std::string email_, name_, password_ ;
    std::vector<std::string> roles_ ;
};

class UserProvider {
public:
    UserProvider() = default ;

} ;

class UserRepository: public UserProvider {
public:
    UserRepository(xdb::Connection &con, const std::string &table_prefix = std::string()): con_(con), prefix_(table_prefix) {
        create() ;
    }

    void createUser(const std::string &email, const std::string &username, const std::string &password, const std::string &role) ;

    // check database for username
    bool userNameExists(const std::string &username) ;

    bool userEmailExists(const std::string &email) ;

    void updatePassword(const std::string &email, const std::string &password) ;
    void updateRole(const std::string &email, const std::string &role) ;

    void removeExpiredTokens() ;
    void addAuthToken(const std::string &id, const std::string &selector, const std::string &token, time_t expires) ;
    void updateLastSignIn(const std::string &id, time_t t) ;
    void removeAuthToken(const std::string &user_id) ;
    bool rememberUser(const std::string &selector, const std::string &token,
                      std::string &user_id, std::string &user_name, std::string &user_role) ;

    void fetchUserByEmail(const std::string &email,
                          std::string &id, std::string &name,
                          std::string &password, std::string &role);
private:

    void create() ;

private:

    xdb::Connection &con_ ;
    std::string prefix_ ;
};

class Authenticator {
public:
    Authenticator(UserRepository *repo, ws::Session &session, const ws::Request &req, ws::Response &res):
        repo_(repo), session_(session), request_(req), response_(res) {
    }

    void persist(const std::string &username, const std::string &id, const std::string &role, bool remember_me = false) ;
    void forget() ;

    bool check() const ;

    std::string userName() const ;
    std::string userId() const ;
    std::string userRole() const ;
    std::string token() const ;

    // check database for username
    bool userEmailExists(const std::string &email) ;

    // verify query password against the one stored in database
    bool verifyPassword(const std::string &query, const std::string &stored) ;

    // fetch user from database
    void load(const std::string &email, std::string &id, std::string &name, std::string &password, std::string &role) ;

    // test if the user has permission to perform the action
    bool can(const std::string &action) const ;

    static std::string sanitizeUserName(const std::string &username);
    static std::string sanitizePassword(const std::string &password);

    void create(const std::string &email, const std::string &username, const std::string &password, const std::string &role) ;
    void updatePassword(const std::string &id, const std::string &password) ;

protected:

    UserRepository *repo_ ;
    ws::Session &session_ ;
    ws::Response &response_ ;
    const ws::Request &request_ ;
} ;

class AuthorizationModel {
public:
    AuthorizationModel() {}

    virtual Dictionary getRoles() const = 0 ;
    virtual std::vector<std::string> getPermissions(const std::string &role) const = 0 ;
    virtual bool can(const std::string &role, const std::string &action) const = 0;
};

class DefaultAuthorizationModel: public AuthorizationModel {
public:
    // In memory access control model
    // roles is an array of roles of the form { "role.1": { "name": "Administrator", "permissions": [ "permision.1", "permision.2", permision.3" ]}, ...}

    DefaultAuthorizationModel(twig::Variant role_map) ;

    virtual Dictionary getRoles() const override;
    std::vector<std::string> getPermissions(const std::string &role) const override ;

    bool can(const std::string &role, const std::string &action) const override;
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
