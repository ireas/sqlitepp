/*
 * (C) 2014 Robin Krahl
 * MIT license -- http://opensource.org/licenses/MIT
 */

#include "sqlitepp.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SQLitePPTest
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>

BOOST_AUTO_TEST_CASE(openClose) {
    sqlitepp::Database database;
    BOOST_CHECK(!database.isOpen());
    database.open("/tmp/test.db");
    BOOST_CHECK(database.isOpen());
    database.close();
    BOOST_CHECK(!database.isOpen());
    database.open("/tmp/test2.db");
    BOOST_CHECK(database.isOpen());
    database.close();
    BOOST_CHECK(!database.isOpen());
    sqlitepp::Database database2("/tmp/test.db");
    BOOST_CHECK(database2.isOpen());
    try {
        database2.open("/tmp/test2.db");
        BOOST_ERROR("Calling open() to an open database does not throw an exception.");
    } catch (std::logic_error &) {
        // everything fine
    }
    BOOST_CHECK(database2.isOpen());
    database2.close();
    BOOST_CHECK(!database2.isOpen());

    std::ifstream testStream("/tmp/test.db");
    BOOST_CHECK(testStream.good());
    testStream.close();
    testStream.open("/tmp/test2.db");
    BOOST_CHECK(testStream.good());
    testStream.close();
}

BOOST_AUTO_TEST_CASE(copy) {
    sqlitepp::Database database;
    // MUST NOT COMPILE:
    // sqlitepp::Database database2 = database;
    database.close();
    sqlitepp::Database database3;
    // MUST NOT COMPILE:
    // database3 = database;
    database3.close();
}

BOOST_AUTO_TEST_CASE(prepare) {
    sqlitepp::Database database("/tmp/test.db");
    sqlitepp::Statement statement(database, "CREATE TABLE IF NOT EXISTS test (id, value);");
    // TODO check std::logic_error
    BOOST_CHECK(statement.isOpen());
    statement.finalize();
    BOOST_CHECK(!statement.isOpen());
    database.close();
}

BOOST_AUTO_TEST_CASE(execute) {
    sqlitepp::Database database("/tmp/test.db");
    database.execute("CREATE TABLE IF NOT EXISTS test (id, value);");
    sqlitepp::Statement statement(database, "INSERT INTO test (id, value) VALUES (:id, ?)");
    statement.bindInt(":id", 1);
    statement.bindString(2, "test value");
    statement.step();
    statement.reset();
    statement.bindInt(":id", 2);
    statement.bindString(2, "other value");
    statement.step();
    statement.finalize();
}

BOOST_AUTO_TEST_CASE(query) {
    sqlitepp::Database database("/tmp/test.db");
    sqlitepp::Statement statement(database, "SELECT id, value FROM test;");
    bool hasNext = statement.step();
    BOOST_CHECK(hasNext);
    BOOST_CHECK_EQUAL(statement.columnCount(), 2);
    int id = statement.readInt(0);
    std::string value = statement.readString(1);
    BOOST_CHECK_EQUAL(id, 1);
    BOOST_CHECK_EQUAL(value, "test value");
    hasNext = statement.step();
    BOOST_CHECK(hasNext);
    id = statement.readInt(0);
    value = statement.readString(1);
    BOOST_CHECK_EQUAL(id, 2);
    BOOST_CHECK_EQUAL(value, "other value");
    hasNext = statement.step();
    BOOST_CHECK(!hasNext);
    statement.finalize();
    database.close();
}

BOOST_AUTO_TEST_CASE(cleanup) {
    sqlitepp::Database database("/tmp/test.db");
    database.execute("DROP TABLE test;");
    database.close();
}