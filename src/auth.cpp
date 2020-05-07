#include "auth.hpp"
#include <ws/crypto.hpp>

#include <regex>

#include "string_utils.hpp"

using namespace std ;
using namespace xdb ;
using namespace ws ;
using namespace twig ;

void UserModel::persist(const std::string &user_name, const std::string &user_id, const std::string &user_role, bool remember_me)
{
    // fill in session cache with user information

    //        session_regenerate_id(true) ;

    ctx_.session_.add("user_name", user_name) ;
    ctx_.session_.add("user_id", user_id) ;
    ctx_.session_.add("user_role", user_role) ;

    // remove any expired tokens

    ctx_.con_.execute("DELETE FROM auth_tokens WHERE expires < ?", time(nullptr)) ;

    // If the user has clicked on remember me button we have to create the cookie

    if ( remember_me ) {
        string selector = encodeBase64(randomBytes(12)) ;
        string token = randomBytes(24) ;
        time_t expires 	= std::time(nullptr) + 3600*24*10; // Expire in 10 days

        ctx_.con_.execute("INSERT INTO auth_tokens ( user_id, selector, token, expires ) VALUES ( ?, ?, ?, ? );", user_id, selector, binToHex(hashSHA256(token)), expires) ;

        ctx_.response_.setCookie("auth_token", selector + ":" + encodeBase64(token),  expires, "/");
    }

    // update user info
    ctx_.con_.execute("UPDATE user_info SET last_sign_in = ? WHERE user_id = ?", std::time(nullptr), user_id) ;


}

void UserModel::forget()
{
    if ( check() ) {

        string user_id = ctx_.session_.get("user_id") ;

        ctx_.session_.remove("user_name") ;
        ctx_.session_.remove("user_id") ;
        ctx_.session_.remove("user_role") ;

        string cookie = ctx_.request_.getCookie("auth_token") ;

        if ( !cookie.empty() ) {
           ctx_.response_.setCookie("auth_token", string(), 0, "/") ;

           ctx_.con_.execute("DELETE FROM auth_tokens WHERE user_id = ?", user_id) ;

        }
    }
}

static bool hash_equals(const std::string &query, const std::string &stored) {

    if ( query.size() != stored.size() ) return false ;

    uint ncount = 0 ;
    for( uint i=0 ; i<stored.size() ; i++ )
        if ( query.at(i) != stored.at(i) ) ncount ++ ;

    return ncount == 0 ;
}

string UserModel::userName() const {
    return ctx_.session_.get("user_name") ;
}

string UserModel::userId() const {
    return ctx_.session_.get("user_id") ;
}

string UserModel::userRole() const {
    return ctx_.session_.get("user_role") ;
}

string UserModel::token() const
{
    string session_token = ctx_.session_.get("token") ;
    if ( session_token.empty() ) {
        string token = binToHex(randomBytes(32)) ;
        ctx_.session_.add("token", token) ;
        return token ;
    }
    else return session_token ;
}



bool UserModel::check() const
{
    if ( ctx_.session_.contains("user_name") ) return true ;

    // No session information. Check if there is user info in the cookie

    string cookie = ctx_.request_.getCookie("auth_token") ;

    if ( !cookie.empty() ) {

        vector<string> tokens ;
        split(tokens, cookie, ':');


        if ( tokens.size() == 2 && !tokens[0].empty() && !tokens[1].empty() ) {
            // if both selector and token were found check if database has the specific token

            QueryResult res = ctx_.con_.query(
                "SELECT a.user_id as user_id, a.token as token, u.name as username, r.role_id as role FROM auth_tokens AS a JOIN users AS u ON a.user_id = u.id JOIN user_roles AS r ON r.user_id = u.id WHERE a.selector = ? AND a.expires > ? LIMIT 1",
                               tokens[0], std::time(nullptr)) ;


            if ( res.next() ) {

                string cookie_token = binToHex(hashSHA256(decodeBase64(tokens[1]))) ;
                string stored_token = res.get<string>("token") ;

                if ( hash_equals(stored_token, cookie_token) ) {
                    ctx_.session_.add("user_name", res.get<string>("username")) ;
                    ctx_.session_.add("user_id", res.get<string>("user_id")) ;
                    ctx_.session_.add("user_role", res.get<string>("role")) ;

                    return true ;
                }
            }
        }

        // if we are here then probably there is a security issue
    }

    return false ;
}

bool UserModel::userNameExists(const string &username)
{
    QueryResult res = ctx_.con_.query("SELECT id FROM 'users' WHERE name = ? LIMIT 1;", username) ;
    return res.next() ;
}

bool UserModel::verifyPassword(const string &query, const string &stored) {
    return passwordVerify(query, decodeBase64(stored)) ;
}

void UserModel::load(const string &username, string &id, string &password, string &role)
{
    QueryResult res = ctx_.con_.query("SELECT u.id AS id, u.password as password, r.role_id as role FROM users AS u JOIN user_roles AS r ON r.user_id = u.id WHERE name = ? LIMIT 1;", username) ;

    if ( res.next() ) {
        id = res.get<string>("id") ;
        password = res.get<string>("password") ;
        role = res.get<string>("role") ;
    }
}

