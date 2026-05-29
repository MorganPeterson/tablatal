#!/bin/sh
set -eu

BIN="./tablatal"

TESTS_RUN=0
TMPDIR=""

fail() {
  echo "FAIL: $1"
  exit 1
}

pass() {
  TESTS_RUN=$((TESTS_RUN + 1))
  printf '.'
}

assert_eq() {
  name="$1"
  expected="$2"
  actual="$3"

  if [ "$expected" != "$actual" ]; then
    echo
    echo "FAIL: $name"
    echo "expected:"
    printf '%s\n' "$expected"
    echo "actual:"
    printf '%s\n' "$actual"
    exit 1
  fi

  pass
}

assert_file_eq() {
  name="$1"
  expected="$2"
  file="$3"

  actual="$(cat "$file")"
  assert_eq "$name" "$expected" "$actual"
}

assert_success() {
  name="$1"
  shift

  if "$@" >/tmp/tablatal-test-out.$$ 2>/tmp/tablatal-test-err.$$; then
    rm -f /tmp/tablatal-test-out.$$ /tmp/tablatal-test-err.$$
    pass
    return
  fi

  echo
  echo "FAIL: $name"
  echo "command failed unexpectedly: $*"
  echo "stdout:"
  cat /tmp/tablatal-test-out.$$
  echo "stderr:"
  cat /tmp/tablatal-test-err.$$
  rm -f /tmp/tablatal-test-out.$$ /tmp/tablatal-test-err.$$
  exit 1
}

assert_failure() {
  name="$1"
  shift

  if "$@" >/tmp/tablatal-test-out.$$ 2>/tmp/tablatal-test-err.$$; then
    echo
    echo "FAIL: $name"
    echo "command succeeded unexpectedly: $*"
    echo "stdout:"
    cat /tmp/tablatal-test-out.$$
    echo "stderr:"
    cat /tmp/tablatal-test-err.$$
    rm -f /tmp/tablatal-test-out.$$ /tmp/tablatal-test-err.$$
    exit 1
  fi

  rm -f /tmp/tablatal-test-out.$$ /tmp/tablatal-test-err.$$
  pass
}

assert_output() {
  name="$1"
  expected="$2"
  shift 2

  actual="$("$@")"
  assert_eq "$name" "$expected" "$actual"
}

cleanup() {
  if [ -n "$TMPDIR" ] && [ -d "$TMPDIR" ]; then
    rm -rf "$TMPDIR"
  fi

  rm -f /tmp/tablatal-test-out.$$ /tmp/tablatal-test-err.$$
}

trap cleanup EXIT INT TERM

if [ "${SKIP_BUILD:-0}" != "1" ]; then
    make clean >/dev/null
    make >/dev/null
fi

TMPDIR="$(mktemp -d)"
cd "$TMPDIR"

###############################################################################
# version
###############################################################################

assert_output \
  "version command prints version" \
"tablatal 0.1.0" \
  "$OLDPWD/$BIN" version

assert_output \
  "--version prints version" \
"tablatal 0.1.0" \
  "$OLDPWD/$BIN" --version

assert_output \
  "-v prints version" \
"tablatal 0.1.0" \
  "$OLDPWD/$BIN" -v

###############################################################################
# init
###############################################################################

assert_success \
  "init creates table" \
  "$OLDPWD/$BIN" init people.tbl NAME:8 AGE:6 COLOR

assert_file_eq \
  "init writes expected header" \
  "NAME    AGE   COLOR" \
  people.tbl

assert_failure \
  "init refuses to overwrite existing file" \
  "$OLDPWD/$BIN" init people.tbl NAME:8 AGE:6 COLOR

assert_failure \
  "init requires non-final widths" \
  "$OLDPWD/$BIN" init bad.tbl NAME AGE:6 COLOR

assert_failure \
  "init rejects too-small width" \
  "$OLDPWD/$BIN" init bad.tbl NAME:4 AGE:6 COLOR

###############################################################################
# schema
###############################################################################

assert_output \
  "schema prints parsed fields" \
"FIELD            OFFSET   WIDTH       
NAME             0        8           
AGE              8        6           
COLOR            14       unbounded   " \
  "$OLDPWD/$BIN" schema people.tbl

###############################################################################
# add
###############################################################################

assert_success \
  "add first row" \
  "$OLDPWD/$BIN" add people.tbl NAME=Erica AGE=12 COLOR=Opal

assert_success \
  "add second row" \
  "$OLDPWD/$BIN" add people.tbl NAME=Alex AGE=23 COLOR=Cyan

assert_success \
  "add third row" \
  "$OLDPWD/$BIN" add people.tbl NAME=Nike AGE=34 COLOR=Red

assert_success \
  "add fourth row" \
  "$OLDPWD/$BIN" add people.tbl NAME=Ruca AGE=45 COLOR=Grey

assert_file_eq \
  "add writes expected rows" \
"NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Cyan
Nike    34    Red
Ruca    45    Grey" \
  people.tbl

before="$(cat people.tbl)"

assert_failure \
  "add rejects too-long fixed-width value" \
  "$OLDPWD/$BIN" add people.tbl NAME=Alexandria AGE=23 COLOR=Cyan

after="$(cat people.tbl)"
assert_eq \
  "failed add preserves file" \
  "$before" \
  "$after"

assert_failure \
  "add rejects unknown field" \
  "$OLDPWD/$BIN" add people.tbl NAME=June AGE=28 MOOD=Happy

###############################################################################
# list
###############################################################################

assert_output \
  "list prints raw table" \
"NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Cyan
Nike    34    Red
Ruca    45    Grey" \
  "$OLDPWD/$BIN" list people.tbl

assert_output \
  "list -n prints numbered table" \
