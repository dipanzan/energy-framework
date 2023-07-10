package com.dipanzan;

import java.io.IOException;
import java.math.BigDecimal;
import java.nio.file.Files;
import java.nio.file.Paths;

public class Method {
    @FunctionalInterface
    public interface Execute {
        void execute();
    }

    public static void execute(Execute e) {
        e.execute();
    }

    public static void execute(int core, Execute e) {
        long before = readEnergy(core);
        e.execute();
        long after = readEnergy(core);

        printEnergyConsumed(core, before, after);
    }

    private static void printEnergyConsumed(int core, long before, long after) {
//        double result = (after - before) / Math.pow(10, 6);
        double result = (after - before) * 2.3283064365386962890625e-10;
        System.out.println("=====================JAVA======================");
        System.out.println("Core: " + core + " energy consumed: " + result + "J");
        System.out.println("=====================JAVA======================");
    }

    private static long readEnergy(int core) {
        core = core + 1;
        try {
            String read = Files.readString(Paths.get("/sys/class/hwmon/hwmon5/energy" + core + "_input"));
            read = read.replace("\n", "");
            return Long.parseLong(read);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
