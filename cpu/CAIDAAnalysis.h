#ifndef _CAIDAANALYSIS_H_
#define _CAIDAANALYSIS_H_

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <set>
#include <cmath>

#include "BurstSketch.h"
#include "DynamicBurstSketch.h"
#include "CorrectDetector.h"
#include "Mmap.h"
#include "Param.h"
#include "Burst.h"
#include "CAIDADataset.h"

uint32_t size_array[] = {
    10000, 20000, 30000, 40000, 
    50000, 60000, 70000, 80000
};

class CAIDAAnalysis {
public:
    CAIDAAnalysis (CAIDADataset *_caida) : caida(_caida) {}
    ~CAIDAAnalysis() {}
    double Compare(std::vector<Burst<uint64_t>> &result, std::vector<Burst<uint64_t>> &ground_truth) {
		std::multimap<uint64_t, std::pair<uint32_t, uint32_t>> gt;
		for (auto &record : ground_truth) {
			gt.insert(std::make_pair(record.id, std::make_pair(record.start_window, record.end_window)));
		}

		int correct = 0, overlap = 0, error = 0;
		for (auto &r : result) {
			auto begin = gt.lower_bound(r.id);
			auto end = gt.upper_bound(r.id);
            int tmperror = 2147483647;
			while (begin != end) {
				if (begin->second.first == r.start_window && begin->second.second == r.end_window) {
					++correct;
					break;
				}
				else if (r.start_window <= begin->second.second && r.end_window >= begin->second.first) {        
					// ++overlap;
                    // printf("WRONG: start: %d(%d) -> end: %d(%d)\n", r.start_window, begin->second.first, r.end_window, begin->second.second);
					tmperror = MIN(tmperror, abs((begin->second.second - begin->second.first) - ((int)r.end_window - (int)r.start_window)));
				}
				++begin;
			}
            if (begin == end && tmperror != 2147483647) {
                ++overlap;
                error += tmperror;
            }
		}
        // std::cout << "correct: " << correct << " overlap: " << overlap << " result: " << result.size() << " truth: " << ground_truth.size() << std::endl;
        // std::cout << "Find " << correct << " bursts\n";
		std::cout << "Precision:            " << std::fixed << std::setprecision(4) << 100.0 * (double)(correct + overlap) / result.size() << std::endl;
		std::cout << "Recall:               " << std::fixed << std::setprecision(4) << 100.0 * (double)(correct + overlap) / ground_truth.size() << std::endl;
        std::cout << "F1:                   " << std::fixed << std::setprecision(4) << 2 * (double)(correct + overlap) / result.size() * (double)(correct + overlap) / ground_truth.size() / ((double)(correct + overlap) / result.size() + (double)(correct + overlap) / ground_truth.size()) << std::endl;
		// std::cout << "Average deviation: " << std::fixed << std::setprecision(4) << (double)(error) / (correct + overlap) << std::endl;
		// std::cout << "Deviation rate: " << std::fixed << std::setprecision(4) << 100.0 * (double)overlap / (correct + overlap) << std::endl;
        return 2 * (double)(correct + overlap) / result.size() * (double)(correct + overlap) / ground_truth.size() / ((double)(correct + overlap) / result.size() + (double)(correct + overlap) / ground_truth.size());
	}

