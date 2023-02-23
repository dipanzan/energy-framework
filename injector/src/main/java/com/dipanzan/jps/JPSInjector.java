package com.dipanzan.jps;

import net.bytebuddy.agent.ByteBuddyAgent;

import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Consumer;

public class JPSInjector {
    private static final String USAGE =
            "Agent attacher arguments: <agentJar> <processId|mainClassRegex|mainJarRegex>";

    public static void main(String[] args) throws IOException, InterruptedException {
        if (args.length == 0)
            System.out.println(USAGE);
        else if (args.length != 2)
            System.err.println(USAGE);
        else
            System.exit(attachAgent(args));
    }

    private static int attachAgent(String[] args) throws IOException, InterruptedException {
        File agentJar = new File(args[0]);
        String processId = args[1].matches("[0-9]+") ? args[1] : getProcessIdFromJPS(args);
        if ("0".equals(processId)) {
            System.err.println("Cannot find process matching regex: " + args[1]);
            return 1;
        }
        if (!agentJar.isFile()) {
            System.err.println("agentJar does not exist: " + agentJar);
            return 2;
        }
        System.out.println("Attaching agent " + agentJar + " to PID " + processId + "...");
        try {
            ByteBuddyAgent.attach(agentJar, processId);
        } catch (Exception e) {
            System.out.println("Failed");
            e.printStackTrace();
            return 3;
        }
        System.out.println("Done");
        return 0;
    }

    private static String getProcessIdFromJPS(String[] args) throws IOException, InterruptedException {
        Process process = startJPS();
        final AtomicInteger jpsPID = new AtomicInteger();

        // See https://www.baeldung.com/run-shell-command-in-java
        StreamGobbler streamGobbler = new StreamGobbler(process.getInputStream(), line -> {
            //System.out.println(line);
            String[] columns = line.split("[ \t]+");
            if (columns.length >= 2 && columns[1].matches(args[1]))
                jpsPID.set(Integer.parseInt(columns[0]));
        });
        ExecutorService executorService = Executors.newSingleThreadExecutor();
        executorService.submit(streamGobbler);
        int exitCode = process.waitFor();
        assert exitCode == 0;

        executorService.shutdown();
        return jpsPID.toString();
    }

    /**
     * See https://www.baeldung.com/run-shell-command-in-java
     */
    private static Process startJPS() throws IOException {
        boolean isWindows = System.getProperty("os.name").toLowerCase().startsWith("windows");
        ProcessBuilder processBuilder = new ProcessBuilder();
        if (isWindows)
            processBuilder.command("cmd.exe", "/c", "jps");
        else
            processBuilder.command("sh", "-c", "jps");
        return processBuilder.start();
    }

    /**
     * See https://www.baeldung.com/run-shell-command-in-java
     */
    private static class StreamGobbler implements Runnable {
        private InputStream inputStream;
        private Consumer<String> consumer;

        public StreamGobbler(InputStream inputStream, Consumer<String> consumer) {
            this.inputStream = inputStream;
            this.consumer = consumer;
        }

        @Override
        public void run() {
            new BufferedReader(new InputStreamReader(inputStream)).lines()
                    .forEach(consumer);
        }
    }

}
