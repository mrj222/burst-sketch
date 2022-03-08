#ifndef _INSIDEBURST_H
#define _INSIDEBURST_H_

#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <cmath>

#include "BurstSketch.h"
#include "DynamicBurstSketch.h"
#include "Param.h"
#include "Burst.h"

#define NEXT(x) ((x) + 1) % (win_cold_thres + 2)
#define PREV(x) ((x) + win_cold_thres + 1) % (win_cold_thres + 2)

class TimeStampStack {
public:
    TimeStampStack(){
        begin = 0;
        end = 0;
        memset(stamp_stack, 0, sizeof(stamp_stack));
    }
    void push(uint32_t cnt) {
        assert(NEXT(end) != begin);
        stamp_stack[end] = cnt;
        end = NEXT(end);
    }
    void pop() {
        assert(begin != end);
        stamp_stack[end] = 0;
        end = PREV(end);
    }
    void check(uint32_t cnt) {
        while (end != begin && cnt - stamp_stack[begin] > win_cold_thres) {
            stamp_stack[begin] = 0;
            begin = NEXT(begin);
        }
    }
    void clear() {
        begin = 0;
        end = 0;
        memset(stamp_stack, 0, sizeof(stamp_stack));
    }
    uint32_t top() {
        assert(begin != end);
        return stamp_stack[PREV(end)];
    
    }
    bool empty() {
        return begin == end;
    }
    void print() {
        printf("STACK: ");
        for (int i = begin; i != end; i = NEXT(i)) {
            printf("%d ", stamp_stack[i]);
        }
        printf("\n");
    }
private: 
    uint32_t begin, end;
    uint32_t stamp_stack[win_cold_thres + 2];
};

template<typename ID_TYPE,typename DATA_TYPE>
class SlotInside {
public: 
    ID_TYPE id;
	DATA_TYPE counters[2];
    TimeStampStack stampstack;
    SlotInside() {}
    SlotInside(ID_TYPE _id): id(_id) {
        memset(counters, 0, sizeof(counters));
        stampstack.clear();
    }
};

template<typename ID_TYPE, typename DATA_TYPE>
class StageTwoInside {
public: 
	StageTwoInside(int b_num, int b_size, uint32_t s1_thres, uint32_t s2_thres) : flag(0), bucket_num(b_num), 
     bucket_size(b_size), stage_one_thres(s1_thres), stage_two_thres(s2_thres) {
        slots = new SlotInside<ID_TYPE, DATA_TYPE>*[bucket_num];
        for (int i = 0; i < bucket_num; ++i) {
            slots[i] = new SlotInside<ID_TYPE, DATA_TYPE>[bucket_size];
            memset(slots[i], 0, sizeof(SlotInside<ID_TYPE, DATA_TYPE>) * bucket_size);
        }
    }
    ~StageTwoInside() {
        for (int i = 0; i < bucket_num; ++i) {
            delete [] slots[i];
        }
        delete [] slots;
    }
    bool lookup(ID_TYPE id) {
        int bucket_pos = hash(id, 22) % bucket_num;
        for (int i = 0; i < bucket_size; ++i) {
            if (slots[bucket_pos][i].id == id) {
                ++slots[bucket_pos][i].counters[flag];
                return true;
            }
        }
        return false;
    }
    bool insert(ID_TYPE id, int32_t window, DATA_TYPE count) {
        int bucket_pos = hash(id, 22) % bucket_num;
        int slot_pos = 0;
        DATA_TYPE min_counter = std::numeric_limits<DATA_TYPE>::max();
        bool non_bursting = false;
        // search bucket for a suitable slot
        // id is guaranteed not to be in the stage 2
        for (int i = 0; i < bucket_size; ++i) {
            if (slots[bucket_pos][i].id == 0) { // empty
                slot_pos = i;
                break;
            }
            if (slots[bucket_pos][i].stampstack.empty()) {
                if (!non_bursting) {
                    non_bursting = true;
                    min_counter = std::numeric_limits<DATA_TYPE>::max();
                }
                if (slots[bucket_pos][i].counters[flag] < min_counter) {
                    min_counter = slots[bucket_pos][i].counters[flag];
                    slot_pos = i;
                }
            }
            else if (!non_bursting) {
                if (slots[bucket_pos][i].counters[flag] < min_counter) {
                    min_counter = slots[bucket_pos][i].counters[flag];
                    slot_pos = i;
                }
            }
        }

        if (slots[bucket_pos][slot_pos].id == 0 ||
         count > slots[bucket_pos][slot_pos].counters[flag]) { // empty or evict
            slots[bucket_pos][slot_pos].id = id;
            slots[bucket_pos][slot_pos].stampstack.clear();
            slots[bucket_pos][slot_pos].counters[flag] = count;
            slots[bucket_pos][slot_pos].counters[flag ^ 1] = 0;
            return true;
        }
        
        return false;
    }


