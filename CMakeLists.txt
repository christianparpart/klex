cmake_minimum_required(VERSION 3.6)
project(klex VERSION "0.0.0" LANGUAGES CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

option(KLEX_EMBEDDED_FMTLIB "Build against embedded fmtlib [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})
option(KLEX_EXAMPLES "Build examples [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})

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
    src/klex/alphabet.cc
    src/klex/builder.cc
    src/klex/fa.cc
    src/klex/lexer.cc
    src/klex/regexpr.cc
    src/klex/ruleparser.cc
    src/klex/transitionmap.cc
    src/klex/util/Flags.cc
    )

target_link_libraries(klex PUBLIC fmt::fmt-header-only)
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

# ----------------------------------------------------------------------------
  add_custom_target(
      OUTPUT table.cc token.h
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/mklex
      ARGS -t table.cc -T token.h -f examples/flow.klex
      DEPENDS mklex examples/flow.klex
      COMMEND "Generating lexer table and tokens"
      VERBATIM)
  set_source_files_properties(table.cc token.h PROPERTIES GENERATED TRUE)

  add_executable(example_flowlexer examples/flowlexer.cc)
  set_target_properties(example_flowlexer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(example_flowlexer klex)
  target_include_directories(example_flowlexer PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/examples)
  add_dependencies(example_flowlexer mklex)
if(KLEX_EXAMPLES)
endif(KLEX_EXAMPLES)
