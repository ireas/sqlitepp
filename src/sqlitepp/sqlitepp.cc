// Copyright (C) 2014--2015 Robin Krahl <robin.krahl@ireas.org>
// MIT license -- http://opensource.org/licenses/MIT

#include "sqlitepp/sqlitepp.h"
#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace sqlitepp {

Openable::Openable(const bool open, const std::string& name)
    : m_open(open), m_name(name) {
}

bool Openable::isOpen() const {
  return m_open;
}

void Openable::requireOpen() const {
  if (!m_open) {
    throw std::logic_error(m_name + " is not open.");
  }
}

void Openable::setOpen(const bool open) {
    m_open = open;
}

std::string DatabaseError::getErrorMessage(const int errorCode,
    const std::string& errorMessage) {
  std::ostringstream stringStream;
  stringStream << "Caught SQLite3 error " << errorCode << " meaning: "
      << errorMessage;
  return stringStream.str();
}

DatabaseError::DatabaseError(const int errorCode)
    : DatabaseError(errorCode, sqlite3_errstr(errorCode)) {
}

DatabaseError::DatabaseError(const int errorCode,
                             const std::string& errorMessage)
    : std::runtime_error(getErrorMessage(errorCode, errorMessage)),
      m_errorCode(errorCode) {
}

int DatabaseError::errorCode() const {
  return m_errorCode;
}

Statement::Statement(sqlite3_stmt* handle)
    : Openable(true, "Statement"), m_canRead(false), m_handle(handle) {
}

Statement::~Statement() {
  if (isOpen()) {
    // errors that could occur during finalizing are ignored as they have
    // already been handled!
    sqlite3_finalize(m_handle);
    setOpen(false);
  }
}

void Statement::bind(const int index, const double value) {
  requireOpen();
  handleBindResult(index, sqlite3_bind_double(m_handle, index, value));
}

void Statement::bind(const std::string& name, const double value) {
  bind(getParameterIndex(name), value);
}

void Statement::bind(const int index, const int value) {
  requireOpen();
  handleBindResult(index, sqlite3_bind_int(m_handle, index, value));
}

void Statement::bind(const std::string& name, const int value) {
  bind(getParameterIndex(name), value);
}

void Statement::bind(const int index, const std::string& value) {
  requireOpen();
  handleBindResult(index, sqlite3_bind_text(m_handle, index, value.c_str(),
      value.size(), NULL));
}

void Statement::bind(const std::string& name, const std::string& value) {
  bind(getParameterIndex(name), value);
}

ResultSet Statement::execute() {
  step();
  return ResultSet(m_instancePointer.lock());
}

void Statement::requireCanRead() const {
  if (!m_canRead) {
    throw std::logic_error("Trying to read from statement without data");
  }
}

void Statement::setInstancePointer(
    const std::weak_ptr<Statement>& instancePointer) {
  m_instancePointer = instancePointer;
}

bool Statement::step() {
  requireOpen();
  int result = sqlite3_step(m_handle);
  if (result == SQLITE_ROW) {
    m_canRead = true;
  } else if (result == SQLITE_DONE) {
    m_canRead = false;
  } else {
    throw DatabaseError(result);
  }
  return m_canRead;
}

void Statement::close() {
  if (isOpen()) {
    // errors that could occur during finalizing are ignored as they have
    // already been handled!
    sqlite3_finalize(m_handle);
    setOpen(false);
  }
}

bool Statement::reset() {
  requireOpen();
  return sqlite3_reset(m_handle) == SQLITE_OK;
}

int Statement::getParameterIndex(const std::string& name) const {
  requireOpen();
  int index = sqlite3_bind_parameter_index(m_handle, name.c_str());
  if (index == 0) {
    throw std::invalid_argument("No such parameter: " + name);
  }
  return index;
}

void Statement::handleBindResult(const int index, const int result) const {
  switch (result) {
    case SQLITE_OK:
      break;
    case SQLITE_RANGE:
      throw std::out_of_range("Bind index out of range: " + index);
    case SQLITE_NOMEM:
      throw std::runtime_error("No memory to bind parameter");
    default:
      throw DatabaseError(result);
  }
}

Database::Database() : Openable(false, "Database") {
}

Database::Database(const std::string & file) : Database() {
    open(file);
}

Database::~Database() {
  if (isOpen()) {
    sqlite3_close(m_handle);
    setOpen(false);
  }
  // m_handle is deleted by sqlite3_close
}

void Database::close() {
  if (isOpen()) {
    int result = sqlite3_close(m_handle);
    if (result == SQLITE_OK) {
      setOpen(false);
    } else {
      throw sqlitepp::DatabaseError(result);
    }
  }
}

void Database::execute(const std::string& sql) {
  requireOpen();
  std::shared_ptr<Statement> statement = prepare(sql);
  statement->step();
}

int Database::lastInsertRowId() const {
  requireOpen();
  return sqlite3_last_insert_rowid(m_handle);
}

void Database::open(const std::string& file) {
  if (isOpen()) {
    throw std::logic_error("sqlitepp::Database::open(std::string&): "
                           "Database already open");
  }
  int result = sqlite3_open(file.c_str(), &m_handle);

  if (m_handle == NULL) {
    throw std::runtime_error("sqlitepp::Database::open(std::string&): "
                             "Can't allocate memory");
  }

  if (result == SQLITE_OK) {
    setOpen(true);
  } else {
    std::string errorMessage = sqlite3_errmsg(m_handle);
    sqlite3_close(m_handle);
    throw sqlitepp::DatabaseError(result, errorMessage);
  }
}

std::shared_ptr<Statement> Database::prepare(const std::string& sql) {
  requireOpen();
  sqlite3_stmt* statementHandle;
  int result = sqlite3_prepare_v2(m_handle, sql.c_str(), sql.size(),
                                  &statementHandle, NULL);
  if (result != SQLITE_OK) {
    throw DatabaseError(result, sqlite3_errmsg(m_handle));
  }
  if (statementHandle == NULL) {
    throw std::runtime_error("Statement handle is NULL");
  }
  auto statement = std::shared_ptr<Statement>(new Statement(statementHandle));
  statement->setInstancePointer(std::weak_ptr<Statement>(statement));
  return statement;
}

ResultSet::ResultSet(const std::shared_ptr<Statement> statement)
    : m_statement(statement) {
}

bool ResultSet::canRead() const {
  return m_statement->m_canRead;
}

int ResultSet::columnCount() const {
  m_statement->requireOpen();
  m_statement->requireCanRead();
  return sqlite3_data_count(m_statement->m_handle);
}

double ResultSet::readDouble(const int column) const {
  m_statement->requireOpen();
  m_statement->requireCanRead();
  return sqlite3_column_double(m_statement->m_handle, column);
}

int ResultSet::readInt(const int column) const {
  m_statement->requireOpen();
  m_statement->requireCanRead();
  return sqlite3_column_int(m_statement->m_handle, column);
}

std::string ResultSet::readString(const int column) const {
  m_statement->requireOpen();
  m_statement->requireCanRead();
  return std::string((const char*) sqlite3_column_text(m_statement->m_handle,
                                                       column));
}

bool ResultSet::next() {
  return m_statement->step();
}

}  // namespace sqlitepp
