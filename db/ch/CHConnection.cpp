//
// Created by l2pic on 11.03.2021.
//

#include "../../common/Logger.h"
#include "CHConnection.h"

CHConnection::CHConnection(const CHConnectionConfig &config, std::shared_ptr<Logger> logger)
    : DBConnection(std::move(logger)) {
    auto opt = clickhouse::ClientOptions{}
        .SetHost(config.host)
        .SetPort(config.port)
        .SetUser(config.user)
        .SetPassword(config.pass)
        .SetDefaultDatabase(config.dbname)
        .SetSendRetries(config.sendRetries)
        .SetPingBeforeQuery(true)
        .TcpKeepAlive(true);

    try {
        conn = std::make_shared<clickhouse::Client>(std::move(opt));
        this->logger->logInfo("CHConnection connected on {}:{}/{}", config.host, config.port, config.dbname);
        established = true;
    } catch (const std::runtime_error& err) {
        this->logger->logCritical("CHConnection {}", err.what());
    }
}



