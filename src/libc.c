// Minimal libc implementations for bare-metal ARM (no newlib).
// Only needed for the device target; the simulator links against system libc.

#if TARGET_PLAYDATE && !TARGET_SIMULATOR

#pragma GCC optimize("no-tree-loop-distribute-patterns")

#include <stddef.h>
#include <stdint.h>

#include "playdate.h"

// malloc/free/realloc via Playdate API (setup.c only provides _malloc_r for newlib).
void *malloc(size_t size)              { return playdate->system->realloc(NULL, size); }
void *calloc(size_t n, size_t size)    { void *p = malloc(n * size); if (p) memset(p, 0, n * size); return p; }
void *realloc(void *ptr, size_t size)  { return playdate->system->realloc(ptr, size); }
void  free(void *ptr)                  { if (ptr) playdate->system->realloc(ptr, 0); }

// Cortex-M7 supports unaligned word access via LDR/STR, so we can
// always use 32-bit transfers. Use volatile to prevent GCC from
// converting loops back into library calls.

__attribute__((used, noinline))
void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    // Align dest to 4-byte boundary
    while (n && ((uintptr_t)d & 3)) { *d++ = *s++; n--; }
    // Word copy — Cortex-M7 handles unaligned src in hardware
    if (n >= 4) {
        volatile uint32_t *d32 = (volatile uint32_t *)d;
        const volatile uint32_t *s32 = (const volatile uint32_t *)s;
        while (n >= 16) {
            d32[0] = s32[0]; d32[1] = s32[1];
            d32[2] = s32[2]; d32[3] = s32[3];
            d32 += 4; s32 += 4; n -= 16;
        }
        while (n >= 4) { *d32++ = *s32++; n -= 4; }
        d = (uint8_t *)d32; s = (const uint8_t *)s32;
    }
    while (n--) *d++ = *s++;
    return dest;
}

// ARM EABI runtime (clang emits these instead of memcpy/memset).
__attribute__((used, noinline))
void __aeabi_memcpy(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
__attribute__((used, noinline))
void __aeabi_memcpy4(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
__attribute__((used, noinline))
void __aeabi_memcpy8(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }

__attribute__((used, noinline))
void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    if (d <= s || d >= s + n) {
        return memcpy(dest, src, n);
    }
    // Overlap, copy backward
    d += n; s += n;
    while (n && ((uintptr_t)d & 3)) { *--d = *--s; n--; }
    if (n >= 4) {
        volatile uint32_t *d32 = (volatile uint32_t *)d;
        const volatile uint32_t *s32 = (const volatile uint32_t *)s;
        while (n >= 4) { *--d32 = *--s32; n -= 4; }
        d = (uint8_t *)d32; s = (const uint8_t *)s32;
    }
    while (n--) *--d = *--s;
    return dest;
}

__attribute__((used, noinline))
void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    uint8_t val = (uint8_t)c;
    while (n && ((uintptr_t)p & 3)) { *p++ = val; n--; }
    if (n >= 4) {
        uint32_t word = val | (val << 8) | (val << 16) | (val << 24);
        volatile uint32_t *p32 = (volatile uint32_t *)p;
        while (n >= 16) {
            p32[0] = word; p32[1] = word;
            p32[2] = word; p32[3] = word;
            p32 += 4; n -= 16;
        }
        while (n >= 4) { *p32++ = word; n -= 4; }
        p = (uint8_t *)p32;
    }
    while (n--) *p++ = val;
    return s;
}

__attribute__((used, noinline))
int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *a = s1, *b = s2;
    while (n--) {
        if (*a != *b) return *a - *b;
        a++; b++;
    }
    return 0;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s1 == *s2) { s1++; s2++; n--; }
    return n ? *(unsigned char *)s1 - *(unsigned char *)s2 : 0;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) d++;
    while (n-- && *src) *d++ = *src++;
    *d = '\0';
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : NULL;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}

int atoi(const char *s) {
    int n = 0, neg = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return neg ? -n : n;
}

