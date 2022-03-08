#ifndef _NETBENCHMARK_H_
#define _NETBENCHMARK_H_

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <cmath>

#include "NetWorkDataset.h"
#include "CorrectDetector.h"
#include "BurstSketch.h"
#include "DynamicBurstSketch.h"
#include "CMSketch.h"
#include "Mmap.h"
#include "Param.h"
#include "Burst.h"



class NetBenchmark {
public:
	NetBenchmark(NetDataset* _net): net(_net) {}
	~NetBenchmark() {}
	void Compare(std::vector<Burst<uint64_t>> &result, std::vector<Burst<uint64_t>> &ground_truth, bool flag) {
		std::multimap<uint64_t, std::pair<uint32_t, uint32_t>> gt;
		for (auto &record : ground_truth) {
			gt.insert(std::make_pair(record.id, std::make_pair(record.start_window, record.end_window)));
		}

		int correct = 0;
		for (auto &r : result) {
			auto begin = gt.lower_bound(r.id);
			auto end = gt.upper_bound(r.id);
			if (flag) {
				while (begin != end) {
					if (begin->second.first == r.start_window) {
						++correct;
						break;
					}
					++begin;
				}
			}
			else {
				while (begin != end) {
					if (begin->second.first == r.start_window && begin->second.second == r.end_window) {
						++correct;
						break;
					}
					else if (begin->second.second >= r.start_window && begin->second.first <= r.end_window) {
						// neglect overlap cases
						++correct;
						break;
					}
					++begin;
				}
			}
		}
        std::cout << "correct: " << correct << " find: " << result.size() << " truth: " << ground_truth.size() << std::endl;
		std::cout << "Precision:            " << std::fixed << std::setprecision(4) << 100.0 * (double)correct / result.size() << std::endl;
		std::cout << "Recall:               " << std::fixed << std::setprecision(4) << 100.0 * (double)correct / ground_truth.size() << std::endl;
        std::cout << "F1:                   " << std::fixed << std::setprecision(4) << 2 * (double)correct / result.size() * (double)correct / ground_truth.size() / ((double)correct / result.size() + (double)correct / ground_truth.size()) << std::endl;

	}
    
