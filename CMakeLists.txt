cmake_minimum_required(VERSION 3.6)
project(lexer VERSION "0.0.0" LANGUAGES CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

option(LEXER_EMBEDDED_FMTLIB "Build against embedded fmtlib [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})

# fmtlib
if(LEXER_EMBEDDED_FMTLIB)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fmt" EXCLUDE_FROM_ALL)
  add_definitions(-DFMT_USE_WINDOWS_H=0)
else()
  # master project must provide its own fmtlib
endif()

# ----------------------------------------------------------------------------
add_definitions(-Wall)
add_definitions(-pedantic)

# ----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/sysconfig.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/src/lexer/sysconfig.h)

# ----------------------------------------------------------------------------
add_library(lexer STATIC
    src/lexer/alphabet.cc
    src/lexer/fa.cc
    src/lexer/regexpr.cc
    )

target_link_libraries(lexer PUBLIC fmt::fmt-header-only)
set_target_properties(lexer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(CLANG_TIDY_EXE)
  set_target_properties(lexer PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
endif()

install(TARGETS lexer DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lexer"
    FILES_MATCHING PATTERN "*.h")

target_include_directories(lexer PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:include>)

# ----------------------------------------------------------------------------
add_executable(regexpr_test src/lexer/regexpr_test.cc)
set_target_properties(regexpr_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(regexpr_test lexer)
