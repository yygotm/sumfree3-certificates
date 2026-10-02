/* check_cert.c : check the Lemma 6 certificate for a greedy strict 3-sumfree sequence S_{f,g,h}.
 *
 * Sequence: s_1=f < s_2=g < s_3=h; for n>3, s_n = least integer > s_{n-1} that is not a sum of
 * 3 distinct earlier terms (van Berkel-Bosma, arXiv:2609.16843, t=3).
 * C[x] = 1 iff x is a term.
 *
 * Certificate (K, M, Q) is accepted iff
 *   (a) C[x] = C[x+M] for all K+1 <= x <= K+(Q-1)M   (Q identical blocks of length M from K+1),
 *   (b) every block [K+1+jM, K+(j+1)M], 0 <= j < Q, contains a term   (non-null),
 *   (c) (Q-6)M >= 2K.
 * By Lemma 6 of the paper, the sequence is then ultimately periodic: C[x] = C[x+M] for all x > K.
 *
 * Generation: bitsets. When a term s is added: T |= P2<<s ; P2 |= C<<s ; C[s]=1
 * (T = sums of 3 distinct terms, P2 = sums of 2 distinct terms). Every new sum exceeds s,
 * so T[z] is final when z is examined.
 *
 * Build: gcc -O2 -o check_cert check_cert.c
 * Usage: check_cert f g h K M Q        exit 0 = certificate holds, 1 = fails
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint64_t u64;

static int bit(const u64 *a, u64 i) { return (a[i >> 6] >> (i & 63)) & 1; }

/* dst |= src << s on bits [0, N] ; src has no bits at or above srcbits */
static void or_shift(u64 *dst, const u64 *src, u64 s, u64 srcbits, u64 N) {
    if (s > N) return;
    if (srcbits > N - s + 1) srcbits = N - s + 1;
    u64 nw = (srcbits + 63) >> 6, ws = s >> 6, bs = s & 63, dw = (N >> 6) + 1;
    for (u64 i = 0; i < nw && i + ws < dw; i++) {
        dst[i + ws] |= src[i] << bs;
        if (bs && i + ws + 1 < dw) dst[i + ws + 1] |= src[i] >> (64 - bs);
    }
}

int main(int argc, char **argv) {
    if (argc != 7) { fprintf(stderr, "usage: check_cert f g h K M Q\n"); return 2; }
    u64 init[3], K, M, Q;
    for (int i = 0; i < 3; i++) init[i] = strtoull(argv[1 + i], 0, 10);
    K = strtoull(argv[4], 0, 10); M = strtoull(argv[5], 0, 10); Q = strtoull(argv[6], 0, 10);
    if (!(0 < init[0] && init[0] < init[1] && init[1] < init[2]) || M == 0 || Q < 6) {
        fprintf(stderr, "bad arguments\n"); return 2;
    }
    u64 N = K + Q * M, W = (N >> 6) + 2;
    u64 *C = calloc(W, 8), *P2 = calloc(W, 8), *T = calloc(W, 8);
    if (!C || !P2 || !T) { fprintf(stderr, "out of memory\n"); return 2; }

    u64 n = 0, last = 0, nK = 0;
    for (u64 z = 1; z <= N; z++) {
        int take = n < 3 ? z == init[n] : !bit(T, z);
        if (!take) continue;
        or_shift(T, P2, z, 2 * last + 1, N);
        or_shift(P2, C, z, last + 1, N);
        C[z >> 6] |= 1ULL << (z & 63);
        last = z; n++;
        if (z <= K) nK = n;
    }

    int ok = 1;
    u64 bad = 0;
    for (u64 x = K + 1; x <= K + (Q - 1) * M; x++)
        if (bit(C, x) != bit(C, x + M)) { ok = 0; bad = x; break; }
    u64 p = 0;
    for (u64 x = K + 1; x <= K + M; x++) p += bit(C, x);   /* terms per block */
    int nonnull = p > 0;                                    /* blocks are identical by (a) */
    int ineq = (Q - 6) * M >= 2 * K;

    printf("S_{%llu,%llu,%llu}: generated to N=%llu, %llu terms\n",
           (unsigned long long)init[0], (unsigned long long)init[1], (unsigned long long)init[2],
           (unsigned long long)N, (unsigned long long)n);
    printf("  terms <= K: %llu ; block: M=%llu, %llu terms per block\n",
           (unsigned long long)nK, (unsigned long long)M, (unsigned long long)p);
    printf("  (a) identical blocks: %s", ok ? "yes\n" : "NO, first mismatch at x=");
    if (!ok) printf("%llu\n", (unsigned long long)bad);
    printf("  (b) non-null: %s\n  (c) (Q-6)M=%llu >= 2K=%llu: %s\n", nonnull ? "yes" : "NO",
           (unsigned long long)((Q - 6) * M), (unsigned long long)(2 * K), ineq ? "yes" : "NO");
    int all = ok && nonnull && ineq;
    printf("  certificate %s\n", all ? "HOLDS: ultimately periodic" : "FAILS");
    return all ? 0 : 1;
}
