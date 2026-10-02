# Periodicity certificates for four greedy 3-sumfree sequences

This repository proves that the greedy strict 3-sumfree sequences

    S_{1,9,24},  S_{5,6,31},  S_{6,13,37},  S_{7,13,38}

are ultimately periodic. Each proof is a finite certificate, and a short C program checks it.

These four triples are among the twelve listed as unresolved in Table 2 of

> J. van Berkel, W. Bosma, *On t-sumfree sequences*, arXiv:2609.16843v1 (15 Sep 2026).

There, no period was found within 500,000 terms. With these four settled, the exceptional set in
that paper's Theorem 5 has at most 8 elements instead of 12.

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

- K is the exact last position x ≤ 2×10^7 − M with C[x] ≠ C[x+M].
- Q is the least value with (Q−6)M ≥ 2K.
- Up to 2×10^7 the identical blocks actually continue: there are 13, 15, 229 and 219 of them.
- The number of terms ≤ K is 268699, 273587, 255886 and 269283 respectively.

## How to verify

You need a C compiler and about 35 MB of memory. All four checks take about 7 minutes on one core.

    sh run_all.sh

`check_cert.c` is a self-contained program of about 100 lines:

1. It generates the sequence up to K+QM with bitsets. When a term s is added, it updates the set
   of 3-sums T and the set of 2-sums P2 by `T |= P2<<s ; P2 |= C<<s`. Every new sum exceeds s,
   so T[z] is final when z is examined.
2. It then checks the three hypotheses: (a) the blocks are identical, (b) they are non-null,
   and (c) the inequality holds.

It prints `certificate HOLDS` for each triple. The exit status is 0 only if all four hold.
`check_output.txt` is a recorded run.

Negative controls on (6,13,37) all report `certificate FAILS`:
- Q = 60, one less than needed, fails (c).
- M = 77947 fails (a).
- K = 2115543 fails (a) at x = 2115544.

## Cross-checks behind these numbers

- **Two generators.** Two independently written generators produced all twelve open sequences
  of Table 2 up to 2×10^7. They agree on:
  - the term counts;
  - which sequences show a period;
  - K, M, Q and the number of terms per period for the four triples above.
- **Generating-function test.** For every z ≤ 2×10^7 and all twelve triples, an FFT test
  confirmed that z is a term exactly when z is not a sum of three distinct terms (0 mismatches).
  The test computes e3 = (p1³ − 3p1p2 + 2p3)/6 with an exact split FFT.
- **Paper's Table 1.** The generator reproduces the period data of the paper's Table 1 examples.
  The checked examples include (1,2,3), (2,3,8), (1,2,14), (3,5,14) and (1,2,16).
- **Lemma 6.** The proof of Lemma 6 was re-derived; no gap was found. The step v − M > s_3,
  which the proof uses implicitly, follows from the blocks being non-null and Q ≥ 6.

## The remaining eight triples

The triples (4,6,17), (4,5,19), (7,9,26), (1,11,29), (1,12,31), (5,7,26), (6,7,27) and (1,15,38)
show no period up to 2×10^7, i.e. about 2.15 to 2.45 million terms each. This is not evidence
of aperiodicity. An extension to 10^8 is in progress.

## Credits

Hirotaka Shimizu (GitHub: yygotm). The computations and the write-up were produced with the AI assistants Claude
(Anthropic) and GPT (OpenAI), working independently of each other.

## License

- Code (`check_cert.c`, `run_all.sh`): MIT, see `LICENSE`.
- Data and text (the certificates, `check_output.txt`, this README): CC0 1.0 Universal
  (public domain dedication), https://creativecommons.org/publicdomain/zero/1.0/
