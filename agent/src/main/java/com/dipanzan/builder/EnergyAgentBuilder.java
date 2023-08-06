package com.dipanzan.builder;

import com.dipanzan.advice.EnergyAdvice;
import com.dipanzan.type.Types;
import net.bytebuddy.agent.builder.AgentBuilder;
import net.bytebuddy.agent.builder.AgentBuilder.RedefinitionStrategy;
import net.bytebuddy.asm.Advice;

import java.lang.instrument.Instrumentation;

import static net.bytebuddy.matcher.ElementMatchers.any;
import static net.bytebuddy.matcher.ElementMatchers.none;

public class EnergyAgentBuilder {
    public static void init(Instrumentation inst) {
        installTransformer(inst);
        System.out.println("EnergyAgent installed");
    }

    private static void installTransformer(Instrumentation inst) {
        new AgentBuilder.Default()
                .disableClassFormatChanges()
                .with(RedefinitionStrategy.RETRANSFORMATION)
                .with(RedefinitionStrategy.Listener.StreamWriting.toSystemOut())
                .with(AgentBuilder.Listener.StreamWriting.toSystemError().withTransformationsOnly())
                .with(AgentBuilder.InstallationListener.StreamWriting.toSystemOut())
                .ignore(none())
                .ignore(Types.IGNORED)
                .type(any())
                .transform((builder, type, classLoader, module, protectionDomain) ->
                        builder.visit(Advice.to(EnergyAdvice.class).on(Types.TARGET_METHODS))
                )
                .installOn(inst);
    }
}