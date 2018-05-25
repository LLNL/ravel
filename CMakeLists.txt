project(Ravel C CXX)

cmake_minimum_required(VERSION 2.8.9) # for Qt5

set(Ravel_MAJOR_VERSION 1)
set(Ravel_MINOR_VERSION 0)
set(Ravel_PATCH_VERSION 0)
set(Ravel_VERSION
    "${Ravel_MAJOR_VERSION}.${Ravel_MINOR_VERSION}.${Ravel_PATCH_VERSION}")

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake-Modules
    ${CMAKE_MODULE_PATH})

add_subdirectory(src)
