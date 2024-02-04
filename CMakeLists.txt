cmake_minimum_required(VERSION 3.22)
project(StockMarketGame)

set(JsonCpp_INCLUDE_DIR "/usr/include/jsoncpp")
set(JsonCpp_LIBRARIES "/usr/lib/x86_64-linux-gnu/libjsoncpp.so")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(CURL REQUIRED)
find_package(DPP REQUIRED)

include_directories(include ${JsonCpp_INCLUDE_DIR})

add_executable(StockMarketGame
  src/main.cpp
  src/stockRetriever.cpp
)

target_include_directories(StockMarketGame PRIVATE
  /usr/include/x86_64-linux-gnu/curl/
  ${DPP_INCLUDE_DIR}
)

target_link_libraries(StockMarketGame PRIVATE
  CURL::libcurl
  ${JsonCpp_LIBRARIES}
  ${DPP_LIBRARIES}
)

set_target_properties(StockMarketGame PROPERTIES 
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
