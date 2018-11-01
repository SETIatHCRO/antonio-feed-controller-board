#include "adler.h"

uint32_t adler(unsigned int len, unsigned char *data) {
    int MOD_ADLER = 65521;

    uint32_t a = 1;
    uint32_t b = 0;

    unsigned int i;

    for (i = 0; i < len; ++i) {
        a = (a + data[i]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }

    return (b << 16 | a);
}