// Copyright (C) 2014--2015 Robin Krahl <robin.krahl@ireas.org>
// MIT license -- http://opensource.org/licenses/MIT

#ifndef SQLITEPP_SQLITEPP_H_
#define SQLITEPP_SQLITEPP_H_

#include <sqlite3.h>
#include <stdexcept>
#include <memory>
#include <string>

/// \file
/// \brief Defines all classes of the sqlitepp library in the namespace
///        sqlitepp.

/// \mainpage sqlitepp -- C++ wrapper for SQLite3
/// **sqlitepp** is a C++ wrapper for the official SQLite3 C API.
///
/// \section compile Compiling sqlitepp
/// sqlitepp uses CMake as a build tool. To build sqlitepp from source,
/// download the source from GitHub and then execute these commands:
/// \code
/// $ mkdir bin && cd bin
/// $ cmake .. && make
/// \endcode
///
/// \section using Using sqlitepp
///
/// \subsection connect Connecting to a database
/// To connect to a SQLite database, you just have to create a new
/// sqlitepp::Database object.
/// \code{.cpp}
/// sqlitepp::Database database("/path/to/database.sqlite");
/// \endcode
/// This snippet is equivalent to:
/// \code{.cpp}
/// sqlitepp::Database database;
/// database.open("/path/to/database.sqlite");
/// \endcode
/// If the database file does not already exist, it is created. If an error
/// occurs during the creation of the database, a sqlitepp::DatabaseError
/// is thrown.
///
/// \subsection execute Executing a simple statement
/// To execute a simple statement, use sqlitepp::Database::execute:
/// \code{.cpp}
/// sqlitepp::Database database("/path/to/database.sqlite");
/// database.execute("CREATE TABLE test (id, value);");
/// \endcode
///
/// \subsection prepare Executing complex statements
/// If you want to execute more complex statements, for example selection or
/// insertion, use prepared statements. You can prepare a statement using
/// sqlitepp::Database::prepare. You can then bind values (if necessary) using
/// the `bind` methods of sqlitepp::Statement and execute the statement using
/// sqlitepp::Statement::execute. `execute` returns a sqlitepp::ResultSet that
/// stores the returned values (if any).
///
/// \subsubsection insert Example 1: insert
/// The recommended way to handle insertions are named bindings:
/// \code{.cpp}
/// sqlitepp::Database database("/path/to/database.sqlite");
/// std::shared_ptr<sqlitepp::Statement> statement = database.prepare(
///     "INSERT INTO test (id, value) VALUES (:id, :value);");
///
/// // insert (1, "test value")
/// statement->bind(":id", 1);
/// statement->bind(":value", "test value");
/// statement->execute();
/// statement->reset();
///
/// // insert (2, "other value")
/// statement->bind(":id", 2);
/// statement->bind(":value", "other value");
/// statement->execute();
/// \endcode
///
/// \subsubsection select Example 2: select
/// \code{.cpp}
/// sqlitepp::Database database("/path/to/database.sqlite");
/// std::shared_ptr<sqlitepp::Statement> statement = database.prepare(
///     "SELECT id, value FROM test;");
/// ResultSet resultSet = statement.execute();
/// while (resultSet.canRead()) {
///   std::cout << "ID: " << resultSet.readInt(0) << "\tvalue: "
///       << resultSet.readString(1) << std::endl;
///   resultSet.next();
/// }
/// \endcode
///
/// \section concepts Concepts
/// \subsection error Error handling
/// If an error occurs during an operation, an exception is thrown. All
/// SQLite3 database errors are wrapped in sqlitepp::DatabaseError. If a
/// method returns, it was successful (if not stated otherwise in the method
/// documentation).
///
/// \subsection resources Resources
/// sqlitepp uses RAII. This means that the destructors of sqlitepp::Database
/// and sqlitepp::Statement take care of freeing their resources once they
/// are destroyed. You can force them to free their resources using the
/// `close` methods.

/// \brief Contains all classes of the sqlitepp library.
namespace sqlitepp {

/// \brief A class that forbids copying and assignments for all subclasses.
///
/// This class defines a private, unimplemented copy constructor and assignment
/// method so that copies and assignments fail at compile-time. This class is
/// inspired by Scott Meyers, <em>Effective C++</em>, 3rd Edition, Item 6.
class Uncopyable {
 protected:
  Uncopyable() {}
  ~Uncopyable() {}

