/* Copyright (C) 2015-2021, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _TCP_ENDPOINT_H_
#define _TCP_ENDPOINT_H_

#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <uvw/tcp.hpp>
#include <uvw/timer.hpp>

#include "baseEndpoint.hpp"
#include "glog/logging.h"
#include "protocolHandler.hpp"

namespace engineserver::endpoints
{

#define CONNECTION_TIMEOUT_MSEC 5000

/**
 * @brief Implements tcp server endpoint using uvw library.
 *
 */
class TCPEndpoint : public BaseEndpoint
{
private:
    int m_port;
    std::string m_ip;

    std::shared_ptr<uvw::Loop> m_loop;
    std::shared_ptr<uvw::TCPHandle> m_server;

    void connectionHandler(uvw::TCPHandle & server);

public:
    /**
     * @brief Construct a new TCPEndpoint object
     *
     * @param config
     * @param eventBuffer
     */
    explicit TCPEndpoint(const std::string & config, moodycamel::BlockingConcurrentQueue<std::string> & eventBuffer);
    ~TCPEndpoint();

    void run() override;

    void close() override;
};
} // namespace engineserver::endpoints

#endif // _TCP_ENDPOINT_H_
