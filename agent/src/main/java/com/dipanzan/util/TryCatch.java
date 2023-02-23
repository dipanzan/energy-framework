package com.dipanzan.util;
public class TryCatch {
    @FunctionalInterface
    public interface MethodThrowingException<T> {
        T execute() throws Exception;
    }
    public static <T> T execute(MethodThrowingException<T> mte) {
        try {
            return mte.execute();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}