//
// Created by l2pic on 12.03.2021.
//

#include "DBConnection.h"

DBConnection::DBConnection(std::shared_ptr<Logger> logger) : logger(std::move(logger)) {};

DBConnection::~DBConnection() = default;
