//
/// Created by l2pic on 17.05.2021.
//

#ifndef CHATCONTROLLER_COMMON_URI_H_
#define CHATCONTROLLER_COMMON_URI_H_

#include <vector>
#include <string>

class URI
{
  public:
    /// Creates an empty URI.
    URI();
    /// Parses an URI from the given string. Throws a
    /// SyntaxException if the uri is not valid.
    URI(const std::string &uri);

    /// Parses an URI from the given string. Throws a
    /// SyntaxException if the uri is not valid.
    URI(const char *uri);

    /// Creates an URI from its parts.
    URI(const std::string &scheme, const std::string &pathEtc);

    /// Creates an URI from its parts.
    URI(const std::string &scheme, const std::string &authority, const std::string &pathEtc);

    /// Creates an URI from its parts.
    URI(const std::string &scheme, const std::string &authority, const std::string &path, const std::string &query);

    /// Creates an URI from its parts.
    URI(const std::string &scheme, const std::string &authority, const std::string &path,
        const std::string &query, const std::string &fragment);

    /// Copy constructor. Creates an URI from another one.
    URI(const URI &uri);

    /// Creates an URI from a base URI and a relative URI, according to
    /// the algorithm in section 5.2 of RFC 3986.
    URI(const URI &baseURI, const std::string &relativeURI);

    ~URI();

    /// Assignment operator.
    URI &operator=(const URI &uri);

    /// Parses and assigns an URI from the given string. Throws a
    /// SyntaxException if the uri is not valid.
    URI &operator=(const std::string &uri);

    /// Parses and assigns an URI from the given string. Throws a
    /// SyntaxException if the uri is not valid.
    URI &operator=(const char *uri);

    /// Swaps the URI with another one.
    void swap(URI &uri);

    /// Swaps one URI with another one.
    friend inline void swap(URI &lhs, URI &rhs) {
        lhs.swap(rhs);
    }

    ///  Clears all parts of the URI.
    void clear();

    /// Returns a string representation of the URI.
    ///
    /// Characters in the path, query and fragment parts will be
    /// percent-encoded as necessary.
    [[nodiscard]] std::string toString() const;

    /// Returns the scheme part of the URI.
    [[nodiscard]] const std::string &getScheme() const;

    /// Sets the scheme part of the URI. The given scheme
    /// is converted to lower-case.
    ///
    /// A list of registered URI schemes can be found
    /// at <http://www.iana.org/assignments/uri-schemes>.
    void setScheme(const std::string &scheme);

    /// Returns the user-info part of the URI.
    [[nodiscard]] const std::string &getUserInfo() const;

    /// Sets the user-info part of the URI.
    void setUserInfo(const std::string &userInfo);

    /// Returns the host part of the URI.
    [[nodiscard]] const std::string &getHost() const;

    /// Sets the host part of the URI.
    void setHost(const std::string &host);

    /// Returns the port number part of the URI.
    ///
    /// If no port number (0) has been specified, the
    /// well-known port number (e.g., 80 for http) for
    /// the given scheme is returned if it is known.
    /// Otherwise, 0 is returned.
    [[nodiscard]] unsigned short getPort() const;

    /// Sets the port number part of the URI.
    void setPort(unsigned short port);

    /// Returns the authority part (userInfo, host and port)
    /// of the URI.
    ///
    /// If the port number is a well-known port
    /// number for the given scheme (e.g., 80 for http), it
    /// is not included in the authority.
    [[nodiscard]] std::string getAuthority() const;


    /// Parses the given authority part for the URI and sets
    /// the user-info, host, port components accordingly.
    void setAuthority(const std::string &authority);

    /// Returns the path part of the URI.
    [[nodiscard]] const std::string &getPath() const;

    /// Sets the path part of the URI.
    void setPath(const std::string &path);

    [[nodiscard]] std::string getQuery() const;
    /// Returns the query part of the URI.

    /// Sets the query part of the URI.
    void setQuery(const std::string &query);

    /// Returns the unencoded query part of the URI.
    [[nodiscard]] const std::string &getRawQuery() const;

    /// Sets the query part of the URI.
    void setRawQuery(const std::string &query);

    /// Returns the fragment part of the URI.
    [[nodiscard]] const std::string &getFragment() const;

    /// Sets the fragment part of the URI.
    void setFragment(const std::string &fragment);

    /// Sets the path, query and fragment parts of the URI.
    void setPathEtc(const std::string &pathEtc);

    /// Returns the path, query and fragment parts of the URI.
    [[nodiscard]] std::string getPathEtc() const;

