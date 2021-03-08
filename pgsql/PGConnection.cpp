//
// Created by l2pic on 07.03.2021.
//

#include "PGConnection.h"

PGConnection::PGConnection(const PGConnectionConfig& config) {
    conn.reset(PQsetdbLogin(config.host.c_str(), std::to_string(config.port).c_str(), nullptr, nullptr,
                               config.dbname.c_str(), config.user.c_str(), config.pass.c_str()), &PQfinish);

    if (PQstatus(conn.get()) != CONNECTION_OK && PQsetnonblocking(conn.get(), 1) != 0) {
        fprintf(stderr, "%s\n", PQerrorMessage(conn.get()));
    } else
        established = true;
}
