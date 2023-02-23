package com.dipanzan.agent;

import com.dipanzan.advice.EnergyAdvice;
import com.dipanzan.advice.TimerAdvice;
import com.dipanzan.util.TryCatch;
import net.bytebuddy.agent.builder.AgentBuilder;
import net.bytebuddy.asm.Advice;
import net.bytebuddy.description.method.MethodDescription;
import net.bytebuddy.description.type.TypeDescription;
import net.bytebuddy.dynamic.ClassFileLocator;
import net.bytebuddy.dynamic.loading.ClassInjector;
import net.bytebuddy.matcher.ElementMatcher;
import net.bytebuddy.matcher.ElementMatchers;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.lang.instrument.Instrumentation;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.*;
import java.util.jar.JarFile;

public class Agent {
    private static Instrumentation instrumentation;

    public static void premain(String agentArgs, Instrumentation inst) {
        defaultmain(agentArgs, inst);
    }

    public static void agentmain(String agentArgs, Instrumentation inst) {
        defaultmain(agentArgs, inst);
    }

    public static void defaultmain(String agentArgs, Instrumentation inst) {
        instrumentation = inst;
        JarFile agentJar = generateAgentJar();
        instrumentation.appendToBootstrapClassLoaderSearch(agentJar);
        TryCatch.execute(Agent::invokeAgent);
    }


