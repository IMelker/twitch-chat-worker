//
// Created by l2pic on 17.05.2021.
//

#include <map>

#include "Utils.h"
#include "Exception.h"
#include "URI.h"

static const std::map<std::string, unsigned short> schemeToPort = {
    { "acap"    , 674 },
    { "afp"     , 548 },
    { "dict"    , 2628 },
    { "dns"     , 53 },
    { "ftp"     , 21 },
    { "git"     , 9418 },
    { "gopher"  , 70 },
    { "http"    , 80 },
    { "https"   , 443 },
    { "imap"    , 143 },
    { "ipp"     , 631 },
    { "ipps"    , 631 },
    { "irc"     , 194 },
    { "ircs"    , 6697 },
    { "ldap"    , 389 },
    { "ldaps"   , 636 },
    { "mms"     , 1755 },
    { "msrp"    , 2855 },
    { "mtqp"    , 1038 },
    { "nfs"     , 111 },
    { "nntp"    , 119 },
    { "nntps"   , 563 },
    { "pop"     , 110 },
    { "prospero", 1525 },
    { "redis"   , 6379 },
    { "rsync"   , 873 },
    { "rtmp"    , 1935 },
    { "rtsp"    , 554 },
    { "rtsps"   , 322 },
    { "rtspu"   , 5005 },
    { "sftp"    , 22 },
    { "smb"     , 445 },
    { "snmp"    , 161 },
    { "ssh"     , 22 },
    { "svn"     , 3690 },
    { "sip"     , 5060 },
    { "sips"    , 5061 },
    { "telnet"  , 23 },
    { "ventrilo", 3784 },
    { "vnc"     , 5900 },
    { "wais"    , 210 },
    { "ws"      , 80 },
    { "wss"     , 443 },
    { "xmpp"    , 5222}
};

URI::URI() = default;

URI::URI(const std::string &uri) {
    parse(uri);
}

URI::URI(const char *uri) {
    parse(std::string(uri));
}

URI::URI(const std::string &scheme, const std::string &pathEtc) :
    scheme(scheme) {
    Utils::String::toLower(&this->scheme);
    port = getWellKnownPort();
    auto beg = pathEtc.cbegin();
    auto end = pathEtc.cend();
    parsePathEtc(beg, end);
}

URI::URI(const std::string &scheme, const std::string &authority, const std::string &pathEtc) :
    scheme(scheme) {
    Utils::String::toLower(&this->scheme);
    auto beg = authority.cbegin();
    auto end = authority.cend();
    parseAuthority(beg, end);
    beg = pathEtc.begin();
    end = pathEtc.end();
    parsePathEtc(beg, end);
}

URI::URI(const std::string &scheme, const std::string &authority, const std::string &path, const std::string &query) :
    scheme(scheme),
    path(path),
    query(query) {
    Utils::String::toLower(&this->scheme);
    auto beg = authority.cbegin();
    auto end = authority.cend();
    parseAuthority(beg, end);
}

URI::URI(const std::string &scheme, const std::string &authority, const std::string &path, const std::string &query, const std::string &fragment) :
    scheme(scheme),
    path(path),
    query(query),
    fragment(fragment) {
    Utils::String::toLower(&this->scheme);
    auto beg = authority.cbegin();
    auto end = authority.cend();
    parseAuthority(beg, end);
}

URI::~URI() = default;

URI::URI(const URI &uri) = default;
URI::URI(const URI &baseURI, const std::string &relativeURI) :
    scheme(baseURI.scheme),
    userInfo(baseURI.userInfo),
    host(baseURI.host),
    port(baseURI.port),
    path(baseURI.path),
    query(baseURI.query),
    fragment(baseURI.fragment) {
    resolve(relativeURI);
}

URI &URI::operator=(const URI &uri) {
    if (&uri != this) {
        scheme = uri.scheme;
        userInfo = uri.userInfo;
        host = uri.host;
        port = uri.port;
        path = uri.path;
        query = uri.query;
        fragment = uri.fragment;
    }
    return *this;
}

URI &URI::operator=(const std::string &uri) {
    clear();
    parse(uri);
    return *this;
}

URI &URI::operator=(const char *uri) {
    clear();
    parse(std::string(uri));
    return *this;
}

void URI::swap(URI &uri) {
    std::swap(scheme, uri.scheme);
    std::swap(userInfo, uri.userInfo);
    std::swap(host, uri.host);
    std::swap(port, uri.port);
    std::swap(path, uri.path);
    std::swap(query, uri.query);
    std::swap(fragment, uri.fragment);
}

void URI::clear() {
    scheme.clear();
    userInfo.clear();
    host.clear();
    port = 0;
    path.clear();
    query.clear();
    fragment.clear();
}

