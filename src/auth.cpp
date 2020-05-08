#include "auth.hpp"
#include <ws/crypto.hpp>

#include <regex>

#include "string_utils.hpp"

using namespace std ;
using namespace xdb ;
using namespace ws ;
using namespace twig ;

Authenticator::AuthResult Authenticator::login(const string &username, const string &password) {
    if ( !userNameExists(username) )
        return USER_NAME_NOT_FOUND ;

    string stored_password, user_id, role, user_name ;
    load(username, user_id, stored_password, role) ;

    if ( !verifyPassword(password, stored_password) )
        return PASSWORD_MISMATCH ;

    return OK ;
}

void Authenticator::persist(const string &username, bool remember_me) {
    string stored_password, user_id, role, user_name ;
    load(username, user_id, stored_password, role) ;
    persistUser(username, user_id, role, remember_me) ;
}

void Authenticator::persistUser(const std::string &user_name, const std::string &user_id, const std::string &user_role, bool remember_me)
{
    // fill in session cache with user information

    //        session_regenerate_id(true) ;

    session_.add("user_name", user_name) ;
    session_.add("user_id", user_id) ;
    session_.add("user_role", user_role) ;

    // remove any expired tokens

    repo_->removeExpiredTokens() ;

    // If the user has clicked on remember me button we have to create the cookie

    if ( remember_me ) {

        string selector = encodeBase64(randomBytes(12)) ;
        string token = randomBytes(24) ;
        time_t expires 	= std::time(nullptr) + 3600*24*10; // Expire in 10 days

        repo_->addAuthToken(user_id, selector, binToHex(hashSHA256(token)), expires) ;

        response_.setCookie("auth_token", selector + ":" + encodeBase64(token),  expires, "/");
    }

    repo_->updateLastSignIn(user_id, std::time(nullptr)) ;
}

