/* check_cert_fast.c (faster variant of check_cert.c, same checks) : check the Lemma 6 certificate for a greedy strict 3-sumfree sequence S_{f,g,h}.
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
 * Build: gcc -O3 -march=native -o check_cert_fast check_cert_fast.c
 * Usage: check_cert f g h K M Q        exit 0 = certificate holds, 1 = fails
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

static double now_s(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

typedef uint64_t u64;

static int bit(const u64 *a, u64 i) { return (a[i >> 6] >> (i & 63)) & 1; }

/* dst |= src << s, for the dst bits <= lim (bits above lim may or may not be set; they are never read).
 * src has no bits at or above srcbits. One store per dst word; same arithmetic as the generator
 * sumfree3r_opt.c (reviewed 2026/10/2). Preconditions: srcbits >= 1, lim <= N, arrays have
 * (N>>6)+2 words. */
static void or_shift(u64 *restrict dst, const u64 *restrict src, u64 s, u64 srcbits, u64 lim, u64 N) {
    if (s > lim) return;
    if (srcbits > lim - s + 1) srcbits = lim - s + 1;
    u64 nw = (srcbits + 63) >> 6, ws = s >> 6, bs = s & 63, dw = (N >> 6) + 1;
    u64 *d = dst + ws;
    if (bs == 0) {
        u64 e = nw; if (e + ws > dw) e = dw - ws;
        for (u64 i = 0; i < e; i++) d[i] |= src[i];
    } else {
        u64 e = nw + 1; if (e + ws > dw) e = dw - ws;
        unsigned cb = 64 - bs;
        d[0] |= src[0] << bs;
        u64 m = e < nw ? e : nw;
        for (u64 j = 1; j < m; j++) d[j] |= (src[j] << bs) | (src[j - 1] >> cb);
        if (e > nw) d[nw] |= src[nw - 1] >> cb;
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

    /* progress on stderr every 2^20 positions: percent of N, terms so far, elapsed, ETA.
     * Work per term = bits copied by the two or_shift calls (after their truncation to lim):
     * min(2z, N-z+1) + min(z, N-2z). Integrated over z with a constant term density d this is
     * d * 5N^2/12 (peak near z = N/3, cheap after N/2); done work is summed exactly. */
    double t0 = now_s(), work = 0;
    u64 n = 0, last = 0, nK = 0;
    for (u64 z = 1; z <= N; z++) {
        if ((z & 0xFFFFF) == 0) {
            double el = now_s() - t0, d = (double)n / z, total = d * 5.0 / 12.0 * (double)N * N;
            fprintf(stderr, "progress z=%llu (%.1f%% of N) terms=%llu work=%.1f%% elapsed=%.0fs eta=%.0fs\n",
                    (unsigned long long)z, 100.0 * z / N, (unsigned long long)n, 100.0 * work / total, el,
                    work > 0 ? el * (total - work) / work : 0.0);
        }
        int take = n < 3 ? z == init[n] : !bit(T, z);
        if (!take) continue;
        work += (double)(2 * last + 1 < N - z + 1 ? 2 * last + 1 : N - z + 1);
        if (2 * z < N) work += (double)(last + 1 < N - 2 * z ? last + 1 : N - 2 * z);
        or_shift(T, P2, z, 2 * last + 1, N, N);
        if (z < N) or_shift(P2, C, z, last + 1, N - z - 1, N);   /* a P2 bit q is read only if q+z' <= N, z' > z */
        C[z >> 6] |= 1ULL << (z & 63);
        last = z; n++;
        if (z <= K) nK = n;
    }

    fprintf(stderr, "generation done in %.0fs\n", now_s() - t0);
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