    private static JarFile generateAgentJar() {
        try (InputStream is = getInputStreamFor("agent-deps.jar")) {
            Path path = copyFromShadedJarFrom(is);
            return createAgentJarFrom(path);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private static InputStream getInputStreamFor(String fileName) {
        return Agent.class.getClassLoader().getResourceAsStream(fileName);
    }

    private static Path copyFromShadedJarFrom(InputStream is) throws IOException {
        Path path = Files.createTempFile(null, ".jar");
        Files.copy(is, path, StandardCopyOption.REPLACE_EXISTING);
        path.toFile().deleteOnExit();
        Runtime.getRuntime().addShutdownHook(new Thread(() -> path.toFile().delete()));
        return path;
    }

    private static JarFile createAgentJarFrom(Path path) throws IOException {
        return new JarFile(path.toFile());
    }

    private static boolean invokeAgent() throws ClassNotFoundException, NoSuchMethodException, InvocationTargetException, IllegalAccessException {
        Class.forName("com.dipanzan.builder.EnergyAgentBuilder")
                .getMethod("init", Instrumentation.class)
                .invoke(null, instrumentation);
        return true;
    }

    public static AgentBuilder simpleAgentBuilder() {
        ClassInjector classInjector = ClassInjector.UsingInstrumentation.of(new File(""), ClassInjector.UsingInstrumentation.Target.BOOTSTRAP, instrumentation);
        injectToBootloader(Agent.class);

        return new AgentBuilder.Default()
                .disableClassFormatChanges()
                .with(AgentBuilder.RedefinitionStrategy.RETRANSFORMATION)
                .with(AgentBuilder.RedefinitionStrategy.Listener.StreamWriting.toSystemOut())
                .with(AgentBuilder.InjectionStrategy.UsingUnsafe.INSTANCE)
                .with(new AgentBuilder.InjectionStrategy.UsingInstrumentation(instrumentation, new File("")))
                .ignore(ElementMatchers.none())
                .type(ElementMatchers.any())
//                .transform(new AgentBuilder.Transformer.ForAdvice().advice())
                .transform((builder, typeDescription, classLoader, javaModule, protectionDomain) ->
                        builder.visit(Advice.to(TimerAdvice.class).on(ElementMatchers.isMethod())));
    }

    private static Map<TypeDescription, Class<?>> injectToBootloader(Class<?> clazz) {
        Map<TypeDescription.ForLoadedType, byte[]> injectables = Collections.singletonMap(
                new TypeDescription.ForLoadedType(clazz),
                ClassFileLocator.ForClassLoader.read(clazz));

        return TryCatch.execute(() -> ClassInjector.UsingUnsafe.ofBootLoader().inject(injectables));
    }


    private static AgentBuilder configureAgentBuilder() {
        return new AgentBuilder.Default()
                .disableClassFormatChanges()
                .with(AgentBuilder.RedefinitionStrategy.RETRANSFORMATION)
                .with(AgentBuilder.RedefinitionStrategy.Listener.StreamWriting.toSystemOut())
                .with(AgentBuilder.Listener.StreamWriting.toSystemOut().withTransformationsOnly())
                .with(AgentBuilder.InstallationListener.StreamWriting.toSystemOut())
                .with(AgentBuilder.TypeStrategy.Default.DECORATE)
                .with(AgentBuilder.DescriptionStrategy.Default.POOL_FIRST)
                .ignore(ElementMatchers.none())
                .ignore(ElementMatchers.nameStartsWith("net.bytebuddy.")
                        .or(ElementMatchers.nameStartsWith("jdk.internal.reflect."))
                        .or(ElementMatchers.nameStartsWith("java.lang.invoke."))
                        .or(ElementMatchers.nameStartsWith("com.sun.proxy.")))
                .disableClassFormatChanges()
                .with(AgentBuilder.RedefinitionStrategy.RETRANSFORMATION)
                .with(AgentBuilder.InitializationStrategy.NoOp.INSTANCE)
                .with(AgentBuilder.TypeStrategy.Default.REDEFINE)
                .type(ElementMatchers.any())
                .transform((builder, typeDescription, classLoader, javaModule, protectionDomain) ->
                        builder.visit(TimerAdvice.getAdvice().on(ElementMatchers.isMethod()))
                );
    }

    private static AgentBuilder configureAgentBuilder2() {
        return new AgentBuilder.Default()
                .disableClassFormatChanges()
                .with(AgentBuilder.RedefinitionStrategy.RETRANSFORMATION)
                .with(AgentBuilder.RedefinitionStrategy.Listener.StreamWriting.toSystemError())
                .with(AgentBuilder.Listener.StreamWriting.toSystemError().withTransformationsOnly())
                .with(AgentBuilder.InstallationListener.StreamWriting.toSystemError())
                .ignore(ElementMatchers.none())
                .ignore(ElementMatchers.nameStartsWith("net.bytebuddy.")
                        .or(ElementMatchers.nameStartsWith("jdk.internal.reflect."))
                        .or(ElementMatchers.nameStartsWith("java.lang.invoke."))
                        .or(ElementMatchers.nameStartsWith("com.sun.proxy.")))
                .disableClassFormatChanges()
//                .with(AgentBuilder.InjectionStrategy.UsingInstrumentation)
                .with(AgentBuilder.RedefinitionStrategy.RETRANSFORMATION)
                .with(AgentBuilder.InitializationStrategy.NoOp.INSTANCE)
                .with(AgentBuilder.TypeStrategy.Default.REDEFINE)
                .type(ElementMatchers.any())
                .transform((builder, typeDescription, classLoader, javaModule, protectionDomain) ->
                        builder.visit(Advice.to(EnergyAdvice.class).on(ElementMatchers.isMethod()))
                );
    }

    private static ElementMatcher.Junction<? super MethodDescription> startsWith(String prefix) {
        return ElementMatchers.isMethod().and(ElementMatchers.nameStartsWith(prefix));
    }

    private static Class<?> findTargetClass() {
        return findTargetClassFromName()
                .or(Agent::findTargetClassFromLoadedClasses)
                .orElseThrow();
    }

    private static Optional<Class<?>> findTargetClassFromName() {
        try {
            return Optional.of(Class.forName(null));
        } catch (ClassNotFoundException cnfe) {
            throw new RuntimeException(cnfe);
        }
    }

    private static Optional<Class<?>> findTargetClassFromLoadedClasses() {
        Class<?> clazz = Arrays.stream(instrumentation.getAllLoadedClasses())
                .filter(Agent::findTargetClassFromName)
                .findFirst()
                .orElseThrow();
        return Optional.of(clazz);
    }

    private static boolean findTargetClassFromName(Class<?> klazz) {
        return klazz.getName().equals("");
    }

    private static List<Method> findMethodsAnnotatedWithEnergyAnnotation(Class<?> clazz) {
        return Arrays.stream(clazz.getMethods())
                .filter(Agent::isEnergyAnnotationPresent)
                .toList();
    }

    static boolean isEnergyAnnotationPresent(Method method) {
//        return method.isAnnotationPresent(Energy.class);
        return true;
    }

}
