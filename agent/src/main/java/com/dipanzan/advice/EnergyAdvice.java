package com.dipanzan.advice;

import net.bytebuddy.asm.Advice;

public class EnergyAdvice {
    @Advice.OnMethodEnter(suppress = Throwable.class)
    public static long enter() {
        return System.nanoTime();
    }

    @Advice.OnMethodExit(suppress = Throwable.class, onThrowable = Throwable.class)
    public static void exit(@Advice.Enter long start, @Advice.Origin String origin) {
        String result = origin + " consumed " + "???" +" Joules during execution.";
        System.out.println(result);
    }

    public static Advice getAdvice() {
        return Advice.to(EnergyAdvice.class);
    }
}