    /// Returns the path and query parts of the URI.
    [[nodiscard]] std::string getPathAndQuery() const;

    /// Resolves the given relative URI against the base URI.
    /// See section 5.2 of RFC 3986 for the algorithm used.
    void resolve(const std::string &relativeURI);

    /// Resolves the given relative URI against the base URI.
    /// See section 5.2 of RFC 3986 for the algorithm used.
    void resolve(const URI &relativeURI);

    /// Returns true if the URI is a relative reference, false otherwise.
    ///
    /// A relative reference does not contain a scheme identifier.
    /// Relative references are usually resolved against an absolute
    /// base reference.
    [[nodiscard]] bool isRelative() const;

    /// Returns true if the URI is empty, false otherwise.
    [[nodiscard]] bool empty() const;

    /// Returns true if both URIs are identical, false otherwise.
    ///
    /// Two URIs are identical if their scheme, authority,
    /// path, query and fragment part are identical.
    bool operator==(const URI &uri) const;

    /// Parses the given URI and returns true if both URIs are identical,
    /// false otherwise.
    bool operator==(const std::string &uri) const;

    /// Returns true if both URIs are identical, false otherwise.
    bool operator!=(const URI &uri) const;

    /// Parses the given URI and returns true if both URIs are identical,
    /// false otherwise.
    bool operator!=(const std::string &uri) const;

    /// Normalizes the URI by removing all but leading . and .. segments from the path.
    ///
    /// If the first path segment in a relative path contains a colon (:),
    /// such as in a Windows path containing a drive letter, a dot segment (./)
    /// is prepended in accordance with section 3.3 of RFC 3986.
    void normalize();

    /// Places the single path segments (delimited by slashes) into the
    /// given vector.
    void getPathSegments(std::vector<std::string> &segments);

    static /// URI-encodes the given string by escaping reserved and non-ASCII
    void encode(const std::string &str,/// characters. The encoded string is appended to encodedStr.
                const std::string &reserved, std::string &encodedStr);

    /// URI-decodes the given string by replacing percent-encoded
    /// characters with the actual character. The decoded string
    /// is appended to decodedStr.
    static void decode(const std::string &str, std::string &decodedStr);

  protected:
    /// Returns true if both uri's are equivalent.
    [[nodiscard]] bool equals(const URI &uri) const;

    /// Returns true if the URI's port number is a well-known one
    /// (for example, 80, if the scheme is http).
    [[nodiscard]] bool isWellKnownPort() const;

    /// Returns the well-known port number for the URI's scheme,
    /// or 0 if the port number is not known.
    [[nodiscard]] unsigned short getWellKnownPort() const;

    /// Parses and assigns an URI from the given string. Throws a
    /// SyntaxException if the uri is not valid.
    void parse(const std::string &uri);

    /// Parses and sets the user-info, host and port from the given data.
    void parseAuthority(std::string::const_iterator &it, const std::string::const_iterator &end);

    /// Parses and sets the host and port from the given data.
    void parseHostAndPort(std::string::const_iterator &it, const std::string::const_iterator &end);

    /// Parses and sets the path from the given data.
    void parsePath(std::string::const_iterator &it, const std::string::const_iterator &end);

    /// Parses and sets the path, query and fragment from the given data.
    void parsePathEtc(std::string::const_iterator &it, const std::string::const_iterator &end);

    /// Parses and sets the query from the given data.
    void parseQuery(std::string::const_iterator &it, const std::string::const_iterator &end);

    ///  Parses and sets the fragment from the given data.
    void parseFragment(std::string::const_iterator &it, const std::string::const_iterator &end);

    /// Appends a path to the URI's path.
    void mergePath(const std::string &path);

    /// Removes all dot segments from the path.
    void removeDotSegments(bool removeLeading = true);

    /// Places the single path segments (delimited by slashes) into the
    /// given vector.
    static void getPathSegments(const std::string &path, std::vector<std::string> &segments);

    /// Builds the path from the given segments.
    void buildPath(const std::vector<std::string> &segments, bool leadingSlash, bool trailingSlash);

    static inline const std::string RESERVED_PATH = "?#";
    static inline const std::string RESERVED_QUERY = "#";
    static inline const std::string RESERVED_FRAGMENT = "";
    static inline const std::string ILLEGAL = "%<>{}|\\\"^`";
  private:
    std::string scheme;
    std::string userInfo;
    std::string host;
    unsigned short port = 0;
    std::string path;
    std::string query;
    std::string fragment;
};

#endif //CHATCONTROLLER_COMMON_URI_H_
