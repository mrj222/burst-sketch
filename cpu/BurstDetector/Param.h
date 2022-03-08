#ifndef _PARAM_H_
#define _PARAM_H_
#include <cstdint>

#define FREQUENT     0x0
#define PROB_DECAY   0x1
#define PROB_REPLACE 0X2

const char* replace_str[] = {
	"frequent", 
	"probabilistic decay", 
	"probabilistic replace"
};

#define FLAG_BURST       0x1
#define FLAG_DYNAMIC     0x2
#define FLAG_CM          0x4
#define FLAG_TOPIC       0x8
#define FLAG_PBE1        0x10
#define FLAG_PBE2        0x20


const static double ratio[] = {
    0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6
};

const static double memory_list[] = {
    20, 40, 60, 80, 100, 120
};


uint32_t window_size = 40000;
const int win_cold_thres = 10;
const int dynamic_bit = 32;
const uint64_t window_time = 8000000000000;

uint32_t BurstThreshold = 50;
int lambda = 2;
double running_track_thres_ratio = 0.2;
int hash_num = 1;
int stage2_bucket_size = 4;
int replace_strategy = FREQUENT;
bool count_based = true;
uint64_t start_time;

double stage_ratio = 1.0;
double stage_ratio_optimized = 1.0;

void ParamInitialize() {
	BurstThreshold = 50;
	lambda = 2;
	running_track_thres_ratio = 0.2;
	hash_num = 1;
	stage2_bucket_size = 4;
	replace_strategy = FREQUENT;
	stage_ratio = 1;
	stage_ratio_optimized = 1;
	count_based = true;
}


#endif
