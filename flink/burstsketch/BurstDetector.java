package burst;

import org.apache.flink.api.common.state.ValueState;
import org.apache.flink.api.common.state.ValueStateDescriptor;
import org.apache.flink.api.common.typeinfo.Types;
import org.apache.flink.configuration.Configuration;
import org.apache.flink.api.java.utils.MultipleParameterTool;
import org.apache.flink.streaming.api.functions.KeyedProcessFunction;
import org.apache.flink.util.Collector;
import org.apache.flink.api.java.tuple.Tuple2;
import org.apache.flink.api.java.tuple.Tuple3;

public class BurstDetector extends KeyedProcessFunction<Integer, Tuple2<Integer, Long>, Tuple3<Integer, Integer, Integer>> {
    private RunningTrack running_track = new RunningTrack();
    private Snapshot snapshot = new Snapshot();
    private long last_timestamp = 0;
    private int window_cnt = 0;

    @Override
    public void processElement(
        Tuple2<Integer, Long> item,
        Context ctx,
        Collector<Tuple3<Integer, Integer, Integer>> collector) throws Exception {

        while (last_timestamp + BurstParams.window_size < item.f1) {
            last_timestamp += BurstParams.window_size;
            running_track.clearAll();
            snapshot.update_window(window_cnt, collector);
            window_cnt++;
        }

        if (snapshot.lookup(item.f0))
            return;
        int ret_pos = running_track.insert(item.f0);
        if (ret_pos == -1)
            return;
        int count = running_track.getCounter(ret_pos);
        if (snapshot.insert(item.f0, window_cnt, count))
            running_track.clearIndex(ret_pos);
    }
}