    void StageOneEffect () {
        int memory = 10; 
        uint32_t running_track_thres = BurstThreshold * running_track_thres_ratio;
        uint32_t run_length = 20000000;
        
        Abstract<uint64_t> *correct_detector = new CorrectDetector<uint64_t>(BurstThreshold);
	    BurstSketchAbstract<uint64_t, uint32_t> *burst_sketch = new BurstSketch<uint64_t, uint32_t>(memory, 1.0, running_track_thres, true);
        BurstSketchAbstract<uint64_t, uint32_t> *dynamic_sketch = new DynamicBurstSketch<uint64_t, uint32_t>(memory, 1.0, running_track_thres, true);
        
        std::set<uint64_t> items;
        for (uint64_t i = 0; i < run_length; ++i) {
            items.insert(caida->dataset[i].id);

            correct_detector->insert(caida->dataset[i].id, i);
            burst_sketch->insert(caida->dataset[i].id, i);
            dynamic_sketch->insert(caida->dataset[i].id, i);
        }

        std::vector<Burst<uint64_t>> ground_truth = correct_detector->query();
        std::set<uint64_t> real_bursts;
        uint32_t passed_bursts = 0;
        for (auto i : ground_truth) {
            real_bursts.insert(i.id);
        }

        std::set<uint64_t> pass_burst_sketch = burst_sketch->get_passed_IDs();
        std::set<uint64_t> pass_dynamic_sketch = dynamic_sketch->get_passed_IDs();

        std::cout << "Number of item IDs: " << items.size() << std::endl;
        std::cout << "Number of burst IDs: " << real_bursts.size() << std::endl;
        
        std::cout << " --- --- --- " << std::endl;
        std::cout << "Burst sketch:" << std::endl;
        std::cout << "Number of item IDs that pass through stage 1: " << pass_burst_sketch.size() << std::endl;
        passed_bursts = 0;
        for (auto i : real_bursts) {
            if (pass_burst_sketch.count(i) != 0)
                ++passed_bursts;
        }
        std::cout << "Number of burst IDs that pass through stage 1: " << passed_bursts << std::endl << std::endl;

        std::cout << " --- --- --- " << std::endl;
        std::cout << "Dynamic sketch:" << std::endl;
        std::cout << "Number of item IDs that pass through stage 1: " << pass_dynamic_sketch.size() << std::endl;
        passed_bursts = 0;
        for (auto i : real_bursts) {
            if (pass_dynamic_sketch.count(i) != 0)
                ++passed_bursts;
        }
        std::cout << "Number of burst IDs that pass through stage 1: " << passed_bursts << std::endl << std::endl;
    }

    void DurationCompare(std::vector<Burst<uint64_t>> &result, std::vector<Burst<uint64_t>> &ground_truth) {
        std::multimap<uint64_t, std::pair<uint32_t, uint32_t>> gt;
        uint32_t truth[win_cold_thres + 1] = {}, res[win_cold_thres + 1] = {};
        uint32_t correct[win_cold_thres + 1] = {}, overlap[win_cold_thres + 1] = {}, error[win_cold_thres + 1] = {};
		for (auto &record : ground_truth) {
			gt.insert(std::make_pair(record.id, std::make_pair(record.start_window, record.end_window)));
            truth[record.end_window - record.start_window]++;
		}

		for (auto &r : result) {
            res[r.end_window - r.start_window]++;
			auto begin = gt.lower_bound(r.id);
			auto end = gt.upper_bound(r.id);
            int tmperror = 2147483647, tmpwindow = 20;
			while (begin != end) {
				if (begin->second.first == r.start_window && begin->second.second == r.end_window) {
					++correct[begin->second.second - begin->second.first];
					break;
				}
				else if (r.start_window <= begin->second.second && r.end_window >= begin->second.first) {        
					// ++overlap;
                    // printf("WRONG: start: %d(%d) -> end: %d(%d)\n", r.start_window, begin->second.first, r.end_window, begin->second.second);
					// tmperror = MIN(tmperror, abs((begin->second.second - begin->second.first) - ((int)r.end_window - (int)r.start_window)));
                    // tmpwindow = begin->second.second - begin->second.first;
				}
				++begin;
			}
            // if (begin == end && tmperror != 2147483647) {
            //     ++overlap[tmpwindow];
            //     error[tmpwindow] += tmperror;
            // }
		}
        for (int i = 1; i < win_cold_thres + 1; ++i) {

            std::cout << "Burst Duration: " << i << std::endl;
            // std::cout << "truth[i]: " << truth[i] << " res[i]: " << res[i] << std::endl;
            std::cout << "correct: " << correct[i] << " overlap: " << overlap[i] << " result: " << res[i] << " truth: " << truth[i] << std::endl;
            // std::cout << "Find " << correct[i] << " bursts\n";
		    std::cout << "Precision:            " << std::fixed << std::setprecision(4) << 100.0 * (double)(correct[i] + overlap[i]) / res[i] << std::endl;
		    std::cout << "Recall:               " << std::fixed << std::setprecision(4) << 100.0 * (double)(correct[i] + overlap[i]) / truth[i] << std::endl;
            std::cout << "F1:                   " << std::fixed << std::setprecision(4) << 2 * (double)(correct[i] + overlap[i]) / res[i] * (double)(correct[i] + overlap[i]) / truth[i] / ((double)(correct[i] + overlap[i]) / res[i] + (double)(correct[i] + overlap[i]) / truth[i]) << std::endl;
		    // std::cout << "Average deviation: " << std::fixed << std::setprecision(4) << (double)(error[i]) / (correct[i] + overlap[i]) << std::endl;
		    // std::cout << "Deviation rate: " << std::fixed << std::setprecision(4) << 100.0 * (double)overlap[i] / (correct[i] + overlap[i]) << std::endl;
        }
        

    }

