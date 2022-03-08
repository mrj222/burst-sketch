#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <cstdint>
#include <cstring>

struct Bitmap {
    uint8_t* bitset;
    uint32_t size;

    Bitmap(uint32_t length) {
        size = ((length + 7) >> 3);
        bitset = new uint8_t[size];
        memset(bitset, 0, size * sizeof(uint8_t));
    }

    ~Bitmap () {
        delete [] bitset;
    }

    inline void Set(uint32_t index) {
        uint32_t position = (index >> 3);
        uint32_t offset = (index & 0x7);
        bitset[position] |= (1 << offset);
    }

    inline bool Get(uint32_t index) {
        uint32_t position = (index >> 3);
        uint32_t offset = (index & 0x7);
        return (bitset[position] & (1 << offset));
    }

    inline void Clear(uint32_t index) {
        uint32_t position = (index >> 3);
        uint32_t offset = (index & 0x7);
        bitset[position] &= (~(1 << offset));
    }

    inline void ClearAll() {
        memset(bitset, 0, sizeof(uint8_t) * size);
    }
};

#endif
