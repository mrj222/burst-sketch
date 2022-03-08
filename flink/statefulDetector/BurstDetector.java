package strawman;

import org.apache.flink.api.common.state.ValueState;
import org.apache.flink.api.common.state.ValueStateDescriptor;
import org.apache.flink.api.common.typeinfo.Types;
import org.apache.flink.configuration.Configuration;
import org.apache.flink.api.java.utils.MultipleParameterTool;
import org.apache.flink.streaming.api.functions.KeyedProcessFunction;
import org.apache.flink.util.Collector;
import org.apache.flink.api.java.tuple.Tuple2;
import org.apache.flink.api.java.tuple.Tuple3;

class BurstState {
    public long last_timestamp;
    public int window_cnt;
    public int last_win_freq;
    public int current_win_freq;
    public int increase_win;

    BurstState() {
        last_timestamp = 0;
        window_cnt = 0;
        last_win_freq = current_win_freq = 0;
        increase_win = -1;
    }
};

public class BurstDetector extends KeyedProcessFunction<Integer, Tuple2<Integer, Long>, Tuple3<Integer, Integer, Integer>> {
    private transient ValueState<BurstState> burst_state; 

    @Override
    public void open(Configuration parameters) {
        ValueStateDescriptor<BurstState> BurstStateDescriptor = new ValueStateDescriptor<>(
                "burst_state",
                BurstState.class);
        burst_state = getRuntimeContext().getState(BurstStateDescriptor);
    }

    @Override
    public void processElement(
        Tuple2<Integer, Long> item,
        Context ctx,
        Collector<Tuple3<Integer, Integer, Integer>> collector) throws Exception {

        BurstState state = burst_state.value();
        if (state == null) {
            state = new BurstState();
        }

        while (state.last_timestamp + BurstParams.window_size < item.f1) {
            // update
            if (state.increase_win != -1 && state.window_cnt - state.increase_win > BurstParams.win_cold_thres) {
                state.increase_win = -1;
            }

            if (state.current_win_freq * BurstParams.lambda <= state.last_win_freq && state.increase_win != -1) {
                Tuple3<Integer, Integer, Integer> found_burst = new Tuple3<Integer, Integer, Integer>(item.f0, state.increase_win, state.window_cnt);
                collector.collect(found_burst);
                state.increase_win = -1;
                state.last_win_freq = state.current_win_freq = 0;
            }
            else if (state.current_win_freq < BurstParams.threshold) {
                state.increase_win = -1;
            }
            else if (state.current_win_freq >= state.last_win_freq * BurstParams.lambda && state.current_win_freq >= BurstParams.threshold) {
                state.increase_win = state.window_cnt;
            }

            state.last_timestamp += BurstParams.window_size;
            state.window_cnt++;
        }

        state.current_win_freq++;
        burst_state.update(state);
    }
}