    void DurationTest(int memory) {
        uint32_t running_track_thres = BurstThreshold * running_track_thres_ratio;
        uint32_t run_length = 2000000;
        
        Abstract<uint64_t> *correct_detector = new CorrectDetector<uint64_t>(BurstThreshold);
	    BurstSketchAbstract<uint64_t, uint32_t> *burst_sketch = new BurstSketch<uint64_t, uint32_t>(memory, 1.0, running_track_thres, true);
        BurstSketchAbstract<uint64_t, uint32_t> *dynamic_sketch = new DynamicBurstSketch<uint64_t, uint32_t>(memory, 1.0, running_track_thres, true);

        for (uint64_t i = 0; i < run_length; ++i) {
            correct_detector->insert(caida->dataset[i].id, i);
            burst_sketch->insert(caida->dataset[i].id, i);
            dynamic_sketch->insert(caida->dataset[i].id, i);
        }

        std::vector<Burst<uint64_t>> ground_truth = correct_detector->query();
        std::vector<Burst<uint64_t>> burst_sketch_result = burst_sketch->query();
        std::vector<Burst<uint64_t>> dynamic_sketch_result = dynamic_sketch->query();

        DurationCompare(burst_sketch_result, ground_truth);
        DurationCompare(dynamic_sketch_result, ground_truth);
        
    }

    double Run(int memory, int flag) {
        uint32_t running_track_thres = BurstThreshold * running_track_thres_ratio;
        uint32_t run_length = 20000000;
        if (!count_based)
            start_time = caida->dataset[0].timestamp;
        Abstract<uint64_t> *correct_detector = new CorrectDetector<uint64_t>(BurstThreshold);
	    Abstract<uint64_t> *burst_sketch = new BurstSketch<uint64_t, uint32_t>(memory, stage_ratio, running_track_thres);
        Abstract<uint64_t> *dynamic_sketch = new DynamicBurstSketch<uint64_t, uint32_t>(memory, stage_ratio_optimized, running_track_thres);
        Abstract<uint64_t> *cm_sketch = new CMSketchDetector<uint64_t, uint32_t>(memory, BurstThreshold);
        
        // std::cout << caida->dataset[0].timestamp << std::endl;
        // std::cout << caida->dataset[run_length - 1].timestamp << std::endl;
        for (uint32_t i = 0; i < run_length; ++i) {
            // if (i >= 20000000) 
            //     break;
            if (count_based)
                correct_detector->insert(caida->dataset[i].id, i);
            else 
                correct_detector->insert(caida->dataset[i].id, caida->dataset[i].timestamp);
        }
        std::vector<Burst<uint64_t>> ground_truth = correct_detector->query();
        
        double temp;

        // run BurstSketch
        if (flag & FLAG_BURST) {
            auto burst_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                if (count_based)
                    burst_sketch->insert(caida->dataset[i].id, i);
                else 
                    burst_sketch->insert(caida->dataset[i].id, caida->dataset[i].timestamp);
            }
            auto burst_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> burst_sketch_result = burst_sketch->query();
            std::chrono::duration<double> burst_span = std::chrono::duration_cast<std::chrono::duration<double>>(burst_end - burst_start);
            std::cout << "Burst Sketch:" << std::endl;
		    // std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * burst_span.count()) << "\n\n";
            temp = Compare(burst_sketch_result, ground_truth);
            
            // return tmp;
        }

