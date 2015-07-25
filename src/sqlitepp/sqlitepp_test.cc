// Copyright (C) 2014--2015 Robin Krahl <robin.krahl@ireas.org>
// MIT license -- http://opensource.org/licenses/MIT

#include <stdexcept>
#include <fstream>
#include <iostream>
#include "gtest/gtest.h"
#include "sqlitepp/sqlitepp.h"

TEST(Database, openClose) {
  sqlitepp::Database database;
  EXPECT_FALSE(database.isOpen());
  database.open("/tmp/test.db");
  EXPECT_TRUE(database.isOpen());
  database.close();
  EXPECT_FALSE(database.isOpen());
  database.open("/tmp/test2.db");
  EXPECT_TRUE(database.isOpen());
  database.close();
  EXPECT_FALSE(database.isOpen());
  sqlitepp::Database database2("/tmp/test.db");
  EXPECT_TRUE(database2.isOpen());
  EXPECT_THROW(database2.open("/tmp/test2.db"), std::logic_error);
  EXPECT_TRUE(database2.isOpen());
  database2.close();
  EXPECT_FALSE(database2.isOpen());

  std::ifstream testStream("/tmp/test.db");
  EXPECT_TRUE(testStream.good());
  testStream.close();
  testStream.open("/tmp/test2.db");
  EXPECT_TRUE(testStream.good());
  testStream.close();
}

TEST(Database, copy) {
  sqlitepp::Database database;
  // MUST NOT COMPILE:
  // sqlitepp::Database database2 = database;
  database.close();
  sqlitepp::Database database3;
  // MUST NOT COMPILE:
  // database3 = database;
  database3.close();
}

TEST(Database, prepare) {
  sqlitepp::Database database("/tmp/test.db");
  std::shared_ptr<sqlitepp::Statement> statement = database.prepare(
      "CREATE TABLE IF NOT EXISTS test (id, value);");
  EXPECT_TRUE(statement->isOpen());
  statement->close();
  EXPECT_FALSE(statement->isOpen());
  database.close();
}

TEST(Database, execute) {
  sqlitepp::Database database("/tmp/test.db");
  database.execute("CREATE TABLE IF NOT EXISTS test (id, value);");
}

TEST(Database, insert) {
  sqlitepp::Database database("/tmp/test.db");
  std::shared_ptr<sqlitepp::Statement> statement = database.prepare(
      "INSERT INTO test (id, value) VALUES (:id, ?)");
  statement->bind(":id", 1);
  statement->bind(2, "test value");
  statement->execute();
  const int rowId1 = database.lastInsertRowId();
  EXPECT_NE(0, rowId1);
  statement->reset();
  statement->bind(":id", 2);
  statement->bind(2, "other value");
  statement->execute();
  const int rowId2 = database.lastInsertRowId();
  EXPECT_NE(0, rowId2);
  EXPECT_NE(rowId1, rowId2);
}

TEST(Database, query) {
  sqlitepp::Database database("/tmp/test.db");
  std::shared_ptr<sqlitepp::Statement> statement = database.prepare(
      "SELECT id, value FROM test;");
  sqlitepp::ResultSet resultSet = statement->execute();
  EXPECT_TRUE(resultSet.canRead());
  EXPECT_EQ(2, resultSet.columnCount());
  int id = resultSet.readInt(0);
  std::string value = resultSet.readString(1);
  EXPECT_EQ(1, id);
  EXPECT_EQ("test value", value);
  EXPECT_TRUE(resultSet.next());
  EXPECT_TRUE(resultSet.canRead());
  id = resultSet.readInt(0);
  value = resultSet.readString(1);
  EXPECT_EQ(2, id);
  EXPECT_EQ("other value", value);
  EXPECT_FALSE(resultSet.next());
  EXPECT_FALSE(resultSet.canRead());
}

TEST(Database, cleanup) {
  sqlitepp::Database database("/tmp/test.db");
  database.execute("DROP TABLE test;");
  database.close();
}
