cmake_minimum_required(VERSION 3.22)
project(StockMarketTest)

set(JsonCpp_INCLUDE_DIR "/usr/include/jsoncpp")
set(JsonCpp_LIBRARIES "/usr/lib/x86_64-linux-gnu/libjsoncpp.so")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

find_package(CURL REQUIRED)
find_package(DPP REQUIRED)
find_package(SQLite3 REQUIRED)

include_directories(include ${JsonCpp_INCLUDE_DIR})

add_executable(StockMarketTest
  ../src/main.cpp
  ../src/stockRetriever.cpp
  ../src/databaseHandler.cpp
)

target_include_directories(StockMarketTest PRIVATE
  /usr/include/x86_64-linux-gnu/curl/
  ${DPP_INCLUDE_DIR}
)

target_link_libraries(StockMarketTest PRIVATE
  CURL::libcurl
  SQLite::SQLite3
  ${JsonCpp_LIBRARIES}
  ${DPP_LIBRARIES}
)

set_target_properties(StockMarketTest PROPERTIES 
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)
