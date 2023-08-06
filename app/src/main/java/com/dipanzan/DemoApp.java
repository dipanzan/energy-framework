package com.dipanzan;

import com.dipanzan.annotation.Energy;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Scanner;

public class DemoApp {
    public static void main(String[] args) throws Exception {
        long pid = ProcessHandle.current().pid();
        System.out.println(pid);

        int threads = Integer.parseInt(args[0]);
        System.out.println(threads);
        int time = Integer.parseInt((args[1]));
        System.out.print("Time: ");
        int time2 = new Scanner(System.in).nextInt();
        System.out.println(time);
//        Method.execute(() -> new Matrix(threads, time2).run2());
        hello(time2);
    }

    @Energy
    private static void hello(int times)
    {
        for (int i = 0; i < times; i++) {
            System.out.println("hello!");
        }
    }
    private static void sleep(int n)
    {
        try {
            Thread.sleep(n);

        } catch (Exception e)
        {

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