    void window_transition(uint32_t window) {
        // clear
        for (int i = 0; i < bucket_num; ++i) {
            for (int j = 0; j < bucket_size; ++j) {
                if (slots[i][j].id != 0 && slots[i][j].counters[0] < stage_one_thres &&
                  slots[i][j].counters[1] < stage_one_thres) {
                    memset(&slots[i][j], 0, sizeof(SlotInside<ID_TYPE, DATA_TYPE>));
                    slots[i][j].stampstack.clear();
                }
                slots[i][j].stampstack.check(window);
            }
        }
        
        // find burst
        for (int i = 0; i < bucket_num; ++i) {
            for (int j = 0; j < bucket_size; ++j) {
                if (slots[i][j].id == 0) 
                    continue;
                if (!slots[i][j].stampstack.empty() && slots[i][j].counters[flag] <= slots[i][j].counters[flag ^ 1] / lambda) {
                    // slots[i][j].stampstack.print();
                    results.emplace_back(Burst<ID_TYPE>(slots[i][j].stampstack.top(), window, slots[i][j].id));
                    slots[i][j].stampstack.pop();
                }
                else if (slots[i][j].counters[flag] < stage_two_thres) {
                    slots[i][j].stampstack.clear();
                }
                else if (slots[i][j].counters[flag] >= lambda * slots[i][j].counters[flag ^ 1] && slots[i][j].counters[flag] >= stage_two_thres) {
                    slots[i][j].stampstack.push(window);
                }
            }
        }

        flag ^= 1;
        for (int i = 0; i < bucket_num; ++i)
            for (int j = 0; j < bucket_size; ++j)
                slots[i][j].counters[flag] = 0;
    }

protected: 
	
	uint32_t stage_one_thres, stage_two_thres;
	int bucket_num, bucket_size;
    uint8_t flag;
	SlotInside<ID_TYPE, DATA_TYPE> **slots;
public:
    std::vector<Burst<ID_TYPE>> results;
};


template<typename ID_TYPE, typename DATA_TYPE>
class BurstSketchInsideAbstract : public Abstract <ID_TYPE> {
public: 
    BurstSketchInsideAbstract() : last_timestamp(0), win_cnt(0), stage1(nullptr), stage2(nullptr) {}
     
    virtual ~BurstSketchInsideAbstract() {
		if (stage1 != nullptr)
			delete stage1;
		if (stage2 != nullptr)
			delete stage2;
	}

    void insert(ID_TYPE id, uint64_t timestamp) {
        DATA_TYPE count;
        while (last_timestamp + window_size < timestamp) {
            // transition to next window
            last_timestamp += window_size;
            stage1->clearAll();
            stage2->window_transition(win_cnt++);
        }
        if (stage2->lookup(id)) {
            return;
        }
        count = stage1->insert(id);
        if (count != 0 && stage2->insert(id, timestamp, count)) {
            stage1->clearID(id);
        }
    }
    std::vector<Burst<ID_TYPE>> query() {
        stage1->clearAll();
        stage2->window_transition(win_cnt++);
        return stage2->results;
    }
protected: 
	uint32_t last_timestamp;
	uint32_t win_cnt;
	StageOne<ID_TYPE, DATA_TYPE> *stage1;
	StageTwoInside<ID_TYPE, DATA_TYPE> *stage2;
};