 private:
  Uncopyable(const Uncopyable&);
  Uncopyable& operator=(const Uncopyable&);
};

/// \brief An element that has the two states *open* and *closed*.
///
/// Subclasses of this class may define methods that require the object to be
/// in a specific state. Refer to the implementing class&rsquo;s documentation
/// for more information about the methods that require a specific state.
///
/// The default state depends on the implementation. You can check the state of
/// an object using the isOpen() method.
///
/// Implementing classes may use setOpen() to change the state and
/// requireOpen() to throw a std::logic_error if the object is currently not
/// open.
class Openable {
 public:
  /// \brief Checks whether this object is open.
  ///
  /// \returns `true` if this object is open; `false` if it is closed
  bool isOpen() const;

 protected:
  /// \brief Creates a new Openable.
  ///
  /// \param open `true` if the objet should be open per default; `false` if
  ///        it shoukd be closed per defaut
  /// \param name the name of the implementing class used in error messages
  Openable(const bool open, const std::string& name);

  /// \brief Requires this object to be open and throws an exception if it is
  ///        not.
  ///
  /// This method should be used at the beginning of other subclass methods
  /// that require this object to be open. The error message of the exception
  /// will contain the class name passed to the constructor.
  ///
  /// \throws std::logic_error if this object is not open
  void requireOpen() const;

  /// \brief Changes the state of this object.
  ///
  /// \param open the new state of this object (`true` if it should be opened;
  ///        `false` if it should be closed)
  void setOpen(const bool open);

 private:
  bool m_open;
  const std::string& m_name;
};

/// \brief An error that occurred during a database operation.
///
/// This error class is only used for errors that occured in the SQLite3
/// library and that are related to database operations. If there are other
/// problems, for example wrong states or illegal arguments, appropriate other
/// exceptions are thrown.
///
/// This exception class stores the SQLite3 error code and the error message.
///
/// \sa [SQLite Result Codes](https://www.sqlite.org/c3ref/c_abort.html)
class DatabaseError : public std::runtime_error {
 public:
  /// \brief Creates a new DatabaseError with the given code and the default
  ///        message.
  ///
  /// The message is retrieved from the default SQLite3 error messages.
  ///
  /// \param errorCode the SQLite3 error code
  /// \sa [SQLite Result Codes](https://www.sqlite.org/c3ref/c_abort.html)
  explicit DatabaseError(const int errorCode);

  /// \brief Creates a new DatabaseError with the given code and message.
  ///
  /// \param errorCode the SQLite3 error code
  /// \param errorMessage the according error message
  /// \sa [SQLite Result Codes](https://www.sqlite.org/c3ref/c_abort.html)
  DatabaseError(const int errorCode, const std::string& errorMessage);

  /// \brief Returns the SQLite3 error code for this error.
  ///
  /// \sa [SQLite Result Codes](https://www.sqlite.org/c3ref/c_abort.html)
  int errorCode() const;

 private:
  const int m_errorCode;

  static std::string getErrorMessage(const int errorCode,
                                     const std::string& errorMessage);
};

class Database;
class ResultSet;

/// \brief A handle for a SQLite3 statement.
///
/// This class stores a reference to a prepared SQLite3 statement and provides
/// methods to bind parameters to the query, execute it and read the results.
/// If a database operation fails, a DatabaseError is thrown.
///
/// Use Database::prepare to obtain instances of this class.
class Statement : private Uncopyable, public Openable {
 public:
  /// \brief Deconstructs this object and finalizes the statement.
  ///
  /// Errors that occur when the statement is finalized are ignored as they
  /// already occured during the last operation.
  ~Statement();

  /// \brief Binds the given double value to the column with the given index.
  ///
  /// \param index the index of the column to bind the value to
  /// \param value the value to bind to that column
  /// \throws std::logic_error if the statement is not open
  /// \throws std::out_of_range if the given index is out of range
  /// \throws std::runtime_error if there is not enough memory to bind the
  ///         value
  /// \throws DatabaseError if an database error occured during the binding
  void bind(const int index, const double value);

