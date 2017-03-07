cmake_minimum_required(VERSION 2.8)
project(bpMqttConsoleExample)
set (Byteport_VERSION_MAJOR 1)
set (Byteport_VERSION_MINOR 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -o0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -o0")

#pkg_check_modules(<PREFIX> [REQUIRED] [QUIET] <MODULE> [<MODULE>]*)
include(FindPkgConfig)

#Input list without .pc ending
set(PKG_CONFIG_DEPS byteport)
pkg_check_modules(PKG_CONFIG_LIBS REQUIRED ${PKG_CONFIG_DEPS})

file(GLOB PROJECT_SRC_CPP *.cpp)
file(GLOB PROJECT_SRC_C *.c)

add_executable (${PROJECT_NAME} ${PROJECT_SRC_CPP} ${PROJECT_SRC_C} )

#Sets compiler/linker include/link flags ( -I... )
target_include_directories( ${PROJECT_NAME} PUBLIC ${PKG_CONFIG_LIBS_STATIC_INCLUDE_DIRS} )
target_link_libraries( ${PROJECT_NAME} ${PKG_CONFIG_LIBS_STATIC_LDFLAGS} )

install (TARGETS ${PROJECT_NAME} DESTINATION bin)
