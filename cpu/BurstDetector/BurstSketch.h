#ifndef _BURSTSKETCH_H_
#define _BURSTSKETCH_H_

#include <cstring>
#include "BurstSketchAbstract.h"
#include "Stage.h"
#include "hash.h"
#include "Param.h"

template<typename ID_TYPE, typename DATA_TYPE>
class SimpleStageOne : public StageOne<ID_TYPE, DATA_TYPE> {
public:
	SimpleStageOne(int num, int len, uint32_t thres) : array_num(num), array_len(len),
	 s1_thres(thres) {
		ids = new ID_TYPE*[array_num];
		counters = new DATA_TYPE*[array_num];
		for (int i = 0; i < array_num; ++i) {
			ids[i] = new ID_TYPE[array_len];
			counters[i] = new DATA_TYPE[array_len];
			memset(ids[i], 0, sizeof(ID_TYPE) * array_len);
			memset(counters[i], 0, sizeof(DATA_TYPE) * array_len);
		}
	}
	
	~SimpleStageOne() {
		for (int i = 0; i < array_num; ++i) {
			delete [] ids[i];
			delete [] counters[i];
		}
		delete [] ids;
		delete [] counters;
	}
		
	void clearAll() {
		for (int i = 0; i < array_num; ++i) {
			memset(ids[i], 0, sizeof(ID_TYPE) * array_len);
			memset(counters[i], 0, sizeof(DATA_TYPE) * array_len);
		}
	}

	void clearID(ID_TYPE id) {
		for (int i = 0; i < array_num; ++i) {
			uint32_t index = hash(id, i + 20) % array_len;
			if (ids[i][index] == id) {
				ids[i][index] = 0;
				counters[i][index] = 0;
			}
		}
	}

	DATA_TYPE insert(ID_TYPE id) {
		DATA_TYPE ret = 0;
		for (int i = 0; i < array_num; ++i) {
			uint32_t index = hash(id, i + 20) % array_len;
			if (ids[i][index] == id) {
				if (++counters[i][index] > s1_thres)
					ret = counters[i][index];
			}
			else if (counters[i][index] == 0) {
				++counters[i][index];
				ids[i][index] = id;
			}
			else {
				if (replace_strategy == FREQUENT)
					--counters[i][index];
				if (replace_strategy == PROB_DECAY) {
					if (!(rand() % int(pow(1.08, counters[i][index]))))
						--counters[i][index];
				}
				if (replace_strategy == PROB_REPLACE) {
					if (!(rand() % (counters[i][index] + 1))) {
						ids[i][index] = id;
						ret = (++counters[i][index]);
					}
				}
			}
		}
		return ret;
	}
			
private:
	int array_num, array_len;
	uint32_t s1_thres;
	ID_TYPE **ids;
	DATA_TYPE **counters;
};


template<typename ID_TYPE, typename DATA_TYPE>
class BurstSketch : public BurstSketchAbstract<ID_TYPE, DATA_TYPE> {
public:
	BurstSketch(int mem, double stage_ratio, uint32_t s1_thres, bool analyze=false) :
    BurstSketchAbstract<ID_TYPE, DATA_TYPE>(analyze) {
		int stage1_mem = mem * 1024 * stage_ratio / (1 + stage_ratio);
		int stage2_mem = mem * 1024 / (1 + stage_ratio);
		int stage1_num = hash_num;
		int stage1_len = stage1_mem / stage1_num / (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
		int stage2_size = stage2_bucket_size;
		int stage2_num = stage2_mem / stage2_size / (sizeof(int32_t) + sizeof (ID_TYPE) + 2 *
		sizeof(DATA_TYPE));
		// printf("%d %d\n", stage1_len, stage2_num);
		// printf("BurstSketch: %d %d\n", stage1_len, stage2_num);
		BurstSketchAbstract<ID_TYPE, DATA_TYPE>::stage1 = new SimpleStageOne<ID_TYPE,
		DATA_TYPE>(stage1_num, stage1_len, s1_thres);
		BurstSketchAbstract<ID_TYPE, DATA_TYPE>::stage2 = new StageTwo<ID_TYPE,
		DATA_TYPE>(stage2_num, stage2_size, s1_thres, BurstThreshold);
	}

	~BurstSketch() {}

};

#endif

