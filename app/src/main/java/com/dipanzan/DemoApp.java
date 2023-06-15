package com.dipanzan;

import java.nio.file.Files;
import java.nio.file.Paths;

public class DemoApp {
    public static void main(String[] args) throws Exception {
        System.out.println("Application Start");

//        DemoApp app = new DemoApp();
//        for (int i = 0; i < 100; i++) {
//            app.sleeping(10);
//        }

        Matrix m = new Matrix();
        m.run();
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