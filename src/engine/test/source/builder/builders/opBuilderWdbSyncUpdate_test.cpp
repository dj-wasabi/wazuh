/* Copyright (C) 2015-2022, Wazuh Inc.
 * All rights reserved.
 *
 */

#include <any>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <baseTypes.hpp>
#include <utils/socketInterface/unixDatagram.hpp>
#include <utils/socketInterface/unixSecureStream.hpp>

#include <logging/logging.hpp>
#include <wdb/wdb.hpp>

#include "opBuilderWdbSync.hpp"
#include "socketAuxiliarFunctions.hpp"

using namespace base;
using namespace wazuhdb;
namespace bld = builder::internals::builders;
namespace unixStream = base::utils::socketInterface;

// Build ok
TEST(opBuilderWdbSyncUpdate, Build)
{
    auto tuple {std::make_tuple(
        std::string {"/sourceField"},
        std::string {"wdb_update"},
        std::vector<std::string> {"agent 007 syscheck integrity_clear ...."})};

    ASSERT_NO_THROW(bld::opBuilderWdbSyncUpdate(tuple));
}

TEST(opBuilderWdbSyncUpdate, BuildsWithJson)
{

    auto tuple {std::make_tuple(
        std::string {"/sourceField"},
        std::string {"wdb_update"},
        std::vector<std::string> {
            "agent 007 syscheck integrity_clear {\"tail\": \"tail\", "
            "\"checksum\":\"checksum\", \"begin\": \"/a/path\", \"end\": \"/z/path\"}"})};

    ASSERT_NO_THROW(bld::opBuilderWdbSyncUpdate(tuple));
}

TEST(opBuilderWdbSyncUpdate, BuildsWithQueryRef)
{

    auto tuple {std::make_tuple(std::string {"/wdb/result"},
                                std::string {"wdb_update"},
                                std::vector<std::string> {"$wdb.query_parameters"})};

    ASSERT_NO_THROW(bld::opBuilderWdbSyncUpdate(tuple));
}

TEST(opBuilderWdbSyncUpdate, checkWrongQttyParams)
{

    auto tuple {
        std::make_tuple(std::string {"/wdb/result"},
                        std::string {"wdb_update"},
                        std::vector<std::string> {"$wdb.query_parameters", "param2"})};

    ASSERT_THROW(bld::opBuilderWdbSyncUpdate(tuple), std::runtime_error);
}

TEST(opBuilderWdbSyncUpdate, gettingEmptyReference)
{
    auto tuple {std::make_tuple(std::string {"/wdb/result"},
                                std::string {"wdb_update"},
                                std::vector<std::string> {"$wdb.query_parameters"})};

    auto op {bld::opBuilderWdbSyncUpdate(tuple)->getPtr<Term<EngineOp>>()->getFn()};
    auto event1 {std::make_shared<json::Json>(R"({"wdb": {
        "query_parameters": ""}
    })")};

    result::Result<Event> result1 {op(event1)};
    ASSERT_FALSE(result1);
}

TEST(opBuilderWdbSyncUpdate, gettingNonExistingReference)
{
    auto tuple {std::make_tuple(std::string {"/wdb/result"},
                                std::string {"wdb_update"},
                                std::vector<std::string> {"$wdb.query_parameters"})};

    auto op {bld::opBuilderWdbSyncUpdate(tuple)->getPtr<Term<EngineOp>>()->getFn()};
    auto event1 {std::make_shared<json::Json>(R"({"wdb": {
        "not_query_parameters": "something"}
    })")};

    result::Result<Event> result1 {op(event1)};
    ASSERT_FALSE(result1);
}

TEST(opBuilderWdbSyncUpdate, completeFunctioningWithBadResponse)
{

    auto tuple {std::make_tuple(std::string {"/wdb/result"},
                                std::string {"wdb_update"},
                                std::vector<std::string> {"$wdb.query_parameters"})};

    auto op {bld::opBuilderWdbSyncUpdate(tuple)->getPtr<Term<EngineOp>>()->getFn()};
    auto event1 {std::make_shared<json::Json>(R"({"wdb": {
        "query_parameters": "agent 007 syscheck integrity_clear {\"tail\": \"tail\", \"checksum\": \"checksum\", \"begin\": \"path\", \"end\": \"path\"}"}
    })")};

    // Create the endpoint for test
    const int serverSocketFD {testBindUnixSocket(TEST_STREAM_SOCK_PATH, SOCK_STREAM)};
    ASSERT_GT(serverSocketFD, 0);

    std::thread t([&]() {
        const int clientRemote {testAcceptConnection(serverSocketFD)};
        testRecvString(clientRemote, SOCK_STREAM);
        testSendMsg(clientRemote, "NotOk");
        close(clientRemote);
    });

    // Disable error logs for this test
    const auto logLevel {fmtlog::getLogLevel()};
    fmtlog::setLogLevel(fmtlog::LogLevel(logging::LogLevel::Off));

    result::Result<Event> result1 {op(event1)};

    fmtlog::setLogLevel(fmtlog::LogLevel(logLevel)); // Restore log level

    ASSERT_TRUE(result1);
    ASSERT_TRUE(result1.payload()->isBool("/wdb/result"));
    ASSERT_FALSE(result1.payload()->getBool("/wdb/result").value());

    t.join();
    close(serverSocketFD);
}

