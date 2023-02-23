package com.dipanzan.advice;

import com.dipanzan.CompleteSTE;
import net.bytebuddy.asm.Advice;
import net.bytebuddy.asm.Advice.AllArguments;
import net.bytebuddy.asm.Advice.Origin;

import java.lang.reflect.Method;
import java.lang.reflect.Type;
import java.util.Stack;

import static net.bytebuddy.implementation.bytecode.assign.Assigner.Typing.DYNAMIC;
import static com.dipanzan.TraceStackInfo.STACKS;

public class TraceAdvice {
  @Advice.OnMethodEnter
  static void onEnter(
    @Origin Method method,
    @AllArguments(typing = DYNAMIC) Object[] args
  )
  {
    System.out.println("[+] ".concat(method.toString()));
    Type[] ts = method.getGenericParameterTypes();
    String[] ts_str = new String[ts.length];
    String[] args_str = new String[args.length];
    for (int i = 0; i < args.length; i++) {
      ts_str[i] = ts[i].getTypeName();
      args_str[i] = args[i].toString();
    }
    String currentThread = Thread.currentThread().getName();
    if (!STACKS.containsKey(currentThread))
      STACKS.put(currentThread, new Stack<>());
    STACKS.get(currentThread).add(new CompleteSTE(method.getDeclaringClass().getName(), method.getName(), ts_str, args_str));
  }

  @Advice.OnMethodExit
  static void onExit(@Origin Method method) {
    System.out.println("[-] ".concat(method.toString()));
    String currentThread = Thread.currentThread().getName();
    STACKS.get(currentThread).pop();
  }
}