"ROW  NAME    AGE   COLOR
1    Erica   12    Opal
2    Alex    23    Cyan
3    Nike    34    Red
4    Ruca    45    Grey" \
  "$OLDPWD/$BIN" list people.tbl -n

assert_output \
  "list --number prints numbered table" \
"ROW  NAME    AGE   COLOR
1    Erica   12    Opal
2    Alex    23    Cyan
3    Nike    34    Red
4    Ruca    45    Grey" \
  "$OLDPWD/$BIN" list people.tbl --number

assert_failure \
  "list rejects unknown option" \
  "$OLDPWD/$BIN" list people.tbl --bad

###############################################################################
# get
###############################################################################

assert_output \
  "get row 2" \
"NAME    AGE   COLOR
Alex    23    Cyan" \
  "$OLDPWD/$BIN" get people.tbl 2

assert_output \
  "get row 2 with -n" \
"ROW  NAME    AGE   COLOR
2    Alex    23    Cyan" \
  "$OLDPWD/$BIN" get people.tbl 2 -n

assert_output \
  "get row 2 with --number" \
"ROW  NAME    AGE   COLOR
2    Alex    23    Cyan" \
  "$OLDPWD/$BIN" get people.tbl 2 --number

assert_failure \
  "get rejects unknown option" \
  "$OLDPWD/$BIN" get people.tbl 2 --bad

assert_failure \
  "get rejects row 0" \
  "$OLDPWD/$BIN" get people.tbl 0

assert_failure \
  "get rejects missing row" \
  "$OLDPWD/$BIN" get people.tbl 99

###############################################################################
# find
###############################################################################

assert_output \
  "find by final field" \
"NAME    AGE   COLOR
Alex    23    Cyan" \
  "$OLDPWD/$BIN" find people.tbl COLOR=Cyan

assert_output \
  "find by fixed-width field" \
"NAME    AGE   COLOR
Nike    34    Red" \
  "$OLDPWD/$BIN" find people.tbl AGE=34

assert_output \
  "find -n by final field" \
"ROW  NAME    AGE   COLOR
2    Alex    23    Cyan" \
  "$OLDPWD/$BIN" find people.tbl COLOR=Cyan -n

assert_output \
  "find --number by fixed-width field" \
"ROW  NAME    AGE   COLOR
3    Nike    34    Red" \
  "$OLDPWD/$BIN" find people.tbl AGE=34 --number

assert_output \
  "find no matches succeeds with no output" \
"" \
  "$OLDPWD/$BIN" find people.tbl COLOR=Blue

assert_failure \
  "find rejects unknown field" \
  "$OLDPWD/$BIN" find people.tbl MOOD=Happy

assert_failure \
  "find rejects malformed query" \
  "$OLDPWD/$BIN" find people.tbl COLOR

assert_failure \
  "find rejects unknown option" \
  "$OLDPWD/$BIN" find people.tbl COLOR=Cyan --bad

###############################################################################
# set
###############################################################################

assert_success \
  "set final field" \
  "$OLDPWD/$BIN" set people.tbl 2 COLOR=Blue

assert_file_eq \
  "set updates final field" \
"NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Blue
Nike    34    Red
Ruca    45    Grey" \
  people.tbl

assert_success \
  "set multiple fields" \
  "$OLDPWD/$BIN" set people.tbl 1 AGE=13 COLOR=Green

assert_file_eq \
  "set updates multiple fields" \
"NAME    AGE   COLOR
Erica   13    Green
Alex    23    Blue
Nike    34    Red
Ruca    45    Grey" \
  people.tbl

before="$(cat people.tbl)"

assert_failure \
  "set rejects too-long fixed-width value" \
  "$OLDPWD/$BIN" set people.tbl 2 NAME=Alexandria

after="$(cat people.tbl)"
assert_eq \
  "failed set preserves file" \
  "$before" \
  "$after"

assert_failure \
  "set rejects unknown field" \
  "$OLDPWD/$BIN" set people.tbl 2 MOOD=Happy

assert_failure \
  "set rejects row 0" \
  "$OLDPWD/$BIN" set people.tbl 0 COLOR=Blue

assert_failure \
  "set rejects missing row" \
  "$OLDPWD/$BIN" set people.tbl 99 COLOR=Blue

###############################################################################
# delete
###############################################################################

assert_success \
  "delete row 2" \
  "$OLDPWD/$BIN" delete people.tbl 2

assert_file_eq \
  "delete removes expected row" \
"NAME    AGE   COLOR
Erica   13    Green
Nike    34    Red
Ruca    45    Grey" \
  people.tbl

assert_failure \
  "delete rejects row 0" \
  "$OLDPWD/$BIN" delete people.tbl 0

assert_failure \
  "delete rejects missing row" \
  "$OLDPWD/$BIN" delete people.tbl 99

###############################################################################
# missing fields
###############################################################################

assert_success \
  "add allows missing fixed-width field" \
  "$OLDPWD/$BIN" add people.tbl NAME=June COLOR=Yellow

assert_file_eq \
  "missing field is written empty" \
"NAME    AGE   COLOR
Erica   13    Green
Nike    34    Red
Ruca    45    Grey
June          Yellow" \
  people.tbl

###############################################################################
# final field is unbounded
###############################################################################

assert_success \
  "final field allows long value" \
  "$OLDPWD/$BIN" add people.tbl NAME=Long AGE=99 COLOR=This-is-a-long-unbounded-final-field

assert_output \
  "find long final field" \
"NAME    AGE   COLOR
Long    99    This-is-a-long-unbounded-final-field" \
  "$OLDPWD/$BIN" find people.tbl COLOR=This-is-a-long-unbounded-final-field

echo
echo "PASS: $TESTS_RUN tests"
