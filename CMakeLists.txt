cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

project(sqlitepp)

set(SOURCES src/sqlitepp.cpp)
set(INCLUDES include)

include_directories(${INCLUDES})

add_library(sqlitepp ${SOURCES})
add_executable(sqlitepptest src/sqlitepptest.cpp)

add_custom_target(check COMMAND sqlitepptest)

set(Boost_USE_MULTITHREADED OFF)
find_package(Boost 1.54.0 REQUIRED COMPONENTS unit_test_framework)
find_package(Sqlite3 REQUIRED)

set(DEP_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIRS})
set(DEP_LIBRARIES ${SQLITE3_LIBRARIES})
set(TEST_INCLUDE_DIRS ${Boost_UNIT_TEST_FRAMEWORK_INCLUDE_DIRS})
set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} sqlitepp)

include_directories(${DEP_INCLUDE_DIRS})
include_directories(${TEST_INCLUDE_DIRS})
target_link_libraries(sqlitepp ${DEP_LIBRARIES})
target_link_libraries(sqlitepptest ${TEST_LIBRARIES})

# add_subdirectory(tests)

