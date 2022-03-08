#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "CAIDABenchmark.h"
#include "WebPageBenchmark.h"
#include "NetWorkBenchmark.h"
#include "CAIDAAnalysis.h"

using namespace std;

void print_help() {
    // keep cpu happy
    cout << "Something wrong!\n";
    cout << "command: ./cpu [path] [command-type] [parameter] [memory]";
}

int main(int argc, char *argv[]) {
    /*
    Parameter Experiment: ./cpu [path] 0 -[parameter] (memory)
        d: hash test
        s: stage ratio test
        t: threshold ratio test
        b: bucket size test
        k: burst ratio test
        r: replacement strategy test 
    Memory Experiment: ./cpu [path] 1 -[parameter] 
        c: caida
        g: webpage
        o: network
    Analysis Experiment: ./cpu [path] 2 -[parameter] (memory)
        v: window type test (no memory)
        n: number passing stage 1 test (no memory)
        u: memory usage test (no memory)
        i: burst inside burst test (no memory)
        l: duration test
    */ 
    int option;
    if (atoi(argv[2]) == 0 && (option = getopt(argc, argv, "d:s:t:b:k:r:")) != -1) {
        CAIDADataset caida(argv[1], "caida");
        CAIDABenchmark benchmark(&caida);
        switch (option) {
            case 'd': 
                benchmark.HashTest(atoi(optarg));
                break;
            case 's': 
                benchmark.StageRatioTest(atoi(optarg));
                break;
            case 't': 
                benchmark.ThresRatioTest(atoi(optarg));
                break;
            case 'b': 
                benchmark.CellTest(atoi(optarg));
                break;
            case 'k':
                benchmark.LambdaTest(atoi(optarg));
                break;
            case 'r':
                benchmark.ReplaceTest(atoi(optarg));
                break;
            default: 
                print_help();
                break;
        }
    }
    else if (atoi(argv[2]) == 1 && (option = getopt(argc, argv, "cgo")) != -1) {
        switch (option) {
            case 'c': {
                CAIDADataset caida(argv[1], "caida");
                CAIDABenchmark benchmark(&caida);
                benchmark.MemoryTest();
                break;
            }
            case 'g': {
                WebDataset web(argv[1], "web");
                WebBenchmark benchmark(&web);
                benchmark.MemoryTest();
                break;
            }
            case 'o': {
                NetDataset net(argv[1], "net");
                NetBenchmark benchmark(&net);
                benchmark.MemoryTest();
                break;
            }
            default: 
                print_help();
                break;
        }
    }
    else if (atoi(argv[2]) == 2 && (option = getopt(argc, argv, "vnuil:")) != -1) {
        CAIDADataset caida(argv[1], "caida");
        CAIDAAnalysis analysis(&caida);
        switch (option) {
            case 'v': 
                analysis.TimeBasedTest();
                break;
            case 'n': 
                analysis.StageOneEffect();
                break;
            case 'u': 
                analysis.MemoryUseTest();
                break;
            case 'i': 
                analysis.MemoryInsideTest();
                break;
            case 'l': 
                analysis.DurationTest(atoi(optarg));
                break;
            default: 
                print_help();
                break;
        }
    }
    else {
        print_help();
    }
    
    return 0;
}
