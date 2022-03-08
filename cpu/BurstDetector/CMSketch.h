#ifndef _CMSKETCH_
#define _CMSKETCH_ 

#include "BurstSketchAbstract.h"
#include "Param.h"
#include "Stage.h"
#include "Bitmap.h"
#include <queue>
#include <cmath>
#include <string.h>

#define CM_hash_num 2
#define GETNOW(x) ((x) % (win_cold_thres + 2))

template<typename ID_TYPE, typename DATA_TYPE>
class CMSketch {
public: 
	uint32_t size;
	uint32_t cm_id;
	DATA_TYPE* CMArray;
	
	CMSketch() {}
	CMSketch(uint32_t s, uint32_t cnt):size(s), cm_id(cnt){
		CMArray = new DATA_TYPE[s];
		memset(CMArray, 0, size * sizeof(DATA_TYPE));
	}
	~CMSketch() {
		delete[] CMArray;
	}
	void insert(ID_TYPE id) {
		uint32_t index = hash(id, cm_id) % size;
		CMArray[index]++;
	}
	void clear() {
		memset(CMArray, 0, size * sizeof(DATA_TYPE));
	}
	void erase(ID_TYPE id) {
		uint32_t index = hash(id, cm_id) % size;
		CMArray[index] = 0;
	}
	DATA_TYPE query(ID_TYPE id) {
		uint32_t index = hash(id, cm_id) % size;
		return CMArray[index];
	}
};

template<typename ID_TYPE, typename DATA_TYPE>
class CMSketchDetector : public Abstract<ID_TYPE> {
public:
	uint32_t size;
	uint64_t last_timestamp;
	uint32_t win_cnt;
	uint32_t thres;
	uint32_t array_num;
	CMSketch<ID_TYPE, DATA_TYPE>** CM[win_cold_thres + 2];
	std::queue<std::pair<ID_TYPE, uint32_t> > Q;
	std::vector<Burst<ID_TYPE>> results;
	CMSketchDetector(int mem, uint32_t t):last_timestamp(0), array_num(CM_hash_num), win_cnt(0), thres(t) {
		size = mem * 1024 / (win_cold_thres + 2) / sizeof(DATA_TYPE) / array_num;
		for (int i = 0; i < array_num; ++i) {
			CM[i] = new CMSketch<ID_TYPE, DATA_TYPE>* [win_cold_thres + 2];
			for (int j = 0; j < win_cold_thres + 2; ++j) {
				CM[i][j] = new CMSketch<ID_TYPE, DATA_TYPE>(size, i);
			}
		}
		
		results.clear();
	}
	~CMSketchDetector() {
		for (int i = 0; i < array_num; ++i) {
			for (int j = 0; j < win_cold_thres + 2; ++j) {
				delete CM[i][j];
			}
			delete[] CM[i];
		}
	}
	void update() {
		win_cnt++;
		for (int i = 0; i < array_num; ++i) {
			CM[i][GETNOW(win_cnt)]->clear();
		}
		last_timestamp += window_size;
	}
	DATA_TYPE get(ID_TYPE id, int window) {
		DATA_TYPE ret_value;
		for (int i = 0; i < array_num; ++i) {
			if (i == 0)
				ret_value = CM[i][GETNOW(window)]->query(id);
			else {
				DATA_TYPE temp = CM[i][GETNOW(window)]->query(id);
				ret_value = ret_value < temp ? ret_value : temp;
			}
		}
		return ret_value;
	}
	void detect() {
		DATA_TYPE cnt[win_cold_thres + 2];
		while (!Q.empty()) {
			ID_TYPE id = Q.front().first;
			uint32_t window = Q.front().second;
			if (win_cnt - window < win_cold_thres + 1)
				break;
			Q.pop();
			
			for (int i = 0; i < win_cold_thres + 2; i++) {
				cnt[i] = get(id, window + i);
			}
			if (cnt[1] < cnt[0] * lambda || cnt[1] < thres)
				continue;
			for (int i = 2; i < win_cold_thres + 2; i++)
			{
				if (cnt[i] >= cnt[i - 1] * lambda)
					break;
				if (lambda * cnt[i] <= cnt[i - 1]) {
					results.push_back(Burst(window + 1, window + i, id));
					break;
				}
				if (cnt[i] < thres)
					break;
			}	
		}
	}
	void insert(ID_TYPE id, uint64_t timestamp) {
		while (last_timestamp + window_size < timestamp) {
			detect();
			update();
        }
		for (int i = 0; i < array_num; ++i) {
			CM[i][GETNOW(win_cnt)]->insert(id);
		}
		DATA_TYPE ret = get(id, win_cnt);
		if (ret == thres && win_cnt > 0) 
			Q.push(std::pair<ID_TYPE, uint32_t>(id, win_cnt - 1));
	}
	std::vector<Burst<ID_TYPE>> query() {
		detect();
		update();
        return results;
    }
};

#endif