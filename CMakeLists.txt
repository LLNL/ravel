project(Ravel C CXX)

cmake_minimum_required(VERSION 2.8.9) # for Qt5

set(RAVEL_MAJOR_VERSION 1)
set(RAVEL_MINOR_VERSION 0)
set(RAVEL_PATCH_VERSION 0)
set(RAVEL_VERSION
    "${RAVEL_MAJOR_VERSION}.${RAVEL_MINOR_VERSION}.${RAVEL_PATCH_VERSION}")

add_subdirectory(src)