std::string URI::toString() const {
    std::string uri;
    if (isRelative()) {
        encode(path, RESERVED_PATH, uri);
    } else {
        uri = scheme;
        uri += ':';
        std::string auth = getAuthority();
        if (!auth.empty() || scheme == "file") {
            uri.append("//");
            uri.append(auth);
        }
        if (!path.empty()) {
            if (!auth.empty() && path[0] != '/')
                uri += '/';
            encode(path, RESERVED_PATH, uri);
        } else if (!query.empty() || !fragment.empty()) {
            uri += '/';
        }
    }
    if (!query.empty()) {
        uri += '?';
        uri.append(query);
    }
    if (!fragment.empty()) {
        uri += '#';
        encode(fragment, RESERVED_FRAGMENT, uri);
    }
    return uri;
}

void URI::setScheme(const std::string &scheme) {
    this->scheme = scheme;
    Utils::String::toLower(&this->scheme);
    if (port == 0)
        port = getWellKnownPort();
}

const std::string &URI::getScheme() const {
    return scheme;
}

void URI::setUserInfo(const std::string &info) {
    userInfo.clear();
    decode(info, this->userInfo);
}

const std::string &URI::getUserInfo() const {
    return userInfo;
}

void URI::setHost(const std::string &newHost) {
    host = newHost;
}

const std::string &URI::getHost() const {
    return host;
}

unsigned short URI::getPort() const {
    if (port == 0)
        return getWellKnownPort();
    else
        return port;
}

void URI::setPort(unsigned short newPort) {
    port = newPort;
}

std::string URI::getAuthority() const {
    std::string auth;
    if (!userInfo.empty()) {
        auth.append(userInfo);
        auth += '@';
    }
    if (host.find(':') != std::string::npos) {
        auth += '[';
        auth += host;
        auth += ']';
    } else auth.append(host);
    if (port && !isWellKnownPort()) {
        auth += ':';
        auth += std::to_string(port);
    }
    return auth;
}

void URI::setAuthority(const std::string &authority) {
    userInfo.clear();
    host.clear();
    port = 0;
    auto beg = authority.cbegin();
    auto end = authority.cend();
    parseAuthority(beg, end);
}

void URI::setPath(const std::string &newPath) {
    path.clear();
    decode(newPath, path);
}

const std::string &URI::getPath() const {
    return path;
}

void URI::setRawQuery(const std::string &newQuery) {
    query = newQuery;
}

const std::string &URI::getRawQuery() const {
    return query;
}

void URI::setQuery(const std::string &newQuery) {
    query.clear();
    encode(newQuery, RESERVED_QUERY, query);
}

std::string URI::getQuery() const {
    std::string decoded;
    decode(query, decoded);
    return decoded;
}

void URI::setFragment(const std::string &newFragment) {
    fragment.clear();
    decode(newFragment, fragment);
}

const std::string &URI::getFragment() const {
    return fragment;
}

void URI::setPathEtc(const std::string &pathEtc) {
    path.clear();
    query.clear();
    fragment.clear();
    auto beg = pathEtc.cbegin();
    auto end = pathEtc.cend();
    parsePathEtc(beg, end);
}

std::string URI::getPathEtc() const {
    std::string pathEtc;
    encode(path, RESERVED_PATH, pathEtc);
    if (!query.empty()) {
        pathEtc += '?';
        pathEtc += query;
    }
    if (!fragment.empty()) {
        pathEtc += '#';
        encode(fragment, RESERVED_FRAGMENT, pathEtc);
    }
    return pathEtc;
}

std::string URI::getPathAndQuery() const {
    std::string pathAndQuery;
    encode(path, RESERVED_PATH, pathAndQuery);
    if (!query.empty()) {
        pathAndQuery += '?';
        pathAndQuery += query;
    }
    return pathAndQuery;
}

void URI::resolve(const std::string &relativeURI) {
    URI parsedURI(relativeURI);
    resolve(parsedURI);
}

void URI::resolve(const URI &relativeURI) {
    if (!relativeURI.scheme.empty()) {
        scheme = relativeURI.scheme;
        userInfo = relativeURI.userInfo;
        host = relativeURI.host;
        port = relativeURI.port;
        path = relativeURI.path;
        query = relativeURI.query;
        removeDotSegments();
    } else {
        if (!relativeURI.host.empty()) {
            userInfo = relativeURI.userInfo;
            host = relativeURI.host;
            port = relativeURI.port;
            path = relativeURI.path;
            query = relativeURI.query;
            removeDotSegments();
        } else {
            if (relativeURI.path.empty()) {
                if (!relativeURI.query.empty())
                    query = relativeURI.query;
            } else {
                if (relativeURI.path[0] == '/') {
                    path = relativeURI.path;
                    removeDotSegments();
                } else {
                    mergePath(relativeURI.path);
                }
                query = relativeURI.query;
            }
        }
    }
    fragment = relativeURI.fragment;
}

