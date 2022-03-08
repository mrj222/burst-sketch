# BurstSketch

## Introduction

Burst is a common pattern in data streams which is characterized by a sudden increase in terms of arrival rate followed by a sudden decrease. Burst detection has attracted extensive attention from the research community. To detect bursts accurately in real time, we propose a novel sketch, namely BurstSketch, which consists of two stages. Stage 1 uses the technique Running Track to select potential burst items efficiently. Stage 2 monitors the potential burst items and captures the key features of burst pattern by a technique called Snapshotting. We further propose a optimization, namely Dynamic Buckets, which can improve the accuracy of BurstSketch. We provide theoretical error bounds for Stage 1, Stage 2 and the optimized version. Experimental results show that, compared with the strawman solution, Burstsketch achieves 2.00 to 11.63 times higher F1 score, and 1.56 times higher throughput. We also integrate BurstSketch into Apache Flink, and show that using BurstSketch can be faster than simply using the built-in APIs provided by Apache Flink.

## About the source code

* `cpu`: contains the source code for CPU experiments.
* `flink`: contain the source code for Apache Flink experiments.