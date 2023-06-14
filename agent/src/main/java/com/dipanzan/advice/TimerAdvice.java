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

    @Advice.OnMethodEnter(suppress = Throwable.class)
    public static long enter(@Advice.Origin Method method,
                             @Advice.AllArguments(typing = DYNAMIC) Object[] args) throws Exception{

        int core = 1;
        String energyPath = "/sys/devices/platform/amd_energy.0/hwmon/hwmon5/energy" + core + "_input";
        String energyPath2 = "/sys/class/hwmon/hwmon5/energy" + core + "_input";
        String result = Files.readString(Paths.get(energyPath2));
        System.out.println(result);
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
    public static void exit(@Advice.Origin Method method, @Advice.Enter long start, @Advice.Origin String origin) {

       /* int core = 1;
        String energyPath = "/sys/devices/platform/amd_energy.0/hwmon/hwmon5/energy" + core + "_input";
        String energyPath2 = "/sys/class/hwmon/hwmon5/energy" + core + "_input";
        try {
            String result = Files.readString(Paths.get(energyPath2));
            System.out.println("After: " + result);

        } catch (IOException e) {

        }*/
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
