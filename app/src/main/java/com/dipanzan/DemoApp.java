package com.dipanzan;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Scanner;

public class DemoApp {
    public static void main(String[] args) throws Exception {
        long pid = ProcessHandle.current().pid();
        System.out.println(pid);

        Scanner sc = new Scanner(System.in);
//        System.out.print("Application Proceed: ");
//        sc.nextLine();

//        Method.execute(() -> new Matrix().run());

        int core = Integer.parseInt(args[0]);
        int size = Integer.parseInt(args[1]);
        Method.execute(core, () -> new Matrix(size).run());

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