  /// \brief Binds the given double value to the column with the given name.
  ///
  /// \param index the name of the column to bind the value to
  /// \param value the value to bind to that column
  /// \throws std::logic_error if the statement is not open
  /// \throws std::invalid_argument if there is no column witht the given name
  /// \throws std::runtime_error if there is not enough memory to bind the
  ///         value
  /// \throws DatabaseError if an database error occured during the binding
  void bind(const std::string& name, const double value);

  /// \brief Binds the given integer value to the column with the given index.
  ///
  /// \param index the index of the column to bind the value to
  /// \param value the value to bind to that column
  /// \throws std::logic_error if the statement is not open
  /// \throws std::out_of_range if the given index is out of range
  /// \throws std::runtime_error if there is not enough memory to bind the
  ///         value
  /// \throws DatabaseError if an database error occured during the binding
  void bind(const int index, const int value);

  /// \brief Binds the given integer value to the column with the given name.
  ///
  /// \param index the name of the column to bind the value to
  /// \param value the value to bind to that column
  /// \throws std::logic_error if the statement is not open
  /// \throws std::invalid_argument if there is no column witht the given name
  /// \throws std::runtime_error if there is not enough memory to bind the
  ///         value
  /// \throws DatabaseError if an database error occured during the binding
  void bind(const std::string& name, const int value);

  /// \brief Binds the given string value to the column with the given index.
  ///
  /// \param index the index of the column to bind the value to
  /// \param value the value to bind to that column
  /// \throws std::logic_error if the statement is not open
  /// \throws std::out_of_range if the given index is out of range
  /// \throws std::runtime_error if there is not enough memory to bind the
  ///         value
  /// \throws DatabaseError if an database error occured during the binding
  void bind(const int index, const std::string& value);

  /// \brief Binds the given string value to the column with the given name.
  ///
  /// \param index the name of the column to bind the value to
  /// \param value the value to bind to that column
  /// \throws std::logic_error if the statement is not open
  /// \throws std::invalid_argument if there is no column witht the given name
  /// \throws std::runtime_error if there is not enough memory to bind the
  ///         value
  /// \throws DatabaseError if an database error occured during the binding
  void bind(const std::string& name, const std::string& value);

  /// \brief Closes this statement.
  ///
  /// Once you closed this statement, you may no longer access it. Any errors
  /// that occur during finalization are ignored as they already occurred
  /// during the last operation.
  void close();

  /// \brief Executes this statement and returns the result (if any).
  ///
  /// \returns the result returned from the query (empty if there was no result)
  /// \throws std::logic_error if the statement is not open
  /// \throws DatabaseError if a database error occurs during the query
  ///         execution
  ResultSet execute();

  /// \brief Resets the statement.
  ///
  /// Resets the statement so that it can be re-executed. Bindings are not
  /// resetted.
  ///
  /// \returns `true` if the reset was successful; otherwise `false`
  /// \throws std::logic_error if the statement is not open
  bool reset();

 private:
  explicit Statement(sqlite3_stmt* handle);

  int getParameterIndex(const std::string& name) const;
  void handleBindResult(const int index, const int result) const;
  void requireCanRead() const;
  void setInstancePointer(const std::weak_ptr<Statement>& instancePointer);
  bool step();

  sqlite3_stmt* m_handle;
  bool m_canRead;
  std::weak_ptr<Statement> m_instancePointer;

  friend class Database;
  friend class ResultSet;
};

/// \brief A handle for a SQLite3 database.
///
/// This class stores a reference to a SQLite3 database and provides methods
/// to open, query and change this database. After you successfully opened a
/// database (using the constructor Database(const std::string&) or using the
/// open(const std::string&)  method), you can execute and prepare statements.
/// You can check whether the database is currently open using isOpen().
///
/// If you try to call a method that queries or updates the database and the
/// database is not open, a std::logic_error is thrown. If a database operation
/// fails, a DatabaseError is thrown.
class Database : private Uncopyable, public Openable {
 public:
  /// \brief Creates a new closed database.
  ///
  /// Before you can access this database, you have to open a database file
  /// using open(std::string&).
  Database();

