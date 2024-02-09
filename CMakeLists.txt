cmake_minimum_required(VERSION 3.22)
project(StockMarketGame)

set(JSONCPP_INCLUDE_DIRS "/usr/include/jsoncpp")
set(JSONCPP_LIBRARIES "/usr/lib/arm-linux-gnueabihf/libjsoncpp.so")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(CURL REQUIRED)
find_package(DPP REQUIRED)
find_package(SQLite3 REQUIRED)

include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${SQLite3_INCLUDE_DIRS})

include_directories(include)

add_executable(StockMarketGame
  src/main.cpp
  src/stockRetriever.cpp
  src/databaseHandler.cpp
)

target_include_directories(StockMarketGame PRIVATE
#  /usr/include/x86_64-linux-gnu/curl/
  ${DPP_INCLUDE_DIR}
)

target_link_libraries(StockMarketGame PRIVATE
#  CURL::libcurl
#  SQLite::SQLite3
  ${JSONCPP_LIBRARIES}
  ${CURL_LIBRARIES}
  ${SQLite3_LIBRARIES}
  ${DPP_LIBRARIES}
)

set_target_properties(StockMarketGame PROPERTIES 
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)
