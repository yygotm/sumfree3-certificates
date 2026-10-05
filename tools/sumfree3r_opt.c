/* sumfree3r.c : resumable version of sumfree3.c (same rule, same bitset update).
 *
 * Usage: sumfree3r f g h N out.bin
 *   Writes the terms as little-endian uint32 to out.bin.
 *   State is saved to out.bin.ckpt about every CKPT_SEC seconds (written to .tmp, then renamed).
 *   If out.bin.ckpt exists, the run resumes from it; out.bin is truncated to the saved term count.
 *   On completion the checkpoint is removed and out.bin.done is written.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef uint64_t u64;
static long CKPT_SEC = 1200;  /* override with env CKPT_SEC */

static void or_shift(u64 *restrict dst, const u64 *restrict src, u64 s, u64 srcbits, u64 lim, u64 N) {
    /* dst bits above lim are never read later, so they are not needed */
    if (s > lim) return;
    if (srcbits > lim - s + 1) srcbits = lim - s + 1;
    u64 nw = (srcbits + 63) / 64, ws = s / 64, bs = s % 64, dw = N / 64 + 1;
    if (bs == 0) {
        u64 e = nw; if (e + ws > dw) e = dw - ws;
        u64 *d = dst + ws;
        for (u64 i = 0; i < e; i++) d[i] |= src[i];
    } else {
        /* one store per dst word: d[j] |= src[j]<<bs | src[j-1]>>(64-bs), j = 0..nw */
        u64 e = nw + 1; if (e + ws > dw) e = dw - ws;   /* j < e */
        u64 *d = dst + ws;
        unsigned cb = 64 - bs;
        d[0] |= src[0] << bs;
        u64 m = e < nw ? e : nw;                       /* j in [1, m): both parts */
        for (u64 j = 1; j < m; j++) d[j] |= (src[j] << bs) | (src[j - 1] >> cb);
        if (e > nw) d[nw] |= src[nw - 1] >> cb;
    }
}

struct hdr { u64 magic, f, g, h, N, W, z, n, last; };
#define MAGIC 0x3353554d46524545ULL

static int save(const char *ck, struct hdr *H, u64 *C, u64 *P2, u64 *T, FILE *out) {
    char tmp[4200]; snprintf(tmp, sizeof tmp, "%s.tmp", ck);
    fflush(out); fsync(fileno(out));
    FILE *fp = fopen(tmp, "wb");
    if (!fp) return -1;
    int ok = fwrite(H, sizeof *H, 1, fp) == 1 && fwrite(C, 8, H->W, fp) == H->W &&
             fwrite(P2, 8, H->W, fp) == H->W && fwrite(T, 8, H->W, fp) == H->W;
    ok = ok && fflush(fp) == 0 && fsync(fileno(fp)) == 0;
    fclose(fp);
    if (!ok) return -1;
    return rename(tmp, ck);
}

int main(int argc, char **argv) {
    if (argc < 6) { fprintf(stderr, "usage: sumfree3r f g h N out.bin\n"); return 1; }
    u64 init[3] = {strtoull(argv[1], 0, 10), strtoull(argv[2], 0, 10), strtoull(argv[3], 0, 10)};
    u64 N = strtoull(argv[4], 0, 10), W = N / 64 + 2;
    char ck[4096]; snprintf(ck, sizeof ck, "%s.ckpt", argv[5]);
    u64 *C = calloc(W, 8), *P2 = calloc(W, 8), *T = calloc(W, 8);
    if (!C || !P2 || !T) { fprintf(stderr, "alloc failed\n"); return 1; }
    struct hdr H = {MAGIC, init[0], init[1], init[2], N, W, 1, 0, 0};
    FILE *out;
    FILE *fp = fopen(ck, "rb");
    if (fp) {
        struct hdr R;
        if (fread(&R, sizeof R, 1, fp) != 1 || R.magic != MAGIC || R.f != H.f || R.g != H.g || R.h != H.h ||
            R.N != N || R.W != W || fread(C, 8, W, fp) != W || fread(P2, 8, W, fp) != W || fread(T, 8, W, fp) != W) {
            fprintf(stderr, "bad checkpoint %s\n", ck); return 1;
        }
        fclose(fp);
        H = R;
        if (truncate(argv[5], (off_t)(H.n * 4)) != 0) { perror("truncate"); return 1; }
        out = fopen(argv[5], "ab");
        fprintf(stderr, "resumed at z=%llu n=%llu\n", (unsigned long long)H.z, (unsigned long long)H.n);
    } else {
        out = fopen(argv[5], "wb");
    }
    if (!out) { fprintf(stderr, "open failed\n"); return 1; }
    if (getenv("CKPT_SEC")) CKPT_SEC = atol(getenv("CKPT_SEC"));
    time_t t0 = time(0);
    for (u64 z = H.z; z <= N; z++) {
        int take;
        if (H.n < 3) take = (z == init[H.n]);
        else take = !((T[z / 64] >> (z % 64)) & 1);
        if (take) {
            or_shift(T, P2, z, 2 * H.last + 1, N, N);
            if (z < N) or_shift(P2, C, z, H.last + 1, N - z - 1, N); /* later use: bit q read only if q+z' <= N, z' > z */
            C[z / 64] |= 1ULL << (z % 64);
            uint32_t v = (uint32_t)z;
            fwrite(&v, 4, 1, out);
            H.last = z; H.n++;
            if ((H.n & 0xFFFF) == 0) {
                fprintf(stderr, "%s,%s,%s: n=%llu z=%llu (%.2f%%)\n", argv[1], argv[2], argv[3],
                        (unsigned long long)H.n, (unsigned long long)z, 100.0 * z / N);
                if (time(0) - t0 >= CKPT_SEC) {
                    H.z = z + 1;
                    if (save(ck, &H, C, P2, T, out) != 0) { fprintf(stderr, "checkpoint failed\n"); return 1; }
                    t0 = time(0);
                }
            }
        }
    }
    fclose(out);
    remove(ck);
    char done[4096]; snprintf(done, sizeof done, "%s.done", argv[5]);
    FILE *d = fopen(done, "w");
    if (d) { fprintf(d, "%s %s %s N=%llu terms=%llu last=%llu\n", argv[1], argv[2], argv[3],
                     (unsigned long long)N, (unsigned long long)H.n, (unsigned long long)H.last); fclose(d); }
    fprintf(stderr, "%s,%s,%s: done n=%llu up to N=%llu\n", argv[1], argv[2], argv[3],
            (unsigned long long)H.n, (unsigned long long)N);
    return 0;
}
