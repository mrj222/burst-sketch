package burst;

/**
 * stage 1 in BurstSketch
 * use Running Track to filter low arrival rate items
 */
class RunningTrackBucket implements java.io.Serializable {
	public int id = 0;
	public int counter = 0;
}

public class RunningTrack implements java.io.Serializable {
	private RunningTrackBucket[] buckets = new RunningTrackBucket [10000];
	private int simple_hash = 3;

	RunningTrack() {
		for (int i = 0; i < 10000; ++i) {
			buckets[i] = new RunningTrackBucket();
		}
	}

	// when move to the next windows, all buckets in the stage 1 
	// should be cleared
	public void clearAll() {
		for (int i = 0; i < BurstParams.RunningTrack_size; ++i) {
			buckets[i].id = 0;
			buckets[i].counter = 0;
		}
	}

	// insert an id into the Stage 1
	// if the id can pass through the Stage 1, return the index
	public int insert(int new_id) {
		int ret = -1;
		int index = ((simple_hash * new_id) % BurstParams.RunningTrack_size);
		if (buckets[index].id == new_id) {
			++buckets[index].counter;
			if (buckets[index].counter >= BurstParams.RunningTrack_threshold)
				ret = index;
		}
		else if (buckets[index].counter == 0) {
			++buckets[index].counter;
			buckets[index].id = new_id;
		}
		else {
			--buckets[index].counter;
		}
		return ret;
	}

	// when id successfully pass to stage 2, the bucket should be cleared up
	void clearIndex(int index) {
		buckets[index].id = 0;
		buckets[index].counter = 0;
	}

	int getCounter(int index) {
		return buckets[index].counter;
	}
}