long strtol(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    long result = 0;
    int neg = 0;

    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;

    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16; s += 2;
    } else if (base == 0 && s[0] == '0') {
        base = 8; s++;
    } else if (base == 0) {
        base = 10;
    }

    while (*s) {
        int digit;
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'z') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') digit = *s - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        s++;
    }

    if (endptr) *endptr = (char *)s;
    return neg ? -result : result;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    return (unsigned long)strtol(nptr, endptr, base);
}

int abs(int j) {
    return j < 0 ? -j : j;
}

char *strdup(const char *s) {
    extern void *malloc(size_t);
    size_t len = strlen(s) + 1;
    char *d = malloc(len);
    if (d) memcpy(d, s, len);
    return d;
}

// Math functions used by TinySoundFont (MIDI synth).
// Cortex-M7 has HW FPU for basic ops; transcendentals need software.

float sqrtf(float x) {
    float result;
    __asm__ volatile ("vsqrt.f32 %0, %1" : "=t"(result) : "t"(x));
    return result;
}

float fabsf(float x) {
    float result;
    __asm__ volatile ("vabs.f32 %0, %1" : "=t"(result) : "t"(x));
    return result;
}

float fmodf(float x, float y) {
    float q = x / y;
    // truncate toward zero
    int qi = (int)q;
    return x - (float)qi * y;
}

// High-accuracy logf using range reduction + minimax polynomial.
// Accurate to ~23 bits (full float precision).
float logf(float x) {
    union { float f; uint32_t u; } u = { x };
    if (x <= 0.0f) return (x == 0.0f) ? -1e30f : 0.0f/0.0f;

    // Decompose: x = m * 2^e where sqrt(0.5) <= m < sqrt(2)
    int e = (int)((u.u >> 23) & 0xFF) - 127;
    u.u = (u.u & 0x007FFFFF) | 0x3F800000;
    float m = u.f;
    if (m > 1.41421356f) { m *= 0.5f; e++; }

    // log(m) via Remez polynomial for m in [sqrt(0.5), sqrt(2)]
    float t = (m - 1.0f) / (m + 1.0f);
    float t2 = t * t;
    // log(m) = 2*t*(1 + t^2/3 + t^4/5 + t^6/7 + t^8/9)
    float r = t * (2.0f + t2 * (0.6666666666f + t2 * (0.4f + t2 * (0.2857142857f + t2 * 0.2222222222f))));

    return (float)e * 0.69314718056f + r;
}

// High-accuracy expf using range reduction + polynomial.
float expf(float x) {
    if (x > 88.72f) return 1e30f;
    if (x < -87.33f) return 0.0f;

    // exp(x) = 2^(x * log2(e)) = 2^k * 2^f
    float xlog2e = x * 1.44269504089f;
    int k = (int)xlog2e;
    if (xlog2e < 0.0f && (float)k != xlog2e) k--;
    float f = xlog2e - (float)k;

    // 2^f via minimax polynomial for f in [0,1), accurate to ~23 bits
    float p = 1.0f + f * (0.69314718056f + f * (0.24022650695f
            + f * (0.05550410866f + f * (0.00961812910f
            + f * (0.00133335581f + f * 0.00015469732f)))));

    // 2^k via bit manipulation
    union { float fl; uint32_t u; } u;
    u.u = (uint32_t)((k + 127) << 23);
    return p * u.fl;
}

float powf(float base, float exp) {
    if (base <= 0.0f) {
        if (base == 0.0f) return (exp > 0.0f) ? 0.0f : 1.0f;
        return 0.0f;
    }
    return expf(exp * logf(base));
}

float log10f(float x) {
    return logf(x) * 0.43429448190f;
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
    // Simple insertion sort - sufficient for DOOM's small sorts.
    char *b = base;
    for (size_t i = 1; i < nmemb; i++) {
        for (size_t j = i; j > 0; j--) {
            char *a = b + (j - 1) * size;
            char *c = b + j * size;
            if (compar(a, c) <= 0) break;
            // swap
            for (size_t k = 0; k < size; k++) {
                char tmp = a[k]; a[k] = c[k]; c[k] = tmp;
            }
        }
    }
}

void exit(int status) {
    (void)status;
    while (1) __builtin_trap();
}

void abort(void) {
    __builtin_trap();
    while (1);
}

#endif // TARGET_PLAYDATE && !TARGET_SIMULATOR
