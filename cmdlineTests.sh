#! /bin/bash

set -e

TMP=${TMP:-/tmp}
MKLEX="$(realpath "${MKLEX:-./mklex}")"
WORKDIR="$(mktemp -d ${TMP}/cmdlineTests.XXXXXXXX)"
TESTDIR="$(realpath "$(dirname $0)/test")"

cleanup() {
  rm -rf ${WORKDIR}
}

einfo() {
  echo "*** ${*}"
}

fail() {
  # echo 1>&2 "Fail. ${*}"
  echo "Fail. ${*}"
  exit 1
}

test_invalid_arguments() {
  einfo "test_invalid_arguments"
  if $MKLEX --invalid &>output; then
    fail "Invalid argument test failed"
  fi
  grep -q "Unknown Option" output || fail
}

test_help() {
  einfo "test_help"

  $MKLEX --help &>output
  grep -q "output-table" output || fail

  $MKLEX -h &>output
  grep -q "output-table" output || fail
}

test_cxx_without_namespaces() {
  einfo "test_cxx_without_namespaces"
  rm -f *
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="table.cc" \
         --output-token="token.h" \
         --table-name="lexerDef" \
         --token-name="Token"
}

test_cxx_with_namespaces() {
  einfo "test_cxx_with_namespaces"
  rm -f *
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="table.cc" \
         --output-token="token.h" \
         --table-name="myns::lexerDef" \
         --token-name="myns::Token"
}

test_debug_nfa() {
  einfo "test_debug_nfa"
  rm -f *
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="table.cc" \
         --output-token="token.h" \
         --table-name="myns::lexerDef" \
         --token-name="myns::Token" \
         --debug-nfa > nfa.dot
  test -f nfa.dot
}

test_debug_dfa() {
  einfo "test_debug_dfa"
  rm -f *
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="table.cc" \
         --output-token="token.h" \
         --table-name="myns::lexerDef" \
         --token-name="myns::Token" \
         --debug-dfa=dfa.dot
  test -f dfa.dot
}

main() {
  einfo "WORKDIR: ${WORKDIR}"
  einfo "TESTDIR: ${TESTDIR}"
  einfo "mklex: ${MKLEX}"

  trap cleanup INT TERM

  cd $WORKDIR

  test_invalid_arguments
  test_help
  test_cxx_without_namespaces
  test_cxx_with_namespaces
  test_debug_nfa
  test_debug_dfa
}

main
