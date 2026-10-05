/* Fast version of exactk.py (period.py + exact K + Lemma 6 test), for large N.
 *
 * Usage: period_fast N out.bin
 *   1. D = differences of the terms. Find the least p (<= n/3) such that the last PROBE differences
 *      reoccur p positions earlier and D[i] = D[i-p] holds on the longest tail; report the preperiod k.
 *   2. M = S[i+p] - S[i] on the periodic tail (the modulus).
 *   3. C = characteristic sequence up to N. K = last x <= N-M with C[x] != C[x+M].
 *      Qmax = (N-K)/M, Qneed = 6 + ceil(2K/M), nonnull = every block C[K+1+iM .. K+(i+1)M] has a 1.
 *      lemma6 = Qmax >= Qneed and nonnull.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PROBE 64

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: period_fast N out.bin\n"); return 1; }
    uint64_t N = strtoull(argv[1], 0, 10);
    FILE *fp = fopen(argv[2], "rb");
    if (!fp) { perror("open"); return 1; }
    fseek(fp, 0, SEEK_END); long sz = ftell(fp); fseek(fp, 0, SEEK_SET);
    size_t ns = sz / 4;
    uint32_t *S = malloc(ns * 4);
    if (!S || fread(S, 4, ns, fp) != ns) { fprintf(stderr, "read failed\n"); return 1; }
    fclose(fp);
    size_t n = ns - 1;
    uint32_t *D = malloc(n * 4);
    for (size_t i = 0; i < n; i++) D[i] = S[i + 1] - S[i];

    /* candidates p: tail window D[n-PROBE..n) equals D[n-PROBE-p..n-p); smallest p first */
    size_t t0 = n - PROBE, bestp = 0, bestk = 0;
    for (size_t p = 1; p <= n / 3; p++) {
        if (D[t0 - p] != D[t0]) continue;
        if (memcmp(D + t0 - p, D + t0, PROBE * 4) != 0) continue;
        /* extend backwards: largest k with D[i] = D[i-p] for all i >= k+p ... i < n */
        size_t i = n;
        while (i > p && D[i - 1] == D[i - 1 - p]) i--;
        /* periodic from index i-p (D[j] = D[j+p] for j >= i-p) */
        size_t k = i - p;
        if (n - k >= 3 * p) { bestp = p; bestk = k; break; }   /* tail covers at least 3 periods */
    }
    const char *name = strrchr(argv[2], '/'); name = name ? name + 1 : argv[2];
    if (!bestp) { printf("%s terms %zu NO PERIOD FOUND\n", name, ns); return 0; }
    uint64_t M = (uint64_t)S[bestk + bestp] - S[bestk];

    uint64_t W = N / 64 + 2;
    uint64_t *C = calloc(W, 8);
    for (size_t i = 0; i < ns; i++) if (S[i] <= N) C[S[i] >> 6] |= 1ULL << (S[i] & 63);
#define BIT(x) ((C[(x) >> 6] >> ((x) & 63)) & 1)
    uint64_t K = 0;
    for (uint64_t x = N - M + 1; x-- > 0;) if (BIT(x) != BIT(x + M)) { K = x; break; }
    uint64_t Qmax = (N - K) / M, Qneed = 6 + (2 * K + M - 1) / M;
    int nonnull = 1;
    for (uint64_t q = 0; q < Qmax && nonnull; q++) {
        uint64_t a = K + 1 + q * M, b = a + M; int any = 0;
        for (uint64_t x = a; x < b; x++) if (BIT(x)) { any = 1; break; }
        nonnull = any;
    }
    printf("%s terms=%zu p=%zu k=%zu M=%llu K=%llu Qmax=%llu Qneed=%llu nonnull=%d lemma6=%d\n", name, ns, bestp, bestk,
           (unsigned long long)M, (unsigned long long)K, (unsigned long long)Qmax, (unsigned long long)Qneed,
           nonnull, Qmax >= Qneed && nonnull);
    return 0;
}
