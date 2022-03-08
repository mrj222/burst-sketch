#ifndef _CAIDADATASET_H_
#define _CAIDADATASET_H_

#include <cstdint>
#include <string>
#include "Mmap.h"

struct CAIDA_Tuple {
    uint64_t timestamp;
    uint64_t id;
};

class CAIDADataset {
public:
    CAIDADataset(std::string PATH, std::string name) {
        filename = name;
        load_result = Load(PATH.c_str());
        dataset = (CAIDA_Tuple*)load_result.start;
        length = load_result.length / sizeof(CAIDA_Tuple);
    }

    ~CAIDADataset() {
        UnLoad(load_result);
    }

public:
    std::string filename;
    LoadResult load_result;
    CAIDA_Tuple *dataset;
    uint64_t length;
};

#endif