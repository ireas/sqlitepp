sqlitepp
========

C++ binding for the SQLite library

Dependencies
------------

 - compile dependencies
   - CMake 3.0 (or later)
   - libgtest
 - runtime dependencies
   - libsqlite3

Example
-------

**test.cpp**

```C++
#include <iostream>
#include <memory>
#include <sqlitepp/sqlitepp.h>

int main(int argc, char** argv) {
  sqlitepp::Database database("/path/to/database.sqlite");
  database.execute("CREATE TABLE test (id, value);");
  std::shared_ptr<sqlitepp::Statement> statement = database.prepare(
      "INSERT INTO test (id, value) VALUES (:id, :value);");
  statement->bind(":id", 1);
  statement->bind(":value", "test value");
  statement->execute();
  statement = database.prepare("SELECT id, value FROM test;");
  ResultSet resultSet = statement->execute();
  while (resultSet.canRead()) {
    std::cout << "ID: " << resultSet.readInt(0) << "\t value: "
        << resultSet.readString(1) << std::endl;
    resultSet.next();
  }
}
```

```
$ g++ --std=c++11 -o test -lsqlitepp -lsqlite3 test.cpp
$ ./test
ID: 1  value: test value
```

For more information, see the [API documentation][api].

[api]: http://robinkrahl.github.io/sqlitepp/
