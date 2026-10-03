# Periodicity certificates for seven greedy 3-sumfree sequences

This repository proves that the greedy strict 3-sumfree sequences

    S_{1,9,24},  S_{5,6,31},  S_{6,13,37},  S_{7,13,38},
    S_{1,11,29}, S_{1,12,31}, S_{1,15,38}

are ultimately periodic. Each proof is a finite certificate, and a short C program checks it.

These seven triples are among the twelve listed as unresolved in Table 2 of

> J. van Berkel, W. Bosma, *On t-sumfree sequences*, arXiv:2609.16843v1 (15 Sep 2026).

There, no period was found within 500,000 terms. With these seven settled, the exceptional set in
that paper's Theorem 5 has at most 5 elements instead of 12.

## Definitions

Let s_1 < s_2 < s_3 be given. For n > 3, s_n is the least integer greater than s_{n−1} that is not
a sum of three distinct earlier terms. Write C[x] = 1 if x is a term and C[x] = 0 otherwise.

**Lemma 6 of the paper (t = 3).** Suppose that, from position K+1 on, there are Q identical
blocks of length M. Suppose also that none of them is null, i.e. each block contains a term, and
that (Q−6)M ≥ 2K. Then C[x] = C[x+M] for all x > K. Hence the sequence is ultimately periodic
with modulus M.

A certificate is a triple (K, M, Q) satisfying these hypotheses.

## Certificates

| (f,g,h) | K | M | Q | (Q−6)M | 2K | terms per period | checked up to K+QM |
|---|---:|---:|---:|---:|---:|---:|---:|
| (1,9,24) | 2482920 | 1322573 | 10 | 5290292 | 4965840 | 143119 | 15708650 |
| (5,6,31) | 2411293 | 1132156 | 11 | 5660780 | 4822586 | 128448 | 14865009 |
| (6,13,37) | 2115544 | 77948 | 61 | 4287140 | 4231088 | 9430 | 6870372 |
| (7,13,38) | 2198920 | 81020 | 61 | 4456100 | 4397840 | 9924 | 7141140 |
| (1,11,29) | 16239979 | 7104875 | 11 | 35524375 | 32479958 | 765807 | 94393604 |
| (1,12,31) | 17606898 | 4109928 | 15 | 36989352 | 35213796 | 443367 | 79255818 |
| (1,15,38) | 13343262 | 7815075 | 10 | 31260300 | 26686524 | 841333 | 91494012 |

- The first four were found in a computation up to 2×10^7, the last three in one up to 10^8.
- K is the exact last position x ≤ N − M with C[x] ≠ C[x+M], where N = 2×10^7 or 10^8.
- Q is the least value with (Q−6)M ≥ 2K.
- Up to N the identical blocks actually continue: there are 13, 15, 229, 219, 11, 20 and 11 of them.
  For (1,11,29) the observed count equals the required Q.
- The number of terms ≤ K is 268699, 273587, 255886, 269283, 1750470, 1899414 and 1436481.
- For the last three, K+QM exceeds 2×10^7, which is why the 2×10^7 computation could not
  certify them.

## How to verify

You need a C compiler. There are two checkers that test the same three hypotheses:

- `check_cert.c`: the original checker, about 100 lines. `sh run_all.sh` checks the first four
  certificates in about 7 minutes on one core with about 35 MB of memory.
- `check_cert_fast.c`: a faster variant for the larger certificates. It truncates the bitset
  shifts to the bits that can still be read later. `sh run_fast.sh` checks the last three in
  about 20 to 30 minutes each on one core with about 40 MB of memory. It also prints progress
  lines to stderr, which `run_fast.sh` discards.

How the checkers work:

1. They generate the sequence up to K+QM with bitsets. When a term s is added, they update the
   set of 3-sums T and the set of 2-sums P2 by `T |= P2<<s ; P2 |= C<<s`. Every new sum exceeds
   s, so T[z] is final when z is examined.
2. They then check the three hypotheses: (a) the blocks are identical, (b) they are non-null,
   and (c) the inequality holds.

Each prints `certificate HOLDS` for each triple. The exit status is 0 only if all hold.
`check_output.txt` and `check_output_fast.txt` are recorded runs.

Negative controls on (6,13,37) all report `certificate FAILS`:
- Q = 60, one less than needed, fails (c).
- M = 77947 fails (a).
- K = 2115543 fails (a) at x = 2115544.

The checkers do not guard against integer overflow in K+QM for absurdly large arguments; any
arguments with K+QM well below 2^63 are handled correctly.

## Cross-checks behind these numbers

- **Two generators up to 2×10^7.** Two independently written generators produced all twelve
  open sequences of Table 2 up to 2×10^7. They agree on:
  - the term counts;
  - which sequences show a period;
  - K, M, Q and the number of terms per period for the first four triples.
- **One generator up to 10^8.** The remaining eight sequences were extended to 10^8 with a faster
  version of one of the generators. Its output agrees with that generator's earlier output up to
  2×10^7.
  The last three certificates do not rely on this run: the checker regenerates each sequence
  from scratch.
- **The faster checker.** `check_cert_fast.c` was reviewed independently by two AI reviewers
  (Claude and GPT), neither seeing the other's report. Both found no defect. Both compared it
  against naive set-based generators on thousands of small cases with randomized shift tests,
  and found no mismatch. On several smaller inputs, including the (6,13,37) certificate and a
  failing control, its output is byte-identical to that of `check_cert.c`.
- **The original checker on the last three.** `check_cert.c` was also run on the last three
  certificates. All three report `certificate HOLDS`, with the same term counts as above. It took
  about 1 to 1.6 hours per triple on one core.
- **Generating-function test.** For every z ≤ 2×10^7 and all twelve triples, an FFT test
  confirmed that z is a term exactly when z is not a sum of three distinct terms (0 mismatches).
  The test computes e3 = (p1³ − 3p1p2 + 2p3)/6 with an exact split FFT.
- **Paper's Table 1.** The generator reproduces the period data of the paper's Table 1 examples.
  The checked examples include (1,2,3), (2,3,8), (1,2,14), (3,5,14) and (1,2,16).
- **Lemma 6.** The proof of Lemma 6 was re-derived; no gap was found. The step v − M > s_3,
  which the proof uses implicitly, follows from the blocks being non-null and Q ≥ 6.

## The remaining five triples

The triples (4,6,17), (4,5,19), (7,9,26), (5,7,26) and (6,7,27) show no period up to 10^8,
i.e. about 11.5 to 12.2 million terms each. This is not evidence of aperiodicity.

## Credits

Hirotaka Shimizu (GitHub: yygotm). The computations and the write-up were produced with the AI assistants Claude
(Anthropic) and GPT (OpenAI), working independently of each other.

## License

- Code (`check_cert.c`, `check_cert_fast.c`, `run_all.sh`, `run_fast.sh`): MIT, see `LICENSE`.
- Data and text (the certificates, `check_output.txt`, `check_output_fast.txt`, this README):
  CC0 1.0 Universal (public domain dedication), https://creativecommons.org/publicdomain/zero/1.0/
