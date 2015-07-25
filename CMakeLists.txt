cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")
add_definitions(-std=c++11)

project(sqlitepp)

set(SOURCES src/sqlitepp/sqlitepp.cc)
set(TEST_SOURCES src/sqlitepp/sqlitepp_test.cc)
set(LINT_FILES include/sqlitepp/sqlitepp.h ${SOURCES} ${TEST_SOURCES})
set(INCLUDES include)

include(StyleCheck)

include_directories(${INCLUDES})

add_library(sqlitepp ${SOURCES})

find_package(Doxygen)
find_package(Sqlite3 REQUIRED)

set(DEP_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIRS})
set(DEP_LIBRARIES PUBLIC ${SQLITE3_LIBRARIES})

include_directories(${DEP_INCLUDE_DIRS})
target_link_libraries(sqlitepp ${DEP_LIBRARIES})

find_package(GTest)
if(GTEST_FOUND)
  enable_testing()
  set(TEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIRS})
  set(TEST_LIBRARIES ${GTEST_BOTH_LIBRARIES} pthread sqlitepp)
  add_executable(sqlitepp_test ${TEST_SOURCES})
  include_directories(${TEST_INCLUDE_DIRS})
  target_link_libraries(sqlitepp_test ${TEST_LIBRARIES})
  set(GTEST_ARGS "")
  gtest_add_tests(sqlitepp_test "${GTEST_ARGS}" ${TEST_SOURCES})
endif(GTEST_FOUND)

add_style_check_target(check "${LINT_FILES}")

if(DOXYGEN_FOUND)
  configure_file(${CMAKE_MODULE_PATH}/sqlitepp.doxyfile
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  add_custom_target(doc ${DOXYGEN_EXECUTABLE}
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
endif(DOXYGEN_FOUND)
