#!/bin/sh
# Build the faster checker and check the three certificates found at N = 10^8. Each line: f g h K M Q
# One core, about 40 MB of memory; 20-30 minutes per triple when run one at a time.
# Running them in parallel is slower in total (memory bandwidth).
cd "$(dirname "$0")"
gcc -O3 -march=native -Wall -o check_cert_fast check_cert_fast.c || exit 2
st=0
while read f g h K M Q; do
  ./check_cert_fast $f $g $h $K $M $Q 2>/dev/null || st=1
done <<'LIST'
1 11 29 16239979 7104875 11
1 12 31 17606898 4109928 15
1 15 38 13343262 7815075 10
LIST
exit $st
