cmake_minimum_required(VERSION 3.20)

project(download VERSION 1.0.0 DESCRIPTION "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)

file(GLOB LUA_SOURCES "include/lua/*.c")
file(GLOB CXX_SOURCES "src/*.c*" "src/lua/*.c*")
message(STATUS "C++ Sources: ${CXX_SOURCES}")
message(STATUS "Lua Sources: ${LUA_SOURCES}")

add_executable(download ${CXX_SOURCES} ${LUA_SOURCES})

set(EXTERNAL_INSTALL_DIR ${CMAKE_BINARY_DIR}/external)

include(ExternalProject)

ExternalProject_Add(
    libcurl
    PREFIX ${EXTERNAL_INSTALL_DIR}
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-8_0_1
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_DIR}
               -DBUILD_SHARED_LIBS=ON
               -DCURL_STATICLIB=OFF
               -DBUILD_CURL_EXE=OFF
)
ExternalProject_Get_Property(libcurl install_dir)
set(LIBCURL_INCLUDE_DIR ${install_dir}/include)
set(LIBCURL_LIBRARY ${install_dir}/lib/libcurl.so)

ExternalProject_Add(
    jsoncpp
    PREFIX ${EXTERNAL_INSTALL_DIR}
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_DIR}
               -DBUILD_SHARED_LIBS=OFF
               -DJSONCPP_BUILD_STATIC_LIB=ON
)
ExternalProject_Get_Property(jsoncpp install_dir)
set(JSONCPP_INCLUDE_DIR ${install_dir}/include)
set(JSONCPP_LIBRARY ${install_dir}/lib/libjsoncpp.a)

target_include_directories(download PRIVATE 
    ${LIBCURL_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIRS}
)

target_link_libraries(download PRIVATE 
    ${LIBCURL_LIBRARY}
    ${JSONCPP_LIBRARY}
    ${LIBXML2_LIBRARIES}
    lua
)

add_dependencies(download libcurl jsoncpp)

set(CMAKE_VERBOSE_MAKEFILE ON)

get_filename_component(HOME_DIR "$ENV{HOME}" REALPATH)

set(CONFIG_DIR "${HOME_DIR}/.config/ani-downloader")
set(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lua")
file(MAKE_DIRECTORY ${CONFIG_DIR})
file(MAKE_DIRECTORY ${CONFIG_DIR}/configs)
file(COPY ${LUA_DIR}/cores ${LUA_DIR}/modules DESTINATION ${CONFIG_DIR})

