#ifndef _CORRECTDETECTOR_H_
#define _CORRECTDETECTOR_H_

#include "Abstract.h"
#include "Burst.h"
#include "Param.h"

#include <map>
#include <vector>
#include <assert.h>

template<typename ID_TYPE>
struct CorrectDetectItem {
    uint32_t counters[2];
    int32_t window;
    ID_TYPE id;

    CorrectDetectItem() {}
    CorrectDetectItem(ID_TYPE _id) : id(_id) {
        counters[0] = counters[1] = 0;
        window = -1;
    }
    CorrectDetectItem(const CorrectDetectItem &item) {
        counters[0] = item.counters[0];
        counters[1] = item.counters[1];
        window = item.window;
        id = item.id;
    }
};

template<typename ID_TYPE>
class CorrectDetector : public Abstract<ID_TYPE> {

public:
    CorrectDetector(uint32_t thres) : last_timestamp(0), win_cnt(0), threshold(thres), flag(0) {
        if (!count_based) {
            last_timestamp = start_time;
        }
    }

	~CorrectDetector() {}

    void transition(uint32_t window) { // is the logic here wrong?
        for (auto &i : detect) {
            if (i.window != -1 && window - i.window > win_cold_thres) {
                i.window = -1;
            }
            if (i.counters[flag] * lambda <= i.counters[flag ^ 1] && i.window != -1) {
                output.emplace_back(Burst<ID_TYPE>(i.window, window, i.id));
                // std::cout << "ID: " << i.id << " start: " << i.window << " end: " << window << std::endl;
                i.window = -1;
            }
            if (i.counters[flag] < threshold) {
                i.window = -1;
            }
            if (i.counters[flag] >= lambda * i.counters[flag ^ 1] && i.counters[flag] >= threshold) {
                i.window = window;
            }
        }
        flag ^= 1;
        for (auto &i : detect) {
            i.counters[flag] = 0;
        }
    }

    void insert (ID_TYPE id, uint64_t timestamp) {
        if (count_based) {
            while (last_timestamp + window_size < timestamp) {
                transition(win_cnt++);
                last_timestamp += window_size;
            }
        }
        else {
            while (last_timestamp + window_time < timestamp) {
                transition(win_cnt++);
                last_timestamp += window_time;
            }
        }
        if (id_index.find(id) == id_index.end()) {
            id_index[id] = detect.size();
            detect.emplace_back(CorrectDetectItem<ID_TYPE>(id));
        }
		assert(id == detect[id_index[id]].id);
        detect[id_index[id]].counters[flag]++;
    }

    std::vector<Burst<ID_TYPE>> query() {
        transition(win_cnt++);
        return output;
    }

private:
    uint8_t flag;
    uint64_t last_timestamp;
	uint32_t win_cnt;
    uint32_t threshold;
    std::map<ID_TYPE, int> id_index;
    std::vector<CorrectDetectItem<ID_TYPE>> detect;
public:
    std::vector<Burst<ID_TYPE>> output;
    
};

#endif
