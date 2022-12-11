CMAKE_MINIMUM_REQUIRED(VERSION 3.24)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_COLOR_DIAGNOSTICS ON)

SET(INCLUDE_FILES
        include/fixpool.hpp)

SET(MEMPOOL_NAME mempool)



ADD_LIBRARY(${MEMPOOL_NAME}
        INTERFACE
        ${INCLUDE_FILES})

SET(COMPILER_OPTIONS "-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-reserved-identifier -Wno-poison-system-directories")

SET_SOURCE_FILES_PROPERTIES(
        ${INCLUDE_FILES}
        PROPERTIES
        COMPILE_FLAGS "${COMPILER_OPTIONS}")
