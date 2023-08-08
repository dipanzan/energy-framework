package com.dipanzan.advice;


import net.bytebuddy.asm.Advice;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Scanner;

import static net.bytebuddy.implementation.bytecode.assign.Assigner.Typing.DYNAMIC;

public class EnergyAdvice {
    public static Advice getAdvice() {
        return Advice.to(EnergyAdvice.class);
    }

    public static void printEnergyConsumed(Method method, long before, long after) {
//        double result = (after - before) / Math.pow(10, 6);
        double result = (after - before) * 2.3283064365386962890625e-10;
        System.out.println("=====================JAVA======================");
        System.out.println(method.getName() + "(): energy consumed: " + result + "J");
        System.out.println("=====================JAVA======================");
    }

    public static long readEnergy(int core) throws Exception {
        String energyPath = "/sys/class/hwmon/hwmon5/energy" + core + "_input";
        Scanner sc = new Scanner((new File(energyPath)));
        return sc.nextLong();

    }

    @Advice.OnMethodEnter(suppress = Throwable.class)
    public static long enter(@Advice.Origin Method method,
                             @Advice.AllArguments(typing = DYNAMIC) Object[] args) throws Exception {
        return readEnergy(1);
    }

    @Advice.OnMethodExit(suppress = Throwable.class, onThrowable = Throwable.class)
    public static void exit(@Advice.Origin Method method, @Advice.Enter long before, @Advice.Origin String origin) throws Exception {
        long after = readEnergy(1);
        printEnergyConsumed(method, before, after);
    }

//    @Advice.OnMethodEnter(suppress = Throwable.class)
//    public static long enter2(@Advice.Origin Method method,
//                             @Advice.AllArguments(typing = DYNAMIC) Object[] args) throws Exception {
//        String energyPath = "/sys/class/hwmon/hwmon5/energy1" + "_input";
//        Scanner sc = new Scanner((new File(energyPath)));
//        return sc.nextLong();
//    }
//    @Advice.OnMethodExit(suppress = Throwable.class, onThrowable = Throwable.class)
//    public static void exit2(@Advice.Origin Method method, @Advice.Enter long before, @Advice.Origin String origin) throws Exception {
//        String energyPath = "/sys/class/hwmon/hwmon5/energy1" + "_input";
//        Scanner sc = new Scanner((new File(energyPath)));
//        long after = sc.nextLong();
//        double consumed = (after - before) * 2.3283064365386962890625e-10;
//        System.out.println("=====================JAVA======================");
//        System.out.println(method.getName() + "(): " + " energy consumed: " + consumed + "J");
//        System.out.println("=====================JAVA======================");
//    }

    private String getEnergy(int core) {
        String energyPath = "/sys/devices/platform/amd_energy.0/hwmon/hwmon5/energy" + core + "_input";
        String energyPath2 = "/sys/class/hwmon/hwmon5/energy" + core + "_input";
        System.out.println(energyPath2);
        try {
            String result = Files.readString(Paths.get(energyPath2));
            return "Energy consumed: " + result;

        } catch (IOException e) {

        }
        return "Energy consumed: ";
    }
}
