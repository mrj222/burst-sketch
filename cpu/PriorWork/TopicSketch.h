#ifndef _TOPIC_H_
#define _TOPIC_H_

#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <cmath>
#include "Abstract.h"
#include "Param.h"

class Velocity {
public:
	Velocity(uint32_t _deltaT) {
		deltaT = _deltaT;
		time = 0;
		v = 0.0;
	}
	void insert(uint32_t t) {
		v = v * exp(1.0 * ((int) time - (int)t) / deltaT) + 1.0 / deltaT;
		time = t;
	}
	void update(uint32_t t) {
		v = v * exp(1.0 * ((int) time - (int) t) / deltaT);
		time = t;
	}
	double query(uint64_t t) {
		update(t);
		return v;
	}
private: 
	uint32_t time;
	double v;
	uint32_t deltaT;
};

template<typename ID_TYPE> 
class TopicSketch: public Abstract<ID_TYPE> {
public:
	TopicSketch(int _d, int _size, uint32_t _deltaT1, uint32_t _deltaT2): d(_d), deltaT1(_deltaT1), deltaT2(_deltaT2), last_timestamp(0), win_cnt(0) {
		size = _size * 1024 / 16 / 8;
		c1 = (Velocity ***)new Velocity**[d];
		c2 = (Velocity ***)new Velocity**[d];
		for(int i = 0; i < d; i++) {
			c1[i] = new Velocity*[size];
			c2[i] = new Velocity*[size];
		}
		for(int i = 0; i < d; i++)
			for(int j = 0; j < size; j++) {
				c1[i][j] = new Velocity(deltaT1);
				c2[i][j] = new Velocity(deltaT2);
			}
	}
	void insert(ID_TYPE id, uint64_t time) {
		if (time > last_timestamp + window_size) {
			last_timestamp += window_size;
			win_cnt++;
			report_id.clear();
		}
		for(int i = 0; i < d; i++) {
			int z = hash(id, i) % size;
			c1[i][z]->insert(time);
			c2[i][z]->insert(time);
		}
		// finish inserting, start query
		double v1 = 1000000.0, v2 = 1000000.0;
		for(int i = 0; i < d; i++) {
			int z = hash(id, i) % size;
			v1 = MIN(v1, c1[i][z]->query(time));
			v2 = MIN(v2, c2[i][z]->query(time));
		}
		if ((v2 - v1) / (deltaT1 - deltaT2) >= 0.00000015 && report_id.find(id) == report_id.end()) {
			assert(id);
			report_id.insert(id);
			results.emplace_back(Burst<ID_TYPE>(win_cnt, 0, id));
		}
		
	}
	std::vector<Burst<ID_TYPE>> query() {
        return results;
    }

private: 
	int d, size;
	uint32_t deltaT1, deltaT2;
	Velocity*** c1;
	Velocity*** c2;
	uint64_t last_timestamp;
	uint32_t win_cnt;
	std::set<ID_TYPE> report_id;
	std::vector<Burst<ID_TYPE>> results;
};


#endif