template<typename ID_TYPE, typename DATA_TYPE>
class BurstSketchInside : public BurstSketchInsideAbstract <ID_TYPE, DATA_TYPE> {
public: 
    BurstSketchInside(int mem, double stage_ratio, uint32_t stage1_thres) {
        int stage1_mem = mem * 1024 * stage_ratio / (1 + stage_ratio);
		int stage2_mem = mem * 1024 / (1 + stage_ratio);
		int stage1_num = hash_num;
		int stage1_len = stage1_mem / stage1_num / (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
		int stage2_size = stage2_bucket_size;
		int stage2_num = stage2_mem / stage2_size / (sizeof(int32_t) + sizeof (ID_TYPE) + 2 *
		sizeof(DATA_TYPE));

        BurstSketchInsideAbstract<ID_TYPE, DATA_TYPE>::stage1 = new SimpleStageOne<ID_TYPE,
		DATA_TYPE>(stage1_num, stage1_len, stage1_thres);
		BurstSketchInsideAbstract<ID_TYPE, DATA_TYPE>::stage2 = new StageTwoInside<ID_TYPE,
		DATA_TYPE>(stage2_num, stage2_size, stage1_thres, BurstThreshold);
    }
    ~BurstSketchInside() {}
};

template<typename ID_TYPE, typename DATA_TYPE>
class DynamicBurstSketchInside: public BurstSketchInsideAbstract <ID_TYPE, DATA_TYPE> {
public: 
    DynamicBurstSketchInside(int mem, double stage_ratio, uint32_t s1_thres) {
        int stage1_mem = mem * 1024 * stage_ratio / (1 + stage_ratio);
        int stage2_mem = mem * 1024 / (1 + stage_ratio);
        int stage1_num = hash_num;
        int stage1_len = stage1_mem / stage1_num * 8 / (dynamic_bit + 1);
        int stage2_size = stage2_bucket_size;
        int stage2_num = stage2_mem / stage2_size / (sizeof(int32_t) + sizeof(ID_TYPE) + 
         2 * sizeof(DATA_TYPE));

        BurstSketchInsideAbstract<ID_TYPE, DATA_TYPE>::stage1 = new DynamicStageOne<ID_TYPE,
         DATA_TYPE>(stage1_num, stage1_len, s1_thres);
        BurstSketchInsideAbstract<ID_TYPE, DATA_TYPE>::stage2 = new StageTwoInside<ID_TYPE, DATA_TYPE>
         (stage2_num, stage2_size, s1_thres, BurstThreshold);
    }
    ~DynamicBurstSketchInside() {}
};

template<typename ID_TYPE>
class CorrectDetectorInside : public Abstract <ID_TYPE> {
public: 
    CorrectDetectorInside(uint32_t thres) : last_timestamp(0), win_cnt(0), threshold(thres), flag(0) {}

	~CorrectDetectorInside() {}
    void transition(uint32_t window) { // is the logic here wrong?
        for (auto &i : detect) {
            i.stampstack.check(window);
            if (i.counters[flag] * lambda <= i.counters[flag ^ 1] && !i.stampstack.empty()) {
                output.emplace_back(Burst<ID_TYPE>(i.stampstack.top(), window, i.id));
                i.stampstack.pop();
            }
            if (i.counters[flag] < threshold) {
                i.stampstack.clear();
            }
            if (i.counters[flag] >= lambda * i.counters[flag ^ 1] && i.counters[flag] >= threshold) {
                i.stampstack.push(window);
            }
        }
        flag ^= 1;
        for (auto &i : detect) {
            i.counters[flag] = 0;
        }
    }
    void insert (ID_TYPE id, uint64_t timestamp) {
        while (last_timestamp + window_size < timestamp) {
            transition(win_cnt++);
            last_timestamp += window_size;
        }
        if (id_index.find(id) == id_index.end()) {
            id_index[id] = detect.size();
            detect.emplace_back(SlotInside<ID_TYPE, uint32_t>(id));
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
    uint32_t last_timestamp;
    uint32_t win_cnt;
    uint32_t threshold;
    std::map<ID_TYPE, int> id_index;
    std::vector<SlotInside<ID_TYPE, uint32_t>> detect;
public:
    std::vector<Burst<ID_TYPE>> output;
};

#endif