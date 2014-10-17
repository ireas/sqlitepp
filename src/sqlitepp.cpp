/*
 * (C) 2014 Robin Krahl
 * MIT license -- http://opensource.org/licenses/MIT
 */

#include "sqlitepp.h"

#include <exception>
#include <iostream>
#include <sstream>

sqlitepp::Openable::Openable(const bool open, const std::string & name) :
        m_open(open), m_name(name) {
}

const bool sqlitepp::Openable::isOpen() const {
    return m_open;
}

void sqlitepp::Openable::requireOpen() const {
    if (!m_open) {
        throw std::logic_error(m_name + " is not open.");
    }
}

void sqlitepp::Openable::setOpen(const bool open) {
    m_open = open;
}

const std::string sqlitepp::DatabaseError::getErrorMessage(const int errorCode, const std::string & errorMessage) {
    std::ostringstream stringStream;
    stringStream << "Caught SQLite3 error " << errorCode << " meaning: " << errorMessage;
    return stringStream.str();
}

sqlitepp::DatabaseError::DatabaseError(const int errorCode) :
        std::runtime_error(getErrorMessage(errorCode, sqlite3_errstr(errorCode))), m_errorCode(errorCode) {
}

sqlitepp::DatabaseError::DatabaseError(const int errorCode, const std::string & errorMessage) :
        std::runtime_error(getErrorMessage(errorCode, errorMessage)), m_errorCode(errorCode)
{
}

const int sqlitepp::DatabaseError::errorCode() const {
    return m_errorCode;
}

sqlitepp::Statement::Statement(sqlitepp::Database & database, const std::string & statement) :
        Openable(true, "Statement"), m_canRead(false) {
    database.requireOpen();
    int result = sqlite3_prepare_v2(database.m_handle, statement.c_str(), -1, &m_handle, NULL);
    if (result != SQLITE_OK) {
        throw DatabaseError(result, sqlite3_errmsg(database.m_handle));
    }
    if (m_handle == NULL) {
        throw std::runtime_error("Statement handle is NULL");
    }
}

sqlitepp::Statement::~Statement() {
    if (isOpen()) {
        // errors that could occur during finalizing are ignored as they have
        // already been handled!
        sqlite3_finalize(m_handle);
        setOpen(false);
    }
}

void sqlitepp::Statement::bindDouble(const int index, const double value) {
    requireOpen();
    handleBindResult(index, sqlite3_bind_double(m_handle, index, value));
}

void sqlitepp::Statement::bindDouble(const std::string & name, const double value) {
    bindDouble(getParameterIndex(name), value);
}

void sqlitepp::Statement::bindInt(const int index, const int value) {
    requireOpen();
    handleBindResult(index, sqlite3_bind_int(m_handle, index, value));
}

void sqlitepp::Statement::bindInt(const std::string & name, const int value) {
    bindInt(getParameterIndex(name), value);
}

void sqlitepp::Statement::bindString(const int index, const std::string & value) {
    requireOpen();
    handleBindResult(index, sqlite3_bind_text(m_handle, index, value.c_str(), -1, NULL));
}

void sqlitepp::Statement::bindString(const std::string & name, const std::string & value) {
    bindString(getParameterIndex(name), value);
}

const bool sqlitepp::Statement::canRead() const {
    return m_canRead;
}

const int sqlitepp::Statement::columnCount() const {
    requireOpen();
    requireCanRead();
    return sqlite3_column_count(m_handle);
}

const double sqlitepp::Statement::readDouble(const int column) const {
    requireOpen();
    requireCanRead();
    return sqlite3_column_double(m_handle, column);
}

const int sqlitepp::Statement::readInt(const int column) const {
    requireOpen();
    requireCanRead();
    return sqlite3_column_int(m_handle, column);
}

const std::string sqlitepp::Statement::readString(const int column) const {
    requireOpen();
    requireCanRead();
    return std::string((const char *) sqlite3_column_text(m_handle, column));
}

void sqlitepp::Statement::requireCanRead() const {
    if (!m_canRead) {
        throw std::logic_error("Trying to read from statement without data");
    }
}

const bool sqlitepp::Statement::step() {
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

void sqlitepp::Statement::finalize() {
    if (isOpen()) {
        // errors that could occur during finalizing are ignored as they have
        // already been handled!
        sqlite3_finalize(m_handle);
        setOpen(false);
    }
}

const bool sqlitepp::Statement::reset() {
    requireOpen();
    return sqlite3_reset(m_handle) == SQLITE_OK;
}

int sqlitepp::Statement::getParameterIndex(const std::string & name) const {
    requireOpen();
    int index = sqlite3_bind_parameter_index(m_handle, name.c_str());
    if (index == 0) {
        throw std::invalid_argument("No such parameter: " + name);
    }
    return index;
}

void sqlitepp::Statement::handleBindResult(const int index, const int result) const {
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

sqlitepp::Database::Database() : Openable(false, "Database") {
}

sqlitepp::Database::Database(const std::string & file) : Openable(false, "Database") {
    open(file);
}

sqlitepp::Database::~Database() {
    if (isOpen()) {
        int result = sqlite3_close(m_handle);
        if (result != SQLITE_OK) {
            std::cerr << "sqlitepp::Database::~Database(): Close failed with code "
                    << result << " meaning: " << sqlite3_errstr(result);
            std::abort();
        } else {
            setOpen(false);
        }
    }
    // m_handle is deleted by sqlite3_close
}

void sqlitepp::Database::close() {
    if (isOpen()) {
        int result = sqlite3_close(m_handle);
        if (result == SQLITE_OK) {
            setOpen(false);
        } else {
            throw sqlitepp::DatabaseError(result);
        }
    }
}

void sqlitepp::Database::execute(const std::string & sql) {
    requireOpen();
    Statement statement(*this, sql);
    statement.step();
    statement.finalize();
}

void sqlitepp::Database::open(const std::string & file) {
    if (isOpen()) {
        throw std::logic_error("sqlitepp::Database::open(std::string&): Database already open");
    }
    int result = sqlite3_open(file.c_str(), & m_handle);

    if (m_handle == NULL) {
        throw std::runtime_error("sqlitepp::Database::open(std::string&): Can't allocate memory");
    }

    if (result == SQLITE_OK) {
        setOpen(true);
    } else {
        std::string errorMessage = sqlite3_errmsg(m_handle);
        sqlite3_close(m_handle);
        throw sqlitepp::DatabaseError(result, errorMessage);
    }
}

std::shared_ptr<sqlitepp::Statement> sqlitepp::Database::prepare(const std::string & sql) {
    return std::shared_ptr<sqlitepp::Statement>(new sqlitepp::Statement(*this, sql));
}