        // run DynamicBurstSketch
        if (flag & FLAG_DYNAMIC) {
            auto dynamic_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                if (count_based)
                    dynamic_sketch->insert(caida->dataset[i].id, i);
                else 
                    dynamic_sketch->insert(caida->dataset[i].id, caida->dataset[i].timestamp);
            }
            auto dynamic_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> dynamic_sketch_result = dynamic_sketch->query();
            std::chrono::duration<double> dynamic_span = std::chrono::duration_cast<std::chrono::duration<double>>(dynamic_end - dynamic_start);
            std::cout << "Dynamic Burst Sketch:" << std::endl;
            // std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * dynamic_span.count()) << "\n\n";
            temp = Compare(dynamic_sketch_result, ground_truth);
            
        }

        // run CMSketch
        if (flag & FLAG_CM) {
            auto cm_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                if (count_based)
                    cm_sketch->insert(caida->dataset[i].id, i);
                else 
                    cm_sketch->insert(caida->dataset[i].id, caida->dataset[i].timestamp);
            }
            auto cm_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> cm_sketch_result = cm_sketch->query();
        
            std::chrono::duration<double> cm_span = std::chrono::duration_cast<std::chrono::duration<double>>(cm_end - cm_start);
            std::cout << "CM Sketch: " << std::endl;
            // std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * cm_span.count()) << "\n\n";
            temp = Compare(cm_sketch_result, ground_truth);
        }
        
        delete correct_detector;
		delete burst_sketch;
        delete dynamic_sketch;
        delete cm_sketch;

        return temp;
        
    }

    void InsideRun(int memory, int flag) {
        uint32_t running_track_thres = BurstThreshold * running_track_thres_ratio;
        uint32_t run_length = 20000000;
        Abstract<uint64_t> *correct_detector = new CorrectDetectorInside<uint64_t>(BurstThreshold);
	    Abstract<uint64_t> *burst_sketch = new BurstSketchInside<uint64_t, uint32_t>(memory, 1.0, running_track_thres);
        Abstract<uint64_t> *dynamic_sketch = new DynamicBurstSketchInside<uint64_t, uint32_t>(memory, 1.0, running_track_thres);
        for (uint32_t i = 0; i < run_length; ++i) {
            correct_detector->insert(caida->dataset[i].id, i);
        }
        std::vector<Burst<uint64_t>> ground_truth = correct_detector->query();

        // run BurstSketch
        if (flag & FLAG_BURST) {
            auto burst_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                burst_sketch->insert(caida->dataset[i].id, i);
            }
            auto burst_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> burst_sketch_result = burst_sketch->query();
            std::chrono::duration<double> burst_span = std::chrono::duration_cast<std::chrono::duration<double>>(burst_end - burst_start);
            std::cout << "Burst Sketch:" << std::endl;
		    Compare(burst_sketch_result, ground_truth);
            // std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * burst_span.count()) << "\n\n";
        }
        
        // run DynamicBurstSketch
        if (flag & FLAG_DYNAMIC) {
            auto dynamic_start = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < run_length; ++i) {
                dynamic_sketch->insert(caida->dataset[i].id, i);
            }
            auto dynamic_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> dynamic_sketch_result = dynamic_sketch->query();
            std::chrono::duration<double> dynamic_span = std::chrono::duration_cast<std::chrono::duration<double>>(dynamic_end - dynamic_start);
            std::cout << "Dynamic Burst Sketch:" << std::endl;
            Compare(dynamic_sketch_result, ground_truth);
            // std::cout << "Throughput(Mips):     " << run_length / (1000 * 1000 * dynamic_span.count()) << "\n\n";
        }
        
        delete correct_detector;
		delete burst_sketch;
        delete dynamic_sketch;
    }

    void MemoryInsideTest() {
        std::cout << "burst inside burst test for BurstSketch, MergeSketch and Dynamic Sketch.\n\n";
        for (auto x : memory_list) {
            std::cout << "memory: " << x << "\n";
            InsideRun(x, FLAG_BURST | FLAG_DYNAMIC);
        }
    }

    void TimeBasedTest() {
        ParamInitialize();
        std::cout << "window type test for BurstSketch. \n\n";
        Run(60, FLAG_BURST);
        count_based = false;
        Run(60, FLAG_BURST);

        ParamInitialize();
        std::cout << "window type test for DynamicSketch. \n\n";
        Run(60, FLAG_DYNAMIC);
        count_based = false;
        Run(60, FLAG_DYNAMIC);
    }

    void MemoryUseTest() {
        // for BurstSketch
        for (auto i : size_array) {
            window_size = i;
            uint32_t left = 1, right = 100;
            while (left + 1 < right) {
                uint32_t mid = (left + right) / 2;
                std::cout << "memory: " << mid << std::endl;
                if (Run(mid, FLAG_BURST) >= 0.95)
                    right = mid;
                else 
                    left = mid;
            }
            std::cout << "BurstSketch: window size = " << window_size << ", memory use = " << right << std::endl;
        }
        // run DynamicBurstSketch
        for (auto i : size_array) {
            window_size = i;
            uint32_t left = 1, right = 100;
            while (left + 1< right) {
                uint32_t mid = (left + right) / 2;
                std::cout << "memory: " << mid << std::endl;
                if (Run(mid, FLAG_DYNAMIC) >= 0.95)
                    right = mid;
                else 
                    left = mid;
                std::cout << left << " " << right << std::endl;
            }
            std::cout << "DynamicBurstSketch: window size = " << window_size << ", memory use = " << right << std::endl;
        }
    }
private:
    CAIDADataset *caida;
};


#endif