bool URI::isRelative() const {
    return scheme.empty();
}

bool URI::empty() const {
    return scheme.empty() && host.empty() && path.empty() && query.empty() && fragment.empty();
}

bool URI::operator==(const URI &uri) const {
    return equals(uri);
}

bool URI::operator==(const std::string &uri) const {
    URI parsedURI(uri);
    return equals(parsedURI);
}

bool URI::operator!=(const URI &uri) const {
    return !equals(uri);
}

bool URI::operator!=(const std::string &uri) const {
    URI parsedURI(uri);
    return !equals(parsedURI);
}

bool URI::equals(const URI &uri) const {
    return scheme == uri.scheme
        && userInfo == uri.userInfo
        && host == uri.host
        && getPort() == uri.getPort()
        && path == uri.path
        && query == uri.query
        && fragment == uri.fragment;
}

void URI::normalize() {
    removeDotSegments(!isRelative());
}

void URI::removeDotSegments(bool removeLeading) {
    if (path.empty()) return;

    bool leadingSlash = *(path.begin()) == '/';
    bool trailingSlash = *(path.rbegin()) == '/';
    std::vector<std::string> segments;
    std::vector<std::string> normalizedSegments;
    getPathSegments(segments);
    for (const auto & segment : segments) {
        if (segment == "..") {
            if (!normalizedSegments.empty()) {
                if (normalizedSegments.back() == "..")
                    normalizedSegments.push_back(segment);
                else
                    normalizedSegments.pop_back();
            } else if (!removeLeading) {
                normalizedSegments.push_back(segment);
            }
        } else if (segment != ".") {
            normalizedSegments.push_back(segment);
        }
    }
    buildPath(normalizedSegments, leadingSlash, trailingSlash);
}

void URI::getPathSegments(std::vector<std::string> &segments) {
    getPathSegments(path, segments);
}

void URI::getPathSegments(const std::string &path, std::vector<std::string> &segments) {
    std::string::const_iterator it = path.begin();
    auto end = path.cend();
    std::string seg;
    while (it != end) {
        if (*it == '/') {
            if (!seg.empty()) {
                segments.push_back(seg);
                seg.clear();
            }
        } else seg += *it;
        ++it;
    }
    if (!seg.empty())
        segments.push_back(seg);
}

void URI::encode(const std::string &str, const std::string &reserved, std::string &encodedStr) {
    for (char c : str) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' ||
            c == '.' || c == '~') {
            encodedStr += c;
        }
        else if (c <= 0x20 || c >= 0x7F || ILLEGAL.find(c) != std::string::npos
            || reserved.find(c) != std::string::npos) {
            encodedStr += '%';
            encodedStr += Utils::String::Number::formatHex((unsigned) (unsigned char) c, 2);
        }
        else
            encodedStr += c;
    }
}

void URI::decode(const std::string &str, std::string &decodedStr) {
    std::string::const_iterator it = str.begin();
    auto end = str.cend();
    while (it != end) {
        char c = *it++;
        if (c == '%') {
            if (it == end)
                throw SyntaxException("URI encoding: no hex digit following percent sign", str);
            char hi = *it++;
            if (it == end)
                throw SyntaxException("URI encoding: two hex digits must follow percent sign", str);
            char lo = *it++;
            if (hi >= '0' && hi <= '9')
                c = hi - '0';
            else if (hi >= 'A' && hi <= 'F')
                c = hi - 'A' + 10;
            else if (hi >= 'a' && hi <= 'f')
                c = hi - 'a' + 10;
            else
                throw SyntaxException("URI encoding: not a hex digit");
            c *= 16;
            if (lo >= '0' && lo <= '9')
                c += lo - '0';
            else if (lo >= 'A' && lo <= 'F')
                c += lo - 'A' + 10;
            else if (lo >= 'a' && lo <= 'f')
                c += lo - 'a' + 10;
            else
                throw SyntaxException("URI encoding: not a hex digit");
        }
        decodedStr += c;
    }
}

bool URI::isWellKnownPort() const {
    return port == getWellKnownPort();
}

unsigned short URI::getWellKnownPort() const {
    auto it = schemeToPort.find(scheme);
    return it == schemeToPort.end() ? 0 : it->second;
}

