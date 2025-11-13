cmake_minimum_required(VERSION 3.30)
project(http_servis LANGUAGES CXX)

find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json 3.12.0 REQUIRED)

add_executable(main src/main.cpp)

target_link_libraries(main PRIVATE Boost::system)

if(WIN32)
    target_link_libraries(main PRIVATE ws2_32)
endif()