void Authenticator::forget()
{
    if ( check() ) {

        string user_id = session_.get("user_id") ;

        session_.remove("user_name") ;
        session_.remove("user_id") ;
        session_.remove("user_role") ;

        string cookie = request_.getCookie("auth_token") ;

        if ( !cookie.empty() ) {
            response_.setCookie("auth_token", string(), 0, "/") ;

            repo_->removeAuthToken(user_id) ;
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

string Authenticator::userName() const {
    return session_.get("user_name") ;
}

string Authenticator::userId() const {
    return session_.get("user_id") ;
}

string Authenticator::userRole() const {
    return session_.get("user_role") ;
}

string Authenticator::token() const
{
    string session_token = session_.get("token") ;
    if ( session_token.empty() ) {
        string token = binToHex(randomBytes(32)) ;
        session_.add("token", token) ;
        return token ;
    }
    else return session_token ;
}

bool Authenticator::userNameExists(const string &name) {
    return repo_->userNameExists(name) ;
}

bool Authenticator::activate(const string &id, const string &selector, const string &token)
{
    return repo_->activate(id, selector, token) ;



}


bool Authenticator::check() const
{
    if ( session_.contains("user_id") ) return true ;

    // No session information. Check if there is user info in the cookie

    string cookie = request_.getCookie("auth_token") ;

    if ( !cookie.empty() ) {

        vector<string> tokens ;
        split(tokens, cookie, ':');

        if ( tokens.size() == 2 && !tokens[0].empty() && !tokens[1].empty() ) {
            // if both selector and token were found check if database has the specific token

            string user_id, user_name, user_role ;
            if ( repo_->rememberUser(tokens[0], tokens[1], user_id, user_name, user_role)) {
                session_.add("user_name", user_name) ;
                session_.add("user_id", user_id) ;
                session_.add("user_role", user_role) ;

                return true ;
            }
        }

        // if we are here then probably there is a security issue
    }

    return false ;
}


bool Authenticator::verifyPassword(const string &query, const string &stored) {
    return passwordVerify(query, decodeBase64(stored)) ;
}

void Authenticator::load(const string &name, string &id, string &password, string &role)
{
    repo_->fetchUserByName(name, id,  password, role) ;

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

bool DefaultAuthorizationModel::can(const std::string &role, const string &action) const {

    vector<string> permissions  = getPermissions(role) ;
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

string Authenticator::sanitizeUserName(const string &username)
{
    return strip_all_tags(username) ;
}

string Authenticator::sanitizePassword(const string &password)
{
    return trim_copy(password) ;
}

void Authenticator::create(const string &username, const string &email, const string &password, const string &role, string &url)
{
    string secure_pass = encodeBase64(passwordHash(password)) ;
    repo_->createUser(email, username, secure_pass, role, url) ;
}

void Authenticator::updatePassword(const string &id, const string &password)
{
    string secure_pass = encodeBase64(passwordHash(password)) ;
    repo_->updatePassword(id, secure_pass) ;

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

void UserRepository::createUser(const string &email, const string &username, const string &password, const string &role, string &url)
{
    con_.execute("INSERT INTO users ( name, password, role ) VALUES ( ?, ?, ? )",  username, password, role);

    int64_t id = con_.last_insert_rowid() ;
    con_.execute("INSERT INTO user_info (user_id, email, last_sign_in) VALUES ( ?, ?, ? )", id, email, time(nullptr) ) ;

    string selector = encodeBase64(randomBytes(12)) ;
    string token = randomBytes(24) ;
    time_t expires 	= std::time(nullptr) + 3600*24*10; // Expire in 10 days

    string htoken = binToHex(hashSHA256(token)) ;
    addAuthToken(to_string(id), selector, htoken, expires) ;

    url = "login/activate?id=" + to_string(id) + "&s=" + selector + "&t=" + encodeBase64(token) ;
}


void UserRepository::updatePassword(const string &id, const string &password) {
    con_.execute("UPDATE users SET password = ? WHERE id = ?", password, id) ;
}

void UserRepository::removeExpiredTokens() {
    con_.execute("DELETE FROM auth_tokens WHERE expires < ?", time(nullptr)) ;
}

void UserRepository::addAuthToken(const string &user_id, const string &selector, const string &token, time_t expires)
{
    con_.execute("INSERT INTO auth_tokens ( user_id, selector, token, expires ) VALUES ( ?, ?, ?, ? );", user_id, selector, token, expires) ;

}

void UserRepository::updateLastSignIn(const string &user_id, time_t t) {
    con_.execute("UPDATE user_info SET last_sign_in = ? WHERE user_id = ?", t, user_id) ;
}

void UserRepository::removeAuthToken(const string &user_id)
{
    con_.execute("DELETE FROM auth_tokens WHERE user_id = ?", user_id) ;
}

bool UserRepository::rememberUser(const string &selector, const std::string &token, string &user_id, string &user_name, string &user_role)
{

    QueryResult res = con_.query(
                "SELECT a.user_id as user_id, a.token as token, u.name as username, r.role_id as role FROM auth_tokens AS a JOIN users AS u ON a.user_id = u.id JOIN user_roles AS r ON r.user_id = u.id WHERE a.selector = ? AND a.expires > ? LIMIT 1",
                selector, std::time(nullptr)) ;

    if ( res.next() ) {

        string cookie_token = binToHex(hashSHA256(decodeBase64(token))) ;
        string stored_token = res.get<string>("token") ;

        if ( hash_equals(stored_token, cookie_token) ) {
            user_name = res.get<string>("username") ;
            user_id = res.get<string>("user_id") ;
            user_role = res.get<string>("user_role") ;

            return true ;
        }
    }
    return false ;
}

void UserRepository::fetchUserByName(const string &name, string &id, string &password, string &role)
{
    QueryResult res = con_.query("SELECT id, password, role FROM users WHERE name = ? LIMIT 1;", name) ;

    if ( res.next() ) {
        id = res.get<string>("id") ;
        password = res.get<string>("password") ;
        role = res.get<string>("role") ;
    }

}


bool UserRepository::userEmailExists(const string &email) {
    QueryResult res = con_.query("SELECT id FROM 'users' WHERE email = ? LIMIT 1;", email) ;
    return res.next() ;
}

bool UserRepository::userNameExists(const string &name) {
    QueryResult res = con_.query("SELECT id FROM 'users' WHERE name = ? LIMIT 1;", name) ;
    return res.next() ;
}

void UserRepository::create() {
    con_.execute("CREATE TABLE IF NOT EXISTS " + prefix_ + "users ( id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, password TEXT NOT NULL, enabled INTEGER DEFAULT 0, role TEXT NOT NULL );") ;
    con_.execute("CREATE TABLE IF NOT EXISTS " + prefix_ + "user_info ( user_id INTEGER PRIMARY KEY, email TEXT NOT NULL, last_sign_in INTEGER NOT NULL, FOREIGN KEY(user_id) REFERENCES users(id) );") ;
    con_.execute("CREATE TABLE IF NOT EXISTS " + prefix_ + "auth_tokens ( id INTEGER PRIMARY KEY AUTOINCREMENT, token TEXT, user_id INTEGER NOT NULL, expires INTEGER, FOREIGN KEY(user_id) REFERENCES users(id) );") ;
}

bool UserRepository::activate(const string &id, const string &selector, const string &token) {
    QueryResult res = con_.query(
                "SELECT token FROM auth_tokens WHERE selector = ? AND user_id = ? AND expires > ? LIMIT 1",
                selector, id, std::time(nullptr)) ;

    if ( res.next() ) {

        string btoken = decodeBase64(token) ;
        string cookie_token = binToHex(hashSHA256(btoken)) ;
        string stored_token = res.get<string>("token") ;

        if ( hash_equals(stored_token, cookie_token) ) {
            con_.execute("DELETE FROM auth_tokens WHERE user_id=? AND selector = ?", id, selector) ;
            con_.execute("UPDATE users SET enabled = 1 WHERE id=?", id) ;
            return true ;
        }
    }


    return false ;
}