void URI::parse(const std::string &uri) {
    std::string::const_iterator it = uri.begin();
    auto end = uri.cend();
    if (it == end) return;
    if (*it != '/' && *it != '.' && *it != '?' && *it != '#') {
        std::string scheme;
        while (it != end && *it != ':' && *it != '?' && *it != '#' && *it != '/') scheme += *it++;
        if (it != end && *it == ':') {
            ++it;
            if (it == end)
                throw SyntaxException("URI scheme must be followed by authority or path: " + uri);
            setScheme(scheme);
            if (*it == '/') {
                ++it;
                if (it != end && *it == '/') {
                    ++it;
                    parseAuthority(it, end);
                } else --it;
            }
            parsePathEtc(it, end);
        } else {
            it = uri.begin();
            parsePathEtc(it, end);
        }
    } else parsePathEtc(it, end);
}

void URI::parseAuthority(std::string::const_iterator &it, const std::string::const_iterator &end) {
    std::string info;
    std::string part;
    while (it != end && *it != '/' && *it != '?' && *it != '#') {
        if (*it == '@') {
            userInfo = part;
            part.clear();
        } else part += *it;
        ++it;
    }
    std::string::const_iterator pbeg = part.begin();
    std::string::const_iterator pend = part.end();
    parseHostAndPort(pbeg, pend);
    userInfo = info;
}

void URI::parseHostAndPort(std::string::const_iterator &it, const std::string::const_iterator &end) {
    if (it == end) return;
    std::string host;
    if (*it == '[') {
        // IPv6 address
        ++it;
        while (it != end && *it != ']') host += *it++;
        if (it == end)
            throw SyntaxException("unterminated IPv6 address");
        ++it;
    } else {
        while (it != end && *it != ':') host += *it++;
    }
    if (it != end && *it == ':') {
        ++it;
        std::string portStr;
        while (it != end) portStr += *it++;
        if (!portStr.empty()) {
            int nport = Utils::String::toNumber(portStr);
            if (nport > 0 && nport < 65536)
                this->port = (unsigned short) nport;
            else
                throw SyntaxException("bad or invalid port number", portStr);
        }
        else
            this->port = getWellKnownPort();
    }
    else
        port = getWellKnownPort();
    this->host = host;
    Utils::String::toLower(&this->host);
}

void URI::parsePath(std::string::const_iterator &it, const std::string::const_iterator &end) {
    std::string pathStr;
    while (it != end && *it != '?' && *it != '#') pathStr += *it++;
    decode(pathStr, path);
}

void URI::parsePathEtc(std::string::const_iterator &it, const std::string::const_iterator &end) {
    if (it == end) return;
    if (*it != '?' && *it != '#')
        parsePath(it, end);
    if (it != end && *it == '?') {
        ++it;
        parseQuery(it, end);
    }
    if (it != end && *it == '#') {
        ++it;
        parseFragment(it, end);
    }
}

void URI::parseQuery(std::string::const_iterator &it, const std::string::const_iterator &end) {
    query.clear();
    while (it != end && *it != '#') query += *it++;
}

void URI::parseFragment(std::string::const_iterator &it, const std::string::const_iterator &end) {
    std::string fragmentStr;
    while (it != end) fragmentStr += *it++;
    decode(fragmentStr, fragment);
}

void URI::mergePath(const std::string &path) {
    std::vector<std::string> segments;
    std::vector<std::string> normalizedSegments;
    bool addLeadingSlash = false;
    if (!path.empty()) {
        getPathSegments(segments);
        bool endsWithSlash = *(path.rbegin()) == '/';
        if (!endsWithSlash && !segments.empty())
            segments.pop_back();
        addLeadingSlash = path[0] == '/';
    }
    getPathSegments(path, segments);
    addLeadingSlash = addLeadingSlash || (!path.empty() && path[0] == '/');
    bool hasTrailingSlash = (!path.empty() && *(path.rbegin()) == '/');
    bool addTrailingSlash = false;
    for (const auto & segment : segments) {
        if (segment == "..") {
            addTrailingSlash = true;
            if (!normalizedSegments.empty())
                normalizedSegments.pop_back();
        } else if (segment != ".") {
            addTrailingSlash = false;
            normalizedSegments.push_back(segment);
        } else addTrailingSlash = true;
    }
    buildPath(normalizedSegments, addLeadingSlash, hasTrailingSlash || addTrailingSlash);
}

void URI::buildPath(const std::vector<std::string> &segments, bool leadingSlash, bool trailingSlash) {
    path.clear();
    bool first = true;
    for (const auto &segment : segments) {
        if (first) {
            first = false;
            if (leadingSlash)
                path += '/';
            else if (scheme.empty() && segment.find(':') != std::string::npos)
                path.append("./");
        } else path += '/';
        path.append(segment);
    }
    if (trailingSlash)
        path += '/';
}
