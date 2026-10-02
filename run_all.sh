#!/bin/sh
# Build and check the four certificates. Each line: f g h K M Q
cd "$(dirname "$0")"
gcc -O2 -Wall -o check_cert check_cert.c || exit 2
st=0
while read f g h K M Q; do
  ./check_cert $f $g $h $K $M $Q || st=1
done <<'LIST'
1 9 24 2482920 1322573 10
5 6 31 2411293 1132156 11
6 13 37 2115544 77948 61
7 13 38 2198920 81020 61
LIST
exit $st
