package burst;

public class BurstParams {
    // stage 1
    public static final int RunningTrack_hash_num = 1;
    public static final int RunningTrack_size = 10000;
    public static final int RunningTrack_threshold = 30;
    // stage 2
    public static final int snapshot_bucket_num = 10000;
    public static final int snapshot_bucket_size = 4; // number of cells in one bucket

    // burst
    public static final int win_cold_thres = 10;
    public static final int threshold = 50;
    public static final int lambda = 2;

    // sliding window
    public static final long window_size = 40000;
}
