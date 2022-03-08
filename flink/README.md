# Integration to Apache Flink

We provide two implementation on top of Apache Flink.

## File Structure

`burstsketch`: implementation of BurstSketch on Flink.

`statefulDetector`: implementation of  Stateful Detector on Flink.

## Input File

We generate the input file based on CAIDA datasets. For the full CAIDA datasets, please register in [CAIDA](http://www.caida.org/home/) first and then apply for the traces.

## Requirements

* `Flink 1.13.1`
* `Hadoop 2.8.3`

To address the problem of dependency when using Hadoop Distributed File System in Flink, [flink-shaded-hadoop-2-uber-2.8.3-9.0.jar]([Central Repository: org/apache/flink/flink-shaded-hadoop-2-uber/2.8.3-9.0](https://repo.maven.apache.org/maven2/org/apache/flink/flink-shaded-hadoop-2-uber/2.8.3-9.0/)) is needed. Please add it to `{FLINK_HOME}/lib/`.

## How to run

1. Build the package. The jar package can be built by `Maven 3.2.5`.

2. Set up Flink configuration and Hadoop Distributed File System. Copy the input file to HDFS.

3. Run the code. Suppose the path of the input file is `hdfs://test.txt`, and the path of the jar package is `/root/test.jar`, you can use the following command to run the codes.

   ```shell
   {FLINK_HOME}/bin/flink run -p k /root/test.jar --input hdfs://test.txt
   ```

   where `k` is the parallism (# parallel instances) in Flink.