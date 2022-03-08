# BurstSketch


## About the source code

The source code in each directory is implemented by C++, including the BurstSketch algorithm and the strawman solution.

You should use your own dataset and move it to the root directory.

## How to run

* change directory to cpu and compile the source code

```
cd cpu
cmake .
make
```

* run parameter experiments on CAIDA dataset: 

	* run hash number test (d), stage ratio test(s), threshold ratio test (t), bucket size test (b), burst ratio test (k), replacement strategy test (r): 

	```
	./cpu {path} 0 -{command-type} {memory}
	```


* run memory experiments on different datasets: 

	* CAIDA dataset (c), Webpage dataset (g), Network dataset (o): 
	```
	./cpu {path} 1 -{command-type}
	```


* run analysis experiments on CAIDA dataset: 

	* run window type test (n), effect of stage 1 test (n), memory usage test (u), burst inside burst test (i): 
	```
	./cpu {path} 2 -{command-type}
	```

	* run duration test: 
	```
	./cpu {path} 2 -l {memory}
	```

## Output format

We output some metrics for each application. (like F1, RR, PR, Throughput etc)