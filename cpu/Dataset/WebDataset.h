#ifndef _WEBDATASET_H_
#define _WEBDATASET_H_

#include <cstdint>
#include <string>
#include "Mmap.h"

struct Web_Tuple {	
	uint32_t id;
	uint32_t timestamp;
};

class WebDataset {
public:
    WebDataset(std::string PATH, std::string name) {
        filename = name;
        load_result = Load(PATH.c_str());
        dataset = (Web_Tuple*)load_result.start;
        length = load_result.length / sizeof(Web_Tuple);
    }

    ~WebDataset() {
        UnLoad(load_result);
    }

public:
    std::string filename;
    LoadResult load_result;
    Web_Tuple *dataset;
    uint64_t length;
};

#endif