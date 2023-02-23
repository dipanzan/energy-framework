package com.dipanzan;

public class DemoApp {
    public static void main(String[] args) {
        System.out.println("Application Start");
        sleeping(2);
    }

    private static void sleeping(long seconds) {
        while (true) {
            trySleep(seconds);
        }
    }

    private static void trySleep(long seconds) {
        try {
            System.out.println("Sleeping for " + seconds + "s ... Zzz");
            Thread.sleep(seconds * 1000);
            System.out.println("Slept for " + seconds + "s. Just woke up!");
        } catch (InterruptedException ignored) {
        }
    }
}