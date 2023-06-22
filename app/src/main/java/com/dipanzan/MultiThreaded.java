package com.dipanzan;

public class MultiThreaded {
    private static long HUGE_NUMBER_MAX = 100_000_0000;
    private static long HUGE_NUMBER_MIN = 100_000_000;

    public void run(int numThreads) {
        Thread[] threads = new Thread[numThreads];

        for (int i = 0; i < numThreads; i++) {
            threads[i] = new Thread(() -> load(random(HUGE_NUMBER_MAX, HUGE_NUMBER_MIN)));
            threads[i].start();
        }
    }

    private void load(long n) {
        long tid = Thread.currentThread().getId();
        String name = Thread.currentThread().getName();

        for (int i = 0; i < n; i++) {
            System.out.println("TID:[" + tid + "]: " + "[" + name + "]" + " executing.");
        }
    }

    private long random(long max, long min) {
        return (long) (Math.random() * (max - min + 1)) + min;
    }
}
