//
// Created by imelker on 26.03.2021.
//

#ifndef CHATSNIFFER_HTTP_BOOSTSSL_H_
#define CHATSNIFFER_HTTP_BOOSTSSL_H_

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <fstream>
#include <streambuf>

bool readFile(const std::string& filename, std::string& target, std::string& err) {
    std::ifstream file;
    try {
        file.open(filename);
        if (!file.is_open()) {
            err = filename;
            return false;
        }

        target.clear();

        file.seekg(0, std::ios::end);
        target.reserve(file.tellg());
        file.seekg(0, std::ios::beg);

        target.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return true;
    } catch (const std::ifstream::failure& e) {
        err = e.what();
        return false;
    }
}

/*  Load a signed certificate into the ssl context, and configure
    the context for use with a server.

    For this to work with the browser or operating system, it is
    necessary to import the "Beast Test CA" certificate into
    the local certificate store, browser, or operating system
    depending on your environment Please see the documentation
    accompanying the Beast certificate for more details.
*/
inline bool loadServerCertificate(boost::asio::ssl::context &ctx,
                                  std::string &err,
                                  std::string *certPath = nullptr,
                                  std::string *keyPath = nullptr,
                                  std::string *dhPath = nullptr)
{
    // TODO remake readFile cert&pem to boost buffer

    ctx.set_password_callback([](std::size_t, boost::asio::ssl::context_base::password_purpose) {
        return "test_server_password";
    });
    ctx.set_options(boost::asio::ssl::context::default_workarounds |
                    boost::asio::ssl::context::no_sslv2 |
                    boost::asio::ssl::context::single_dh_use);

    bool valid = false;
    if (certPath && !certPath->empty()) {
        std::string cert;
        if (!readFile(*certPath, cert, err))
            return false;

        ctx.use_certificate_chain(boost::asio::buffer(cert.data(), cert.size()));
        valid = true;
    }

    if (keyPath && !keyPath->empty()) {
        std::string key;
        if (!readFile(*keyPath, key, err))
            return false;

        ctx.use_private_key(boost::asio::buffer(key.data(), key.size()), boost::asio::ssl::context::pem);
        valid = true;
    }

    if (dhPath && !dhPath->empty()) {
        std::string dh;
        if (!readFile(*dhPath, dh, err))
            return false;

        ctx.use_tmp_dh(boost::asio::buffer(dh.data(), dh.size()));
        valid = true;
    }

    if (!valid) {
        err = "Empty credentials";
        return false;
    }
    return true;
}

#endif //CHATSNIFFER_HTTP_BOOSTSSL_H_
