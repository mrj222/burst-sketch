package burst;

import org.apache.flink.util.Collector;
import org.apache.flink.api.java.tuple.Tuple3;

/**
 * stage 2 in BurstSketch
 * use Snapshotting to find burst pattern
 */
class Bucket implements java.io.Serializable {
	public int id = 0;
	public int window = -1;
	public int[] win_counter = new int [2];
}

public class Snapshot implements java.io.Serializable {
	private int flag = 0;
	private int simple_hash = 1;
	private Bucket[][] buckets = new Bucket [10000][4];

	Snapshot() {
		for (int i = 0; i < 10000; ++i)
			for (int j = 0; j < 4; ++j)
				buckets[i][j] = new Bucket();
	}

	// check whether the id is already in the stage 2
	// if so, increment it by 1
	public boolean lookup(int new_id) {
		int index = ((simple_hash * new_id) % BurstParams.snapshot_bucket_num);
		for (int i = 0; i < BurstParams.snapshot_bucket_size; ++i) {
			if (buckets[index][i].id == new_id) {
				++buckets[index][i].win_counter[flag];
				return true;
			}
		}
		return false;
	}

	// insert (new_id, count) into stage 2
	// sometimes need to select substitute
	public boolean insert(int new_id, int window, int count) {
		int index = ((simple_hash * new_id) % BurstParams.snapshot_bucket_num);
		int slot_index = 0;
        int min_value = Integer.MAX_VALUE;
		boolean non_bursting = false;
		
		for (int i = 0; i < BurstParams.snapshot_bucket_size; ++i) {
			if (buckets[index][i].id == 0) { // empty slot
				slot_index = i;
				break;
			}
			// find minimum current counter
			// first search in no those slots whose window==-1
			if (buckets[index][i].window == -1) {
				if (!non_bursting) {
					non_bursting = true;
					min_value = Integer.MAX_VALUE;
				}
				if (buckets[index][i].win_counter[flag] < min_value) {
					min_value = buckets[index][i].win_counter[flag];
					slot_index = i;
				}
			}
			else if (!non_bursting) {
				if (buckets[index][i].win_counter[flag] < min_value) {
					min_value = buckets[index][i].win_counter[flag];
					slot_index = i;
				}
			}
		}
		// slot_index is the position to substitute
		if (buckets[index][slot_index].id == 0 || 
		  count > buckets[index][slot_index].win_counter[flag]) {
		  	// substitute
			buckets[index][slot_index].id = new_id;
			buckets[index][slot_index].window = -1;
			buckets[index][slot_index].win_counter[flag] = count;
			buckets[index][slot_index].win_counter[1 - flag] = 0;
			return true;
		}
		return false;
	}

	public void update_window(int cur_window, Collector<Tuple3<Integer, Integer, Integer>> collector) {
		// clear up
		for (int i = 0; i < BurstParams.snapshot_bucket_num; ++i) {
			for (int j = 0; j < BurstParams.snapshot_bucket_size; ++j) {
				if (buckets[i][j].id != 0 && buckets[i][j].window != -1 &&
				  cur_window - buckets[i][j].window > BurstParams.win_cold_thres) {
					buckets[i][j].win_counter[0] = buckets[i][j].win_counter[1] = 0;
					buckets[i][j].id = 0;
					buckets[i][j].window = -1;
				}
				else if (buckets[i][j].id != 0 && buckets[i][j].win_counter[0] < BurstParams.RunningTrack_threshold &&
				  buckets[i][j].win_counter[1] < BurstParams.RunningTrack_threshold) {
					buckets[i][j].win_counter[0] = buckets[i][j].win_counter[1] = 0;
					buckets[i][j].id = 0;
					buckets[i][j].window = -1;
				}
			}
		}

		// detect burst
		for (int i = 0; i < BurstParams.snapshot_bucket_num; ++i) {
			for (int j = 0; j < BurstParams.snapshot_bucket_size; ++j) {
				if (buckets[i][j].id == 0) continue;
				if (buckets[i][j].window != -1 && buckets[i][j].win_counter[flag] * BurstParams.lambda <= buckets[i][j].win_counter[1 - flag]) {
                    Tuple3<Integer, Integer, Integer> found_burst = new Tuple3<Integer, Integer, Integer>(buckets[i][j].id, buckets[i][j].window, cur_window);
					collector.collect(found_burst);
					buckets[i][j].id = 0;
					buckets[i][j].win_counter[0] = buckets[i][j].win_counter[1] = 0;
					buckets[i][j].window = -1;
				}
				else if (buckets[i][j].win_counter[flag] < BurstParams.threshold)
					buckets[i][j].window = -1;
				else if (buckets[i][j].win_counter[flag] >= BurstParams.threshold &&
                 buckets[i][j].win_counter[flag] >= BurstParams.lambda * buckets[i][j].win_counter[1 - flag]) 
					buckets[i][j].window = cur_window;
			}
		}
		// update flag
		flag = 1 - flag;
		for (int i = 0; i < BurstParams.snapshot_bucket_num; ++i)
			for (int j = 0; j < BurstParams.snapshot_bucket_size; ++j)
				buckets[i][j].win_counter[flag] = 0;
	}
}
