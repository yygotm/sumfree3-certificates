// HIP version of sumfree3r.c: same rule, same bitset update, one term per step.
// Usage: sumfree3_hip f g h N out.bin   (terms as little-endian uint32, same format as sumfree3r.c)
//
// Per term z (state kept on the device; the host only launches kernels):
//   k_t : T  |= P2 << z   (sums of three distinct terms)
//   k_p : P2 |= C  << z   (sums of two distinct terms; C does not contain z yet)
//   k_n : C  |= bit z; next term = init[n] for n < 3, else the smallest z' > z with T bit 0
// Build: hipcc -O3 -std=c++17 --offload-arch=gfx1201 sumfree3_hip.cpp -o sumfree3_hip
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <chrono>

typedef unsigned long long u64;
#define CK(x) do { hipError_t e_ = (x); if (e_ != hipSuccess) { \
    fprintf(stderr, "HIP error %s at %s:%d\n", hipGetErrorString(e_), __FILE__, __LINE__); exit(1); } } while (0)

// st: [0]=z (current term), [1]=n (terms written), [2]=done (1 = passed N, 2 = overflow)
__global__ void k_t(u64 *T, const u64 *P2, const u64 *st, u64 W) {
    if (st[2]) return;
    u64 z = st[0], ws = z >> 6; unsigned bs = (unsigned)(z & 63);
    u64 hi = ((3 * z) >> 6) + 1; if (hi > W - 1) hi = W - 1;
    for (u64 d = ws + blockIdx.x * (u64)blockDim.x + threadIdx.x; d <= hi; d += (u64)gridDim.x * blockDim.x) {
        u64 j = d - ws, v = P2[j] << bs;
        if (bs && j) v |= P2[j - 1] >> (64 - bs);
        T[d] |= v;
    }
}
__global__ void k_p(u64 *P2, const u64 *C, const u64 *st, u64 W) {
    if (st[2]) return;
    u64 z = st[0], ws = z >> 6; unsigned bs = (unsigned)(z & 63);
    u64 hi = ((2 * z) >> 6) + 1; if (hi > W - 1) hi = W - 1;
    for (u64 d = ws + blockIdx.x * (u64)blockDim.x + threadIdx.x; d <= hi; d += (u64)gridDim.x * blockDim.x) {
        u64 j = d - ws, v = C[j] << bs;
        if (bs && j) v |= C[j - 1] >> (64 - bs);
        P2[d] |= v;
    }
}
// one block of 256
__global__ void k_n(u64 *C, const u64 *T, u64 *st, uint32_t *terms, u64 W, u64 N, u64 i1, u64 i2, u64 cap) {
    __shared__ u64 best;
    if (st[2]) return;
    u64 z = st[0], n = st[1];
    unsigned lid = threadIdx.x;
    if (lid == 0) { C[z >> 6] |= 1ULL << (z & 63); best = ~0ULL; }
    __syncthreads();
    u64 nz;
    if (n < 3) {
        nz = n == 1 ? i1 : i2;
    } else {
        u64 s = z + 1, w0 = s >> 6;
        for (u64 base = w0; base < W; base += 256) {
            u64 w = base + lid;
            if (w < W) {
                u64 m = ~T[w];
                if (w == w0) m &= ~0ULL << (s & 63);
                if (m) atomicMin(&best, (w << 6) + (u64)__ffsll((long long)m) - 1);
            }
            __syncthreads();
            if (best != ~0ULL) break;
            __syncthreads();
        }
        nz = best;
    }
    if (lid == 0) {
        if (nz > N) st[2] = 1;
        else if (n >= cap) st[2] = 2;
        else { terms[n] = (uint32_t)nz; st[0] = nz; st[1] = n + 1; }
    }
}

int main(int argc, char **argv) {
    if (argc < 6) { fprintf(stderr, "usage: sumfree3_hip f g h N out.bin\n"); return 1; }
    u64 f = strtoull(argv[1], 0, 10), g = strtoull(argv[2], 0, 10), h = strtoull(argv[3], 0, 10);
    u64 N = strtoull(argv[4], 0, 10), W = N / 64 + 2, cap = N / 4 + 16;
    u64 *T, *P2, *C, *st; uint32_t *terms;
    CK(hipMalloc(&T, W * 8)); CK(hipMalloc(&P2, W * 8)); CK(hipMalloc(&C, W * 8));
    CK(hipMalloc(&st, 3 * 8)); CK(hipMalloc(&terms, cap * 4));
    CK(hipMemset(T, 0, W * 8)); CK(hipMemset(P2, 0, W * 8)); CK(hipMemset(C, 0, W * 8));
    u64 st_h[3] = {f, 1, 0};
    uint32_t f32 = (uint32_t)f;
    CK(hipMemcpy(st, st_h, sizeof st_h, hipMemcpyHostToDevice));
    CK(hipMemcpy(terms, &f32, 4, hipMemcpyHostToDevice));
    int G = argc > 6 ? atoi(argv[6]) : 256;  // blocks of 256 for k_t / k_p
    auto t0 = std::chrono::steady_clock::now();
    const int BATCH = 4096;
    u64 lastrep = 0;
    for (;;) {
        for (int i = 0; i < BATCH; i++) {
            k_t<<<G, 256>>>(T, P2, st, W);
            k_p<<<G, 256>>>(P2, C, st, W);
            k_n<<<1, 256>>>(C, T, st, terms, W, N, g, h, cap);
        }
        CK(hipGetLastError());
        CK(hipMemcpy(st_h, st, sizeof st_h, hipMemcpyDeviceToHost));
        if (st_h[2]) break;
        double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        if (el - lastrep >= 60) {
            lastrep = (u64)el;
            fprintf(stderr, "%llu,%llu,%llu: n=%llu z=%llu (%.2f%%) %.0fs\n", f, g, h, st_h[1], st_h[0], 100.0 * st_h[0] / N, el);
        }
    }
    if (st_h[2] != 1) { fprintf(stderr, "terms buffer overflow\n"); return 1; }
    u64 n = st_h[1];
    std::vector<uint32_t> res(n);
    CK(hipMemcpy(res.data(), terms, n * 4, hipMemcpyDeviceToHost));
    FILE *fp = fopen(argv[5], "wb");
    if (!fp || fwrite(res.data(), 4, n, fp) != n || fclose(fp) != 0) { fprintf(stderr, "write failed\n"); return 1; }
    double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    fprintf(stderr, "%llu,%llu,%llu: done n=%llu last=%u up to N=%llu in %.1fs\n", f, g, h, n, res[n - 1], N, el);
    return 0;
}
