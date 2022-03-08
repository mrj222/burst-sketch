#ifndef _NETDATASET_H_
#define _NETDATASET_H_

#include <cstdint>
#include <string>
#include "Mmap.h"

struct Net_Tuple {	
	uint32_t id;
	uint32_t other_id;
	uint32_t timestamp;
};

class NetDataset {
public:
    NetDataset(std::string PATH, std::string name) {
        filename = name;
        load_result = Load(PATH.c_str());
        dataset = (Net_Tuple*)load_result.start;
        length = load_result.length / sizeof(Net_Tuple);
    }

    ~NetDataset() {
        UnLoad(load_result);
    }

public:
    std::string filename;
    LoadResult load_result;
    Net_Tuple *dataset;
    uint64_t length;
};

#endif