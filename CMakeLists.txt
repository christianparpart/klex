cmake_minimum_required(VERSION 3.6)
project(klex VERSION "0.0.0" LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(mklex)

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

option(KLEX_EMBEDDED_FMTLIB "Build against embedded fmtlib [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})
option(KLEX_EXAMPLES "Build examples [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})
option(KLEX_TESTS "Build test suite [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})
option(MKLEX_LINK_STATIC "Build mklex as staticaly linked binary [default: OFF]" OFF)

# fmtlib
if(KLEX_EMBEDDED_FMTLIB)
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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/klex/sysconfig.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/src/klex/sysconfig.h)

# ----------------------------------------------------------------------------
add_library(klex STATIC
    src/klex/Alphabet.cc
    src/klex/Compiler.cc
    src/klex/DFA.cc
    src/klex/DFABuilder.cc
    src/klex/DFAMinimizer.cc
    src/klex/DotWriter.cc
    src/klex/NFA.cc
    src/klex/NFABuilder.cc
    src/klex/RegExpr.cc
    src/klex/RegExprParser.cc
    src/klex/RuleParser.cc
    src/klex/State.cc
    src/klex/Symbols.cc
    src/klex/util/Flags.cc
    )

target_link_libraries(klex PUBLIC fmt::fmt-header-only stdc++fs)
set_target_properties(klex PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(CLANG_TIDY_EXE)
  set_target_properties(klex PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
endif()

install(TARGETS klex DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/klex/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/include/klex"
    FILES_MATCHING PATTERN "*.h")

target_include_directories(klex PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:include>)

# ----------------------------------------------------------------------------
add_executable(mklex src/mklex.cc)
set_target_properties(mklex PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(mklex klex)

if(MKLEX_LINK_STATIC AND UNIX AND NOT APPLE)
  set_target_properties(mklex PROPERTIES
                        LINK_FLAGS -static
                        LINK_SEARCH_START_STATIC ON
                        LINK_SEARCH_END_STATIC ON)
endif()

# ----------------------------------------------------------------------------
if(KLEX_TESTS)
  add_executable(klex_test
      src/klex/DFABuilder_test.cc
      src/klex/NFA_test.cc
      src/klex/RegExprParser_test.cc
      src/klex/RuleParser_test.cc
      src/klex/Symbols_test.cc
      src/klex/klex_test.cc
      src/klex/util/AnsiColor.cc
      src/klex/util/testing.cc
      )

  target_link_libraries(klex_test PUBLIC klex)
  target_link_libraries(klex_test PUBLIC fmt::fmt-header-only stdc++fs)
  set_target_properties(klex_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  if(CLANG_TIDY_EXE)
    set_target_properties(klex_test PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
  endif()
endif(KLEX_TESTS)

# ----------------------------------------------------------------------------
if(KLEX_EXAMPLES)
  set(FLOW_TOKEN_SRC "${CMAKE_CURRENT_BINARY_DIR}/examples/token.h")
  klex_generate_cpp(examples/flow.klex ${FLOW_TOKEN_SRC} FLOW_TABLE_SRC)
  message(STATUS "example: KLEX_TABLE_SRC: ${FLOW_TABLE_SRC}")

  add_executable(example_flowlexer examples/flowlexer.cc ${FLOW_TABLE_SRC})
  set_target_properties(example_flowlexer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_include_directories(example_flowlexer PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/examples)
  target_link_libraries(example_flowlexer PUBLIC fmt::fmt-header-only stdc++fs)
  set_target_properties(example_flowlexer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

  add_executable(example_mathexpr examples/mathexpr.cc)
  set_target_properties(example_mathexpr PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(example_mathexpr klex)
  target_include_directories(example_mathexpr PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/examples)
endif(KLEX_EXAMPLES)
