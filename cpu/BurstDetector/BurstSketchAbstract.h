#ifndef _BURSTSKETCHABSTRACT_H_
#define _BURSTSKETCHABSTRACT_H_

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include "Abstract.h"
#include "Stage.h"

template<typename ID_TYPE, typename DATA_TYPE>
class BurstSketchAbstract : public Abstract<ID_TYPE> {
public:
    BurstSketchAbstract(bool analyze = false) : win_cnt(0), stage1(nullptr), stage2(nullptr), analysis(analyze) {
        if (!count_based) {
            last_timestamp = start_time;
        }
        else {
            last_timestamp = 0;
        }
    }
     
    virtual ~BurstSketchAbstract() {
		if (stage1 != nullptr)
			delete stage1;
		if (stage2 != nullptr)
			delete stage2;
	}

    void insert(ID_TYPE id, uint64_t timestamp) {
        // if count-based window, the second parameter is counter
        // else, the second parameter is the timestamp
        DATA_TYPE count;
        if (count_based) {
            while (last_timestamp + window_size < timestamp) {
                // transition to next window
                last_timestamp += window_size;
                stage1->clearAll();
                stage2->window_transition(win_cnt++);
            }
        }
        else {
            while (last_timestamp + window_time < timestamp) {
                // transition to next window
                last_timestamp += window_time;
                stage1->clearAll();
                stage2->window_transition(win_cnt++);
            }
        }
        if (stage2->lookup(id)) {
            return;
        }
        count = stage1->insert(id);
        if (count != 0 && stage2->insert(id, timestamp, count)) {
            stage1->clearID(id);
            if (analysis)
                passed.insert(id);
        }
    }

    std::vector<Burst<ID_TYPE>> query() {
        stage1->clearAll();
        stage2->window_transition(win_cnt++);
        return stage2->results;
    }

    std::set<ID_TYPE> get_passed_IDs() {
        return passed;
    }

protected:
    uint64_t last_timestamp;
	uint32_t win_cnt;
    StageOne<ID_TYPE, DATA_TYPE> *stage1;
    StageTwo<ID_TYPE, DATA_TYPE> *stage2;

    // record IDs that passed through stage one
    bool analysis;
    std::set<ID_TYPE> passed;
};

#endif
