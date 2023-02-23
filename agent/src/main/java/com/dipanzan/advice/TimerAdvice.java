package com.dipanzan.advice;


import net.bytebuddy.asm.Advice;

import java.lang.reflect.Method;

import static net.bytebuddy.implementation.bytecode.assign.Assigner.Typing.DYNAMIC;

public class TimerAdvice {
    public static Advice getAdvice() {
        return Advice.to(TimerAdvice.class);
    }

    @Advice.OnMethodEnter(suppress = Throwable.class)
    public static long enter(@Advice.Origin Method method,
                             @Advice.AllArguments(typing = DYNAMIC) Object[] args) {
        return System.nanoTime();
    }

    @Advice.OnMethodExit(suppress = Throwable.class, onThrowable = Throwable.class)
    public static void exit(@Advice.Origin Method method, @Advice.Enter long start, @Advice.Origin String origin) {
        String result = origin + " took " + (System.nanoTime() - start) + "ns to execute";
        System.out.println(result);

    }
}
