package com.dipanzan;

import com.dipanzan.annotation.Energy;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Scanner;

public class DemoApp {
    public static void test() {
        do {
            Scanner sc = new Scanner(System.in);
            System.out.print("input: ");
            long result = sc.nextLong();
            System.out.println("RESULT: " + result * 2.3283064365386962890625e-10 + "J");
        } while (true);

    }

    public static void main(String[] args) throws Exception {

//        test();
        long pid = ProcessHandle.current().pid();
        System.out.println(pid);

        int threads = Integer.parseInt(args[0]);
        System.out.println(threads);
        int time = Integer.parseInt((args[1]));
//        System.out.print("Press [enter]: ");
//        new Scanner(System.in).nextLine();
//        Method.execute(1, () -> new Matrix(threads, time).run2());
        new Matrix(threads, time).run2();
//        HELLO_BROCK(time);
    }

    private static boolean run = false;

    private static void inc(int time) {

        for (int i = 0; i < time; i++) {
            try {
                Thread.sleep(1000);
            } catch (Exception e) {

            }
        }
        run = false;

    }

    @Energy
    private static void HELLO_BROCK(int time) {
        Thread t = new Thread(() -> inc(time));
        t.start();
        run = true;
        while (run) {
            System.out.print("HELLO BROCK");
        }
        t.stop();
    }

    @Energy
    private static void hello(int times) {
        for (int i = 0; i < times; i++) {
            System.out.print("");
        }
    }

    private static void sleep(int n) {
        try {
            Thread.sleep(n);

        } catch (Exception e) {

        }
    }


    private void sleeping(long seconds) throws Exception {
        String energyPath2 = "/sys/class/hwmon/hwmon5/energy" + 1 + "_input";
        String result1 = Files.readString(Paths.get(energyPath2));
        trySleep(seconds);
        String result2 = Files.readString(Paths.get(energyPath2));

        System.out.println("BEFORE: " + result1);
        System.out.println("AFTER: " + result2);
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