    void Run(int memory, int flag) {
        uint32_t running_track_thres = BurstThreshold * running_track_thres_ratio;
        uint32_t run_length = 20000000;
        if (!count_based)
            start_time = net->dataset[0].timestamp;
        Abstract<uint64_t> *correct_detector = new CorrectDetector<uint64_t>(BurstThreshold);
	    Abstract<uint64_t> *burst_sketch = new BurstSketch<uint64_t, uint32_t>(memory, stage_ratio, running_track_thres);
        Abstract<uint64_t> *dynamic_sketch = new DynamicBurstSketch<uint64_t, uint32_t>(memory, stage_ratio_optimized, running_track_thres);
        Abstract<uint64_t> *cm_sketch = new CMSketchDetector<uint64_t, uint32_t>(memory, BurstThreshold);
        Abstract<uint64_t> *topic_sketch = new TopicSketch<uint64_t>(8, memory, 20000, 10000);
		Abstract<uint64_t> *cm_pbe_1 = new CM_PBE1<uint64_t>(memory);
		Abstract<uint64_t> *cm_pbe_2 = new CM_PBE2<uint64_t>(memory);
        
        // std::cout << net->dataset[0].timestamp << std::endl;
        // std::cout << net->dataset[run_length - 1].timestamp << std::endl;
        for (uint32_t i = 0; i < run_length; ++i) {
            if (count_based)
                correct_detector->insert(net->dataset[i].id, i);
            else 
                correct_detector->insert(net->dataset[i].id, net->dataset[i].timestamp);
        }
        std::vector<Burst<uint64_t>> ground_truth = correct_detector->query();
        
        // run BurstSketch
        if (flag & FLAG_BURST) {
            auto burst_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                if (count_based)
                    burst_sketch->insert(net->dataset[i].id, i);
                else 
                    burst_sketch->insert(net->dataset[i].id, net->dataset[i].timestamp);
            }
            auto burst_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> burst_sketch_result = burst_sketch->query();
            std::chrono::duration<double> burst_span = std::chrono::duration_cast<std::chrono::duration<double>>(burst_end - burst_start);
            std::cout << "Burst Sketch:" << std::endl;
		    Compare(burst_sketch_result, ground_truth, false);
            std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * burst_span.count()) << "\n\n";
            // return tmp;
        }

        // run DynamicBurstSketch
        if (flag & FLAG_DYNAMIC) {
            auto dynamic_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                if (count_based)
                    dynamic_sketch->insert(net->dataset[i].id, i);
                else 
                    dynamic_sketch->insert(net->dataset[i].id, net->dataset[i].timestamp);
            }
            auto dynamic_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> dynamic_sketch_result = dynamic_sketch->query();
            std::chrono::duration<double> dynamic_span = std::chrono::duration_cast<std::chrono::duration<double>>(dynamic_end - dynamic_start);
            std::cout << "Dynamic Burst Sketch:" << std::endl;
            Compare(dynamic_sketch_result, ground_truth, false);
            std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * dynamic_span.count()) << "\n\n";
        }

        // run CMSketch
        if (flag & FLAG_CM) {
            auto cm_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                if (count_based)
                    cm_sketch->insert(net->dataset[i].id, i);
                else 
                    cm_sketch->insert(net->dataset[i].id, net->dataset[i].timestamp);
            }
            auto cm_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> cm_sketch_result = cm_sketch->query();
        
            std::chrono::duration<double> cm_span = std::chrono::duration_cast<std::chrono::duration<double>>(cm_end - cm_start);
            std::cout << "CM Sketch: " << std::endl;
            Compare(cm_sketch_result, ground_truth, false);
            std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * cm_span.count()) << "\n\n";
        }
        
                // run TopicSketch
        if (flag & FLAG_TOPIC) {
            auto topic_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                if (count_based)
                    topic_sketch->insert(net->dataset[i].id, i);
                else 
                    topic_sketch->insert(net->dataset[i].id, net->dataset[i].timestamp);
            }
            auto topic_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> topic_sketch_result = topic_sketch->query();
        
            std::chrono::duration<double> topic_span = std::chrono::duration_cast<std::chrono::duration<double>>(topic_end - topic_start);
            std::cout << "TopicSketch: " << std::endl;
            Compare(topic_sketch_result, ground_truth, true);
            std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * topic_span.count()) << "\n\n";
        }

        if (flag & FLAG_PBE1) {
            auto pbe1_start = std::chrono::steady_clock::now();
		    for (uint32_t i = 0; i < run_length; ++i) {
                cm_pbe_1->insert(net->dataset[i].id, i);
            }
		    auto pbe1_end = std::chrono::steady_clock::now();
		    std::vector<Burst<uint64_t>> pbe1_result = cm_pbe_1->query();
		    std::chrono::duration<double> pbe1_span = std::chrono::duration_cast<std::chrono::duration<double>>(pbe1_end - pbe1_start);
		    std::cout << "CM-PBE1:" << std::endl;
		    Compare(pbe1_result, ground_truth, true);
		    std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * pbe1_span.count()) << "\n\n";
        }

        if (flag & FLAG_PBE2) {
            auto pbe2_start = std::chrono::steady_clock::now();
		    for (uint32_t i = 0; i < run_length; ++i) {
                cm_pbe_2->insert(net->dataset[i].id, i);
            }
		    auto pbe2_end = std::chrono::steady_clock::now();
		    std::vector<Burst<uint64_t>> pbe2_result = cm_pbe_2->query();
		    std::chrono::duration<double> pbe2_span = std::chrono::duration_cast<std::chrono::duration<double>>(pbe2_end - pbe2_start);
		    std::cout << "CM-PBE2:" << std::endl;
		    Compare(pbe2_result, ground_truth, true);
		    std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * pbe2_span.count()) << "\n\n";
        }

        delete correct_detector;
		delete burst_sketch;
        delete dynamic_sketch;
        delete cm_sketch;
        delete topic_sketch;
        delete cm_pbe_1;
        delete cm_pbe_2;
        
    }
    void MemoryTest() {
        // std::cout << "memory test for BurstSketch, MergeSketch, Dynamic Sketch and CMSketch.\n\n";
        ParamInitialize();
        for (auto x : memory_list) {
            std::cout << "memory: " << x << "\n";
            Run(x, FLAG_BURST | FLAG_DYNAMIC | FLAG_CM | FLAG_TOPIC | FLAG_PBE1 | FLAG_PBE2);
        }
    }
private:
    NetDataset* net;	
};


#endif