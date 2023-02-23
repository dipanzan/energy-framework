package com.dipanzan.type;

import net.bytebuddy.description.method.MethodDescription;
import net.bytebuddy.description.type.TypeDescription;
import net.bytebuddy.matcher.ElementMatcher;

import static net.bytebuddy.matcher.ElementMatchers.*;

public class Types {

    private static final String AGENT_PACKAGE = "com.dipanzan.agent.";
    private static final String BYTEBUDDY_PACKAGE = "net.bytebuddy.";

    public static final ElementMatcher.Junction<TypeDescription> IGNORED =
            nameStartsWith(AGENT_PACKAGE)
                    .or(nameStartsWith(BYTEBUDDY_PACKAGE))
                    .or(nameStartsWith("com.sun.proxy"))
                    .or(nameStartsWith("java.instrument."))
                    .or(nameStartsWith("java.io"))
                    .or(nameStartsWith("java.lang"))
                    .or(nameStartsWith("java.lang.invoke"))
                    .or(nameStartsWith("java.lang.reflect."))
                    .or(nameStartsWith("java.nio").and(nameContains("Buffer")))
                    .or(nameStartsWith("java.nio.charset"))
                    .or(nameStartsWith("java.security."))
                    .or(nameStartsWith("java.util.").and(not(nameStartsWith("java.util.UUID"))))
                    .or(nameStartsWith("jdk.internal."))
                    .or(nameStartsWith("sun."))
                    .or(nameStartsWith("sun.reflect"))
                    .or(nameStartsWith("sun.security."))
                    .or(named("java.io.PrintStream"))
                    .or(named("java.lang.Character"))
                    .or(named("java.lang.Class"))
                    .or(named("java.lang.Integer"))
                    .or(named("java.lang.Long"))
                    .or(named("java.lang.Math"))
                    .or(named("java.lang.Object"))
                    .or(named("java.lang.PublicMethods"))
                    .or(named("java.lang.SecurityManager"))
                    .or(named("java.lang.String"))
                    .or(named("java.lang.StringBuilder"))
                    .or(named("java.lang.Throwable"))
                    .or(named("java.lang.WeakPairMap"))
                    .or(named("java.lang.ref.SoftReference"))
                    .or(named("java.util.Arrays"))
                    .or(named("java.util.HashMap"))
                    .or(named("java.util.Stack"))
                    .or(named("java.util.String"))
                    .or(nameEndsWith("Exception"))
                    .or(nameMatches(".*[.]instrument[.].*"))
                    .or(nameMatches("java[.]io[.].*Writer"))
                    .or(nameMatches("java[.]net[.]URL.*"));

    public static final ElementMatcher.Junction<MethodDescription> TARGET_METHODS =
            isMethod()
                    .and(not(nameContains("getMethod")))
                    .and(not(named("equals")))
                    .and(not(named("getChars")))
                    .and(not(named("getSecurityManager")))
                    .and(not(named("hashCode")))
                    .and(not(named("requireNonNull")))
                    .and(not(named("stringSize")))
                    .and(not(named("toString")))
                    .and(not(named("valueOf")));
}
