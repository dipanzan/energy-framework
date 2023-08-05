package com.dipanzan.advice;


import net.bytebuddy.asm.Advice;

import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Paths;

import static net.bytebuddy.implementation.bytecode.assign.Assigner.Typing.DYNAMIC;

public class TimerAdvice {
    public static Advice getAdvice() {
        return Advice.to(TimerAdvice.class);
    }

    private static long before = 0;
    private static long after = 0;

    @Advice.OnMethodEnter(suppress = Throwable.class)
    public static long enter(@Advice.Origin Method method,
                             @Advice.AllArguments(typing = DYNAMIC) Object[] args) throws Exception {

        String energyPath = "/sys/class/hwmon/hwmon5/energy1" + "_input";
        String result = Files.readString(Paths.get(energyPath));
        before = Long.parseLong(result);
        System.out.println("before: " + before);
//        try {
//            String result = Files.readString(Paths.get(energyPath2));
//            System.out.println("Before: " + result);
//
//        } catch (IOException e) {
//
//        }
        return 0;

    }

    @Advice.OnMethodExit(suppress = Throwable.class, onThrowable = Throwable.class)
    public static void exit(@Advice.Origin Method method, @Advice.Enter long start, @Advice.Origin String origin) throws Exception {

        String energyPath = "/sys/class/hwmon/hwmon5/energy1" + "_input";
        String result = Files.readString(Paths.get(energyPath));
        after = Long.parseLong(result);

        double consumed = (after - before) * 2.3283064365386962890625e-10;
        System.out.println("=====================JAVA======================");
        System.out.println("Core: " + " energy consumed: " + consumed + "J");
        System.out.println("=====================JAVA======================");
    }

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