TEST(opBuilderWdbSyncUpdate, completeFunctioningWithOkResponse)
{
    auto tuple {std::make_tuple(std::string {"/wdb/result"},
                                std::string {"wdb_update"},
                                std::vector<std::string> {"$wdb.query_parameters"})};

    auto op {bld::opBuilderWdbSyncUpdate(tuple)->getPtr<Term<EngineOp>>()->getFn()};
    auto event1 {std::make_shared<json::Json>(R"({"wdb": {
        "query_parameters": "agent 007 syscheck integrity_clear {\"tail\": \"tail\", \"checksum\": \"checksum\", \"begin\": \"path\", \"end\": \"path\"}"}
    })")};

    // Create the endpoint for test
    const int serverSocketFD {testBindUnixSocket(TEST_STREAM_SOCK_PATH, SOCK_STREAM)};
    ASSERT_GT(serverSocketFD, 0);

    std::thread t([&]() {
        const int clientRemote {testAcceptConnection(serverSocketFD)};
        testRecvString(clientRemote, SOCK_STREAM);
        testSendMsg(clientRemote, "ok");
        close(clientRemote);
    });

    result::Result<Event> result1 {op(event1)};

    ASSERT_TRUE(result1);
    ASSERT_TRUE(result1.payload()->isBool("/wdb/result"));
    ASSERT_TRUE(result1.payload()->getBool("/wdb/result").value());

    t.join();
    close(serverSocketFD);
}

TEST(opBuilderWdbSyncUpdate, completeFunctioningWithOkResponseWPayload)
{
    auto tuple {std::make_tuple(std::string {"/wdb/result"},
                                std::string {"wdb_update"},
                                std::vector<std::string> {"$wdb.query_parameters"})};

    auto op {bld::opBuilderWdbSyncUpdate(tuple)->getPtr<Term<EngineOp>>()->getFn()};
    auto event1 {std::make_shared<json::Json>(R"({"wdb": {
        "query_parameters": "agent 007 syscheck integrity_clear {\"tail\": \"tail\", \"checksum\": \"checksum\", \"begin\": \"path\", \"end\": \"path\"}"}
    })")};

    // Create the endpoint for test
    const int serverSocketFD {testBindUnixSocket(TEST_STREAM_SOCK_PATH, SOCK_STREAM)};
    ASSERT_GT(serverSocketFD, 0);

    std::thread t([&]() {
        const int clientRemote {testAcceptConnection(serverSocketFD)};
        testRecvString(clientRemote, SOCK_STREAM);
        testSendMsg(clientRemote, "ok with discart payload");
        close(clientRemote);
    });

    result::Result<Event> result1 {op(event1)};
    ASSERT_TRUE(result1);
    ASSERT_TRUE(result1.payload()->isBool("/wdb/result"));
    ASSERT_TRUE(result1.payload()->getBool("/wdb/result").value());

    t.join();
    close(serverSocketFD);
}

TEST(opBuilderWdbSyncUpdate, QueryResultCodeNotOkWithPayload)
{
    auto tuple {std::make_tuple(std::string {"/wdb/result"},
                                std::string {"wdb_update"},
                                std::vector<std::string> {"$wdb.query_parameters"})};

    auto op {bld::opBuilderWdbSyncUpdate(tuple)->getPtr<Term<EngineOp>>()->getFn()};
    auto event1 {std::make_shared<json::Json>(R"({"wdb": {
        "query_parameters": "agent 007 syscheck integrity_clear {\"tail\": \"tail\", \"checksum\": \"checksum\", \"begin\": \"path\", \"end\": \"path\"}"}
    })")};

    // Create the endpoint for test
    const int serverSocketFD {testBindUnixSocket(TEST_STREAM_SOCK_PATH, SOCK_STREAM)};
    ASSERT_GT(serverSocketFD, 0);

    std::thread t([&]() {
        const int clientRemote {testAcceptConnection(serverSocketFD)};
        testRecvString(clientRemote, SOCK_STREAM);
        testSendMsg(clientRemote, "Random payload");
        close(clientRemote);
    });

    // Disable error logs for this test
    const auto logLevel {fmtlog::getLogLevel()};
    fmtlog::setLogLevel(fmtlog::LogLevel(logging::LogLevel::Off));

    result::Result<Event> result1 {op(event1)};

    fmtlog::setLogLevel(fmtlog::LogLevel(logLevel)); // Restore log level

    ASSERT_TRUE(result1);
    ASSERT_TRUE(result1.payload()->isBool("/wdb/result"));
    ASSERT_FALSE(result1.payload()->getBool("/wdb/result").value());

    t.join();
    close(serverSocketFD);
}

TEST(opBuilderWdbSyncUpdate, QueryResultCodeOkPayloadEmpty)
{
    auto tuple {std::make_tuple(std::string {"/wdb/result"},
                                std::string {"wdb_update"},
                                std::vector<std::string> {"$wdb.query_parameters"})};

    auto op {bld::opBuilderWdbSyncUpdate(tuple)->getPtr<Term<EngineOp>>()->getFn()};
    auto event1 {std::make_shared<json::Json>(R"({"wdb": {
        "query_parameters": "agent 007 syscheck integrity_clear {\"tail\": \"tail\", \"checksum\": \"checksum\", \"begin\": \"path\", \"end\": \"path\"}"}
    })")};

    // Create the endpoint for test
    const int serverSocketFD {testBindUnixSocket(TEST_STREAM_SOCK_PATH, SOCK_STREAM)};
    ASSERT_GT(serverSocketFD, 0);

    std::thread t([&]() {
        const int clientRemote {testAcceptConnection(serverSocketFD)};
        testRecvString(clientRemote, SOCK_STREAM);
        testSendMsg(clientRemote, "ok "); // ok followed by an empty space
        close(clientRemote);
    });

    result::Result<Event> result1 {op(event1)};
    ASSERT_TRUE(result1);
    ASSERT_TRUE(result1.payload()->isBool("/wdb/result"));
    ASSERT_TRUE(result1.payload()->getBool("/wdb/result").value());

    t.join();
    close(serverSocketFD);
}