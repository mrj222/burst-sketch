#ifndef _DYNAMICBURSTSKETCH_H_
#define _DYNAMICBURSTSKETCH_H_

#include "BurstSketchAbstract.h"
#include "Param.h"
#include "Stage.h"
#include "Bitmap.h"
#include <cmath>

// #define FP1 24
// #define FP2 20
// #define CNT1 4
// #define CNT2 8

uint32_t FP1, FP2, CNT1, CNT2;

struct DynamicArray {
    Bitmap offset_flags;
    uint32_t *units;
    int len;

    DynamicArray(int _len) : len(_len), offset_flags(_len) {
        units = new uint32_t [len];
        memset(units, 0, len * sizeof(uint32_t));
    }

    ~DynamicArray() {
        delete [] units;
    }
    
    void clearAll() {
        memset(units, 0, sizeof(uint32_t) * len);
        offset_flags.ClearAll();
    }

    uint32_t insert(uint32_t fingerprint, uint32_t index) {
        if (offset_flags.Get(index)) { // FP2 | CNT2
            fingerprint = fingerprint & ((1 << FP2) - 1);
            uint32_t fp = units[index] >> CNT2;
            uint32_t count = units[index] & ((1 << CNT2) - 1);
            if (count == 0) { // empty
                units[index] = fingerprint << CNT2;
                units[index]++;
                return 1;
            }
            else if ((fingerprint & ((1 << FP2) - 1)) == fp) { // fp match
                if (count != (1 << CNT2) - 1) {
                    units[index]++;
                    count++;
                }
                return count;
            }
            else { // not match
                if (replace_strategy == FREQUENT)
                    --units[index];
                if (replace_strategy == PROB_DECAY) {
                    if (!(rand() % int(pow(1.08, count)))) 
                        --units[index];
                }
                if (replace_strategy == PROB_REPLACE) {
                    if (!(rand() % (count + 1))) {
						units[index] = fingerprint << CNT2;
                        units[index] += count;
                        if (count != (1 << CNT2) - 1) {
                            units[index]++;
                            count++;
                        }
                        return count;
					}
                }
                // if (units[index] == 0)
                //     ClearID(index);
                return 0;
            }
        } else { // FP1 | CNT1
            fingerprint = fingerprint & ((1 << FP1) - 1);
            uint32_t fp = units[index] >> CNT1;
            uint32_t count = units[index] & ((1 << CNT1) - 1);
            if (count == 0) { // empty
                units[index] = fingerprint << CNT1;
                units[index]++;
                return 1;
            }
            else if ((fingerprint & ((1 << FP1) - 1)) == fp) { // fp match
                if (count != (1 << CNT1) - 1) {
                    units[index]++;
                    count++;
                }
                else {
                    offset_flags.Set(index);
                    units[index] = fingerprint << CNT2;
                    units[index] |= (1 << CNT1);
                    count = (1 << CNT1);
                }
                return count;
            }
            else { // not match
                if (replace_strategy == FREQUENT) 
                    units[index]--;
                if (replace_strategy == PROB_DECAY) {
                    if (!(rand() % int(pow(1.08, count)))) 
                        --units[index];
                }
                if (replace_strategy == PROB_REPLACE) {
                    if (!(rand() % (count + 1))) {
						units[index] = fingerprint << CNT1;
                        units[index] += count;
                        if (count != (1 << CNT1) - 1) {
                            units[index]++;
                            count++;
                        }
                        else {
                            offset_flags.Set(index);
                            units[index] = fingerprint << CNT2;
                            units[index] |= (1 << CNT1);
                            count = (1 << CNT1);
                        }
                        return count;
					}
                }
                // if (units[index] == 0)
                //     ClearID(index);
                return 0;
            }
        }     
    }

    void ClearID(uint32_t index) {
        units[index] = 0;
        offset_flags.Clear(index);
    }
};

template<typename ID_TYPE, typename DATA_TYPE>
class DynamicStageOne : public StageOne<ID_TYPE, DATA_TYPE> {
public:
	DynamicStageOne(int num, int len, uint32_t thres) : array_num(num), array_len(len),
	 s1_thres(thres) {
	CNT1 = log2(thres - 1) + 1;
	FP1 = dynamic_bit - CNT1;
	CNT2 = 2 * CNT1;
	FP2 = dynamic_bit - CNT2;
        // printf("FP1: %d  CNT1: %d\nFP2: %d  CNT2: %d\n", FP1, CNT1, FP2, CNT2);
       	arrays = new DynamicArray*[array_num];
        for (int i = 0; i < array_num; ++i) {
            arrays[i] = new DynamicArray(array_len);
        }
     }
	
	~DynamicStageOne() {
        for (int i = 0; i < array_num; ++i)
            delete arrays[i];
        delete [] arrays;
    }

    void clearAll() {
        for (int i = 0; i < array_num; ++i) 
            arrays[i]->clearAll();
    }

    void clearID(ID_TYPE id) {
        // should we check the fingerprint before clear it?
        for (int i = 0; i < array_num; ++i) {
            uint32_t index = hash(id, i + 20) % array_len;
            arrays[i]->ClearID(index);
        }
    }

    DATA_TYPE insert(ID_TYPE id) {
        // uint32_t fingerprint = (uint32_t) id;
        uint32_t fingerprint = hash(id, 33);
        DATA_TYPE min_v = std::numeric_limits<DATA_TYPE>::max(); // should we use max?
        for (int i = 0; i < array_num; ++i) {
            uint32_t index = hash(id, i + 20) % array_len;
            min_v = std::min(min_v, (DATA_TYPE)arrays[i]->insert(fingerprint, index));
        }
        return min_v >= s1_thres ? min_v : 0;
    }

private:
	int array_num, array_len;
	uint32_t s1_thres;
    DynamicArray **arrays;
};

template<typename ID_TYPE, typename DATA_TYPE>
class DynamicBurstSketch : public BurstSketchAbstract<ID_TYPE, DATA_TYPE> {
public:
    DynamicBurstSketch(int mem, double stage_ratio, uint32_t s1_thres, bool analyze=false) :
    BurstSketchAbstract<ID_TYPE, DATA_TYPE>(analyze) {
        int stage1_mem = mem * 1024 * stage_ratio / (1 + stage_ratio);
        int stage2_mem = mem * 1024 / (1 + stage_ratio);
        int stage1_num = hash_num;
        int stage1_len = stage1_mem / stage1_num * 8 / (dynamic_bit + 1);
        int stage2_size = stage2_bucket_size;
        int stage2_num = stage2_mem / stage2_size / (sizeof(int32_t) + sizeof(ID_TYPE) + 
         2 * sizeof(DATA_TYPE));

        // printf("DynamicBurstSketch: %d %d\n", stage1_len, stage2_num);
        BurstSketchAbstract<ID_TYPE, DATA_TYPE>::stage1 = new DynamicStageOne<ID_TYPE,
         DATA_TYPE>(stage1_num, stage1_len, s1_thres);
        BurstSketchAbstract<ID_TYPE, DATA_TYPE>::stage2 = new StageTwo<ID_TYPE, DATA_TYPE>
         (stage2_num, stage2_size, s1_thres, BurstThreshold);
    }

    ~DynamicBurstSketch() {}

};

#endif