  /// \brief Creates a new database and opens the given file.
  ///
  /// The given file must either be a valid SQLite3 database file or may not
  /// exist yet. This constructor is an abbreviation for:
  /// \code{.cpp}
  /// Database database;
  /// database.open(file);
  /// \endcode
  ///
  /// \param file the name of the database file (not required to exist)
  /// \throws std::runtime_error if there is not enough memory to create a
  ///         database connection
  /// \throws DatabaseError if the SQLite3 database could not be opened
  explicit Database(const std::string& file);

  /// \brief Destructs this object and closes the database connection.
  ///
  /// Errors that occur closing the database are ignored.
  ~Database();

  /// \brief Closes the database if it is open.
  ///
  /// \throws DatabaseError if the database cannot be closed
  void close();

  /// \brief Executes the given SQL string.
  ///
  /// You can only call this method if there is an open database connection.
  /// If you want to access the values returned by a SQL statement, use
  /// prepare(const std::string&) instead.
  ///
  /// \param sql the SQL statement to execute
  /// \throws std::logic_error if the database is not open
  /// \throws DatabaseError if an error occurred during the execution
  void execute(const std::string& sql);

  /// \brief Opens the given database file.
  ///
  /// The given file must either be a valid SQLite3 database file or may not
  /// exist yet. You can only open a new connection when the previous
  /// connection has been closed (if any).
  ///
  /// \param file the name of the database file (not required to exist)
  /// \throws std::logic_error if the database is already open
  /// \throws std::runtime_error if there is not enough memory to create a
  ///         database connection
  /// \throws DatabaseError if the SQLite3 database could not be opened
  void open(const std::string& file);

  /// \brief Prepares a statement and returns a pointer to it.
  ///
  /// You can either pass a complete SQL statement or a statement with
  /// wildcards. If you use wildcards, you can bind them to a value using the
  /// returned Statement.
  ///
  /// \param sql the SQL statement to prepare (may contain wildcards)
  /// \returns a pointer to the prepared statement
  /// \throws std::logic_error if the database is not open
  /// \throws DatabaseError if an error occurred during the preparation
  std::shared_ptr<Statement> prepare(const std::string& sql);

 private:
  sqlite3* m_handle;
};

/// \brief A result set returned from a SQL query.
///
/// As long as there is data (`canRead()`), you can read it using the
/// `read*Type*` methods. To advance to the next row, use `next()`.
class ResultSet {
 public:
  /// \brief Checks whether there is data to read.
  ///
  /// \returns `true` if there is data to read; otherwise `false`
  bool canRead() const;

  /// \brief Returns the column count of the result data.
  ///
  /// You may only call this method when there is data to read (canRead()).
  ///
  /// \returns the column count of the result
  /// \throws std::logic_error if the statement is not open or there is no
  ///         data to read
  int columnCount() const;

  /// \brief Steps to the next row of the result (if there is one).
  ///
  /// \returns `true` if there is new data to read or `false` if there are
  ///          no more results
  /// \throws std::logic_error if the statement is not open
  /// \throws DatabaseError if a database error occurs during the query
  ///         execution
  bool next();

  /// \brief Returns the current double value of the result column with the
  ///        given index.
  ///
  /// You may only call this metod when there is data to read (canRead()).
  ///
  /// \param column the index of the column to read from
  /// \returns the current value of the result column with the given index
  /// \throws std::logic_error if the statement is not open or there is no
  ///         data to read
  double readDouble(const int column) const;

  /// \brief Returns the current integer value of the result column with the
  ///        given index.
  ///
  /// You may only call this metod when there is data to read (canRead()).
  ///
  /// \param column the index of the column to read from
  /// \returns the current value of the result column with the given index
  /// \throws std::logic_error if the statement is not open or there is no
  ///         data to read
  int readInt(const int column) const;

  /// \brief Returns the current string value of the result column with the
  ///        given index.
  ///
  /// You may only call this metod when there is data to read (canRead()).
  ///
  /// \param column the index of the column to read from
  /// \returns the current value of the result column with the given index
  /// \throws std::logic_error if the statement is not open or there is no
  ///         data to read
  std::string readString(const int column) const;

 private:
  explicit ResultSet(const std::shared_ptr<Statement> statement);

  const std::shared_ptr<Statement> m_statement;

  friend class Statement;
};

}  // namespace sqlitepp

#endif  // SQLITEPP_SQLITEPP_H_
