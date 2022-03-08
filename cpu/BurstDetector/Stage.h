#ifndef _STAGE_H_
#define _STAGE_H_

#include <bitset>
#include <vector>
#include <climits>
#include "Burst.h"
#include "Abstract.h"
#include "hash.h"
#include "Param.h"

template<typename ID_TYPE, typename DATA_TYPE>
class StageOne {
public:
    StageOne() {}
    virtual ~StageOne() {}

    virtual void clearAll() = 0;

    virtual void clearID(ID_TYPE id) = 0;

    // return 0 if fail, mininal counter otherwise
    virtual DATA_TYPE insert(ID_TYPE id) = 0;
};


template<typename ID_TYPE, typename DATA_TYPE>
class StageTwo {
public:
    StageTwo(int b_num, int b_size, uint32_t s1_thres, uint32_t s2_thres) : flag(0), bucket_num(b_num), 
     bucket_size(b_size), stage_one_thres(s1_thres), stage_two_thres(s2_thres) {
        slots = new Slot*[bucket_num];
        for (int i = 0; i < bucket_num; ++i) {
            slots[i] = new Slot[bucket_size];
            memset(slots[i], 0, sizeof(Slot) * bucket_size);
            for (int j = 0; j < bucket_size; ++j) {
                slots[i][j].window = -1;
            }
        }
    }

    ~StageTwo() {
        for (int i = 0; i < bucket_num; ++i) {
            delete [] slots[i];
        }
        delete [] slots;
    }

    bool lookup(ID_TYPE id) {
        int bucket_pos = hash(id, 32) % bucket_num;
        for (int i = 0; i < bucket_size; ++i) {
            if (slots[bucket_pos][i].id == id) {
                ++slots[bucket_pos][i].counters[flag];
                return true;
            }
        }
        return false;
    }

    bool insert(ID_TYPE id, int32_t window, DATA_TYPE count) {
        int bucket_pos = hash(id, 32) % bucket_num;
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
            if (slots[bucket_pos][i].window == -1) {
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
            slots[bucket_pos][slot_pos].window = -1;
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
                if (slots[i][j].id != 0 && slots[i][j].window != -1 &&
                  window - slots[i][j].window > win_cold_thres) {
                    // memset(&slots[i][j], 0, sizeof(Slot));
                    slots[i][j].window = -1;
                }
                else if (slots[i][j].id != 0 && slots[i][j].counters[0] < stage_one_thres &&
                  slots[i][j].counters[1] < stage_one_thres) {
                    memset(&slots[i][j], 0, sizeof(Slot));
                    slots[i][j].window = -1;
                }
            }
        }
        
        // find burst
        for (int i = 0; i < bucket_num; ++i) {
            for (int j = 0; j < bucket_size; ++j) {
                if (slots[i][j].id == 0) 
                    continue;
                if (slots[i][j].window != -1 && slots[i][j].counters[flag] <= slots[i][j].counters[flag ^ 1] / lambda) {
                    results.emplace_back(Burst<ID_TYPE>(slots[i][j].window, window, slots[i][j].id));
                    // memset(&slots[i][j], 0, sizeof(Slot));
                    slots[i][j].window = -1;
                }
                else if (slots[i][j].counters[flag] < stage_two_thres) {
                    slots[i][j].window = -1;
                }
                else if (slots[i][j].counters[flag] >= lambda * slots[i][j].counters[flag ^ 1] && slots[i][j].counters[flag] >= stage_two_thres) {
                    slots[i][j].window = window;
                }
            }
        }

        flag ^= 1;
        for (int i = 0; i < bucket_num; ++i)
            for (int j = 0; j < bucket_size; ++j)
                slots[i][j].counters[flag] = 0;
    }

private:
    struct Slot {
        ID_TYPE id;
        DATA_TYPE counters[2];
        int32_t window;
    };

    uint32_t stage_one_thres, stage_two_thres;
    int bucket_num, bucket_size;
    uint8_t flag;
    Slot **slots;
    
public:
    std::vector<Burst<ID_TYPE>> results;
};

#endif
