#!/bin/bash
# verify perft numbers (positions from https://www.chessprogramming.org/Perft_Results)

TESTS_FAILED=0

error()
{
  echo "perft testing failed on line $1"
  exit 1
}
trap 'error ${LINENO}' ERR

echo "perft testing started"

EXPECT_SCRIPT=$(mktemp)

cat << 'EOF' > $EXPECT_SCRIPT
#!/usr/bin/expect -f
set timeout 120
lassign [lrange $argv 0 3] pos depth result logfile
log_file -noappend $logfile
spawn ./build/Zugzwang

send "position $pos\n"
send "go perft $depth\n"

expect {
  -re "Total: $result nodes.*" {}
  timeout {puts "TIMEOUT: Expected $result nodes"; exit 1}
  eof {puts "EOF: Engine crashed"; exit 2}
}

send "quit\n"
expect eof
EOF

chmod +x $EXPECT_SCRIPT

run_test() {
  local pos="$1"
  local depth="$2"
  local expected="$3"
  local tmp_file=$(mktemp)

  echo -n "Testing depth $depth: ${pos:0:40}... "

  if $EXPECT_SCRIPT "$pos" "$depth" "$expected" "$tmp_file" > /dev/null 2>&1; then
    echo "OK"
    rm -f "$tmp_file"
  else
    local exit_code=$?
    echo "FAILED (exit code: $exit_code)"
    echo "===== Output for failed test ====="
    cat "$tmp_file"
    echo "=================================="
    rm -f "$tmp_file"
    TESTS_FAILED=1
  fi
}

# standard positions

run_test "startpos" 6 119060324
run_test "fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -" 5 193690690
run_test "fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -" 7 178633661
run_test "fen r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1" 6 706045033
run_test "fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8" 5 89941194
run_test "fen r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10" 5 164075551
run_test "fen r7/4p3/5p1q/3P4/4pQ2/4pP2/6pp/R3K1kr w Q - 1 3" 5 11609488

rm -f $EXPECT_SCRIPT
echo "perft testing completed"

if [ $TESTS_FAILED -ne 0 ]; then
  echo "Some tests failed"
  exit 1
fi