static string glob_to_regex(const string &pat)
{
    // Convert pattern
    string rx = "^", be ;

    string::const_iterator cursor = pat.begin(), end = pat.end() ;
    bool in_char_class = false ;

    while ( cursor != end )
    {
        char c = *cursor++;

        switch (c)
        {
            case '*':
                rx += ".*?" ;
                break;
            case '$':  //Regex special characters
            case '(':
            case ')':
            case '+':
            case '.':
            case '|':
                rx += '\\';
                rx += c;
                break;
            case '\\':
                if ( *cursor == '*' ) rx += "\\*" ;
                else if ( *cursor == '?' )  rx += "\\?" ;
                cursor ++ ;
            break ;
            default:
                rx += c;
        }
    }

    rx += "$" ;
    return rx ;
}

static bool match_permissions(const string &glob, const string &action) {
    string rxs = glob_to_regex(glob) ;
    regex rx(rxs) ;
    bool res = regex_match(action, rx) ;
    return res ;
}

bool UserModel::can(const string &action) const {
    string role = userRole() ;
    vector<string> permissions  = auth_.getPermissions(role) ;
    for( uint i=0 ; i<permissions.size() ; i++ ) {
        if ( match_permissions(permissions[i], action) ) return true ;
    }
    return false ;
}



static string strip_all_tags(const string &str, bool remove_breaks = false) {
    static regex rx_stags(R"(<(script|style)[^>]*?>.*?<\/\1>)", regex::icase) ;
    static regex rx_tags(R"(<[^>]*>)") ;
    static regex rx_lb(R"([\r\n\t ]+)") ;

    // remove spacial tags and their contents
    string res = regex_replace(str, rx_stags, "") ;
    // remove all other tags
    res = regex_replace(res, rx_tags, "") ;

    if ( remove_breaks )
        res = regex_replace(res, rx_lb, " ") ;

    return trim_copy(res) ;
}

string UserModel::sanitizeUserName(const string &username)
{
    return strip_all_tags(username) ;
}

string UserModel::sanitizePassword(const string &password)
{
    return trim_copy(password) ;
}

void UserModel::create(const string &username, const string &password, const string &role)
{
    string secure_pass = encodeBase64(passwordHash(password)) ;
    ctx_.con_.execute("INSERT INTO users ( name, password ) VALUES ( ?, ? )", username, secure_pass);
    ctx_.con_.execute("INSERT INTO user_roles ( user_id, role_id ) VALUES ( (SELECT last_insert_rowid()), ? )", role) ;
}

void UserModel::update(const string &id, const string &password, const string &role)
{
    string secure_pass = encodeBase64(passwordHash(password)) ;
    ctx_.con_.execute("UPDATE users SET password=? WHERE id=?", secure_pass, id) ;
    ctx_.con_.execute("UPDATE user_roles SET role_id=? WHERE user_id=?", role, id) ;
}

//////////////////////////////////////////////////////////////////////////////

DefaultAuthorizationModel::DefaultAuthorizationModel(Variant role_map)
{
    for( const string &key: role_map.keys() ) {
        Variant pv = role_map.at(key) ;
        Variant name = pv.at("name") ;

        string rname ;
        if ( name.isNull() ) rname = key ;
        else rname = name.toString() ;

        Variant permv = pv.at("permissions") ;
        vector<string> permissions ;
        if ( permv.isArray() ) {
            for( uint i=0 ; i<permv.length() ; i++ ) {
                Variant val = permv.at(i) ;
                if ( val.isPrimitive() ) permissions.emplace_back(val.toString()) ;
            }
        }
        role_map_.insert({key, Role(rname, permissions)}) ;
    }
}

std::vector<string> DefaultAuthorizationModel::getPermissions(const std::string &role) const
{
    auto it = role_map_.find(role) ;
    if ( it != role_map_.end() ) return it->second.permissions_ ;
    else return {} ;
}

Dictionary DefaultAuthorizationModel::getRoles() const {
    Dictionary roles ;
    for( const auto &pr: role_map_) {
        roles.emplace(pr.first, pr.second.name_) ;
    }
    return roles ;
}

////////////////////////////////////////////////////////////////////////////////////////////

void UserRepository::createUser(const string &email, const string &username, const string &password, const std::vector<string> &roles)
{

}

User UserRepository::fetchUser(int64_t id)
{
    QueryResult res = con_.query("SELECT id, name, email, password, role FROM users WHERE id = ?;", id) ;

    User u ;
    if ( res.next() ) {
        u.id_ = res.get<int64_t>("id") ;
        u.name_ = res.get<string>("name") ;
        u.password_ = res.get<string>("password") ;
        u.email_ = res.get<string>("email") ;
        split(u.roles_, res.get<string>("roles"), '|');
    }

    return u ;
}

void UserRepository::create() {
    con_.execute("CREATE TABLE IF NOT EXISTS " + prefix_ + "users ( id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, email TEXT UNIQUE NOT NULL, password TEXT NOT NULL, enabled INTEGER DEFAULT 0, created INTEGER NOT NULL, roles TEXT NOT NULL );") ;
    con_.execute("CREATE TABLE IF NOT EXISTS " + prefix_ + "auth_tokens ( id INTEGER PRIMARY KEY AUTOINCREMENT, selector TEXT, token TEXT, user_id INTEGER NOT NULL, expires INTEGER, FOREIGN KEY(user_id) REFERENCES users(id) );") ;
}
