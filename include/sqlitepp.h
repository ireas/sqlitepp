/*
 * (C) 2014 Robin Krahl
 * MIT license -- http://opensource.org/licenses/MIT
 */

#ifndef __SQLITEPP_H
#define __SQLITEPP_H

#include <memory>
#include <string>

#include <boost/noncopyable.hpp>
#include <sqlite3.h>

namespace sqlitepp {
    class Openable {
    public:
        const bool isOpen() const;

    protected:
        Openable(const bool open, const std::string & name);

        void requireOpen() const;
        void setOpen(const bool open);

    private:
        bool m_open;
        const std::string & m_name;
    };

    class DatabaseError : public std::runtime_error {
    public:
        DatabaseError(const int errorCode);
        DatabaseError(const int errorCode, const std::string & errorMessage);

        const int errorCode() const;
    private:
        const int m_errorCode;

        static const std::string getErrorMessage(const int errorCode, const std::string & errorMessage);
    };

    class Statement;

    class Database : private boost::noncopyable, public Openable {
        friend class Statement;
    public:
        Database();
        Database(const std::string & file);
        ~Database();

        void close();
        void execute(const std::string & sql);
        void open(const std::string & file);
        std::shared_ptr<Statement> prepare(const std::string & sql);

    private:
        sqlite3 * m_handle;
    };

    class Statement : private boost::noncopyable, public Openable {
    public:
        Statement(Database & database, const std::string & statement);
        ~Statement();

        void bindDouble(const int index, const double value);
        void bindDouble(const std::string & name, const double value);
        void bindInt(const int index, const int value);
        void bindInt(const std::string & name, const int value);
        void bindString(const int index, const std::string & value);
        void bindString(const std::string & name, const std::string & value);
        const bool canRead() const;
        const int columnCount() const;
        const double readDouble(const int column) const;
        const int readInt(const int column) const;
        const std::string readString(const int column) const;
        const bool step();
        void finalize();
        const bool reset();

    private:
        sqlite3_stmt * m_handle;
        bool m_canRead;

        int getParameterIndex(const std::string & name) const;
        void handleBindResult(const int index, const int result) const;
        void requireCanRead() const;
    };
}

#endif