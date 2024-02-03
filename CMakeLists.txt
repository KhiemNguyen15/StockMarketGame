cmake_minimum_required(VERSION 3.12)
project(StockMarketGame)

set(CMAKE_CXX_STANDARD 11)

set(JsonCpp_INCLUDE_DIR "/usr/include/jsoncpp")
set(JsonCpp_LIBRARIES "/usr/lib/x86_64-linux-gnu/libjsoncpp.so")

find_package(CURL REQUIRED)

include_directories(include ${JsonCpp_INCLUDE_DIR})

add_executable(StockMarketGame
  src/main.cpp
  src/stockRetriever.cpp
)

target_include_directories(StockMarketGame PRIVATE
  /usr/include/x86_64-linux-gnu/curl/
)

target_link_libraries(StockMarketGame PRIVATE
  CURL::libcurl
  ${JsonCpp_LIBRARIES}
)
