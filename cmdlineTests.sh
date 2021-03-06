#! /bin/bash

set -e

TMP=${TMP:-/tmp}
WORKDIR="$(mktemp -d ${TMP}/cmdlineTests.XXXXXXXX)"
OUTFILE="${WORKDIR}/stdout.txt"
TESTDIR="../test"
MKLEX="./mklex"
# TESTDIR="$(realpath "$(dirname $0)/test")"
# MKLEX="$(realpath "${MKLEX:-./mklex}")"

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
  if $MKLEX --invalid &>${OUTFILE}; then
    fail "Invalid argument test failed"
  fi
  grep -q "Unknown Option" ${OUTFILE} || fail
}

test_help() {
  einfo "test_help"

  $MKLEX --help &>${OUTFILE}
  grep -q "output-table" ${OUTFILE} || fail

  $MKLEX -h &>${OUTFILE}
  grep -q "output-table" ${OUTFILE} || fail
}

test_cxx_without_namespaces() {
  einfo "test_cxx_without_namespaces"
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="${WORKDIR}/table.cc" \
         --output-token="${WORKDIR}/token.h" \
         --table-name="lexerDef" \
         --token-name="Token"
}

test_cxx_with_namespaces() {
  einfo "test_cxx_with_namespaces"
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="${WORKDIR}/table.cc" \
         --output-token="${WORKDIR}/token.h" \
         --table-name="myns::lexerDef" \
         --token-name="myns::Token"
}

test_cxx_output_stderr() {
  einfo "test_cxx_with_namespaces"
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table=- \
         --output-token=- \
         --table-name="lexerDef" \
         --token-name="Token" \
         2>"${WORKDIR}/output.inc"

  test -f "${WORKDIR}/output.inc"
}

test_debug_nfa() {
  einfo "test_debug_nfa"
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="${WORKDIR}/table.cc" \
         --output-token="${WORKDIR}/token.h" \
         --table-name="myns::lexerDef" \
         --token-name="myns::Token" \
         --debug-nfa > "${WORKDIR}/nfa.dot"
  test -f "${WORKDIR}/nfa.dot"
}

test_debug_nfa_multi() {
  einfo "test_debug_nfa_multi"
  $MKLEX -f "${TESTDIR}/multiple_conditions.klex" \
         --output-table="${WORKDIR}/table.cc" \
         --output-token="${WORKDIR}/token.h" \
         --table-name="lexerDef" \
         --token-name="Token" \
         --debug-nfa > "${WORKDIR}/nfa.dot"
  test -f "${WORKDIR}/nfa.dot"
}

test_debug_dfa() {
  einfo "test_debug_dfa"
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="${WORKDIR}/table.cc" \
         --output-token="${WORKDIR}/token.h" \
         --table-name="myns::lexerDef" \
         --token-name="myns::Token" \
         --debug-dfa="${WORKDIR}/dfa.dot"
  test -f "${WORKDIR}/dfa.dot"
}

test_debug_dfa_stdout() {
  einfo "test_debug_dfa_stdout"
  $MKLEX -f "${TESTDIR}/good.klex" \
         --output-table="${WORKDIR}/table.cc" \
         --output-token="${WORKDIR}/token.h" \
         --table-name="myns::lexerDef" \
         --token-name="myns::Token" \
         --debug-dfa=- >"${WORKDIR}/dfa.dot"
  test -f "${WORKDIR}/dfa.dot"
}

test_overshadowed() {
  einfo "test_overshadowed"
  $MKLEX -f "${TESTDIR}/overshadowed.klex" \
         --output-table="${WORKDIR}/table.cc" \
         --output-token="${WORKDIR}/token.h" \
         --table-name="lexerDef" \
         --token-name="Token" \
         &>${OUTFILE} && fail "Failure expected."
  grep -q "Rule If cannot be matched as rule" ${OUTFILE} || fail "missing error string"
}

main() {
  einfo "WORKDIR: ${WORKDIR}"
  einfo "TESTDIR: ${TESTDIR}"
  einfo "mklex: ${MKLEX}"

  trap cleanup INT TERM

  test_invalid_arguments
  test_help
  test_cxx_without_namespaces
  test_cxx_with_namespaces
  test_cxx_output_stderr
  test_debug_nfa
  test_debug_nfa_multi
  test_debug_dfa
  test_debug_dfa_stdout
  test_overshadowed
}

main
