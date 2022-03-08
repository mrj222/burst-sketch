#ifndef _CM_PBE_1_H_
#define _CM_PBE_1_H_

#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <cmath>
#include "Abstract.h"
#include "Param.h"
#include "Stair.h"


template<typename ID_TYPE>
class CM_PBE1 : public Abstract<ID_TYPE> {
public: 
	CM_PBE1(uint32_t memory): d(8), last_timestamp(0), win_cnt(0) {
		w = memory * 1024 / sizeof(uint32_t) / d;
		// pla.resize(d);
		stair = new Staircase* [d];
		C = new uint32_t* [d];
		for (int i = 0; i < d; ++i) {
			C[i] = new uint32_t [w];
			memset(C[i], 0, w * sizeof(uint32_t));
			stair[i] = new Staircase [w];
			for (int j = 0; j < w; ++j) {
				stair[i][j].set_param(40, 250000);
			}
			// stair[i].resize(w, nullptr);
		}
		// std::cout << "initialize success\n";
	}
	~CM_PBE1() {
		for (int i = 0; i < d; ++i) {
			delete[] C[i];
		}
		delete[] C;
	}
	void insert(ID_TYPE id, uint64_t time) {
		if (last_timestamp + window_size < time) {
			last_timestamp += window_size;
			win_cnt++;
			report_id.clear();
		}
		uint32_t min_cnt = 2147483647;
		for (int i = 0; i < d; ++i) {
			uint32_t z = hash(id, i + 20) % w;
			min_cnt = MIN(min_cnt, ++C[i][z]);
			stair[i][z].feed({time, (double)C[i][z]});
		}		
		// start finding burst
		bool report = false;
		double c = estimate(id, time);
		if (time < window_size && c > tol) 
			report = true;
		else {
			double b = estimate(id, time - window_size);
			if (time < 2 * window_size && c - b > tol) 
				report = true;
			else {
				double a = estimate(id, time - 2 * window_size);
				if (c - 2 * b + a > tol)
					report = true;
			}
		}
		if (report && report_id.find(id) == report_id.end()) {
			report_id.insert(id);
			results.emplace_back(Burst<ID_TYPE>(win_cnt, 0, id));
		}
		// std::cout << "No." << time << "insert success\n";
	}
	double estimate(ID_TYPE id, uint64_t time) {
		double vals[d];
    	for (int i = 0; i < d; i++) {
        	uint32_t z = hash(id, i + 20) % w;
        	vals[i] = stair[i][z].estimate(time);
    	}
    	std::sort(vals, vals + d);
    	if (d & 1) { // d is odd
        	return vals[d/2];
    	} else {
        	return (vals[d/2] + vals[(d/2)-1]) / 2;
    	}
	}
	std::vector<Burst<ID_TYPE>> query() {
        return results;
    }
private: 
	uint64_t last_timestamp;
	uint32_t win_cnt;
	uint32_t w, d;
    uint32_t** C;
	Staircase** stair;
	std::set<ID_TYPE> report_id;
    std::vector<Burst<ID_TYPE>> results;
};





#endif