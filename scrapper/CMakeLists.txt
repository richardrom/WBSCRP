CMAKE_MINIMUM_REQUIRED(VERSION 3.24)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_COLOR_DIAGNOSTICS ON)

SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
SET(THREADS_PREFER_PTHREAD_FLAG TRUE)

FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Catch2 CONFIG REQUIRED)
FIND_PACKAGE(fmt CONFIG REQUIRED)
FIND_PACKAGE(cpr CONFIG REQUIRED)
FIND_PACKAGE(re2 CONFIG REQUIRED)


SET(INCLUDE_FILES
        include/tokenizer.hpp
        include/scrapper.hpp
        include/parser.hpp
        include/parser_error.hpp
        include/encoding.hpp
        include/encoding_character_reference.hpp
        include/crc64.hpp)

SET(SOURCE_FILES
        src/tokenizer.cpp
        src/scrapper.cpp
        src/parser_error.cpp
        src/encoding.cpp)

SET(LIBRARY_NAME wbscrp)


ADD_LIBRARY(${LIBRARY_NAME} STATIC
        ${INCLUDE_FILES}
        ${SOURCE_FILES})

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE include)
TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ../pool/include)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE fmt::fmt)
TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE cpr::cpr)
TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE re2::re2)

IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

    SET(WARN_COMPILER_OPTIONS "-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-reserved-identifier -Wno-poison-system-directories")

    SET(OTHER_COMPILER_OPTIONS "")

    SET(COMPILER_OPTIONS "${WARN_COMPILER_OPTIONS}  ${OTHER_COMPILER_OPTIONS}")


    SET_SOURCE_FILES_PROPERTIES(
            ${INCLUDE_FILES}
            PROPERTIES
            COMPILE_FLAGS "${COMPILER_OPTIONS}")
ENDIF ()