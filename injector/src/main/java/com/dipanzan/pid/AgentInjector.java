package com.dipanzan.pid;

import com.sun.tools.attach.*;
import net.bytebuddy.agent.ByteBuddyAgent;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class AgentInjector {
    private static String targetPid;
    private static final String IN_PROGRESS = "[IN PROGRESS]: ";

    public static void main(String[] args) throws IOException {
        File jar = getAgentJar();
        tryAgentAttach(jar);
    }

    private static File getAgentJar() throws IOException {
        String filePath = new File(".").getCanonicalPath() + "/agent/target/" + "agent-1.0-SNAPSHOT.jar";
        return new File(filePath);
    }

    private static void tryAgentAttach(File jar) {
        System.out.println(IN_PROGRESS + "Initializing agent attachment:\n");
        while (true) {
            String pid = "";
            try {
                pid = scanPids();
                ByteBuddyAgent.attach(jar, pid);
                break;
            } catch (IllegalStateException e) {
                System.out.println("\nAgent failed to attach to [PID: " + pid + "]\n");
                e.printStackTrace();
                throw new RuntimeException(e);
            }
        }
    }

    private static void initPid() {
        List<VirtualMachineDescriptor> vmds = VirtualMachine.list();
//        vmds.forEach(System.out::println);
        targetPid = vmds
                .stream()
                .filter(AgentInjector::matchWithTargetClass)
                .findFirst()
                .orElseThrow()
                .id();
        System.out.println("Target PID: " + targetPid);
    }

    private static String scanPids() {
        List<String> pids = new ArrayList<>();
        Scanner sc = new Scanner(System.in);
        String pid;
        while (true) {
            VirtualMachine.list().forEach(t -> storeAndDisplayPids(pids, t));
            System.out.print("\n" + IN_PROGRESS + "Enter PID for attaching agent: ");
            pid = sc.nextLine();

            if (pids.contains(pid)) {
                System.out.println("Attaching agent to [PID: " + pid + "]");
                break;
            } else {
                System.out.println("\nPlease enter valid PID from list.");
            }
            System.out.println();
        }
        return pid;
    }

    private static void storeAndDisplayPids(List<String> pids, VirtualMachineDescriptor vmd) {
        pids.add(vmd.id());
        System.out.println(vmd.id() + ": " + vmd.displayName());
    }

    private static boolean matchWithTargetClass(VirtualMachineDescriptor vmd) {
        return vmd.displayName().startsWith("");
    }

    private static boolean attachAgentToVM() throws
            AgentLoadException,
            AttachNotSupportedException,
            AgentInitializationException,
            IOException {

        VirtualMachine vm = VirtualMachine.attach(targetPid);
        vm.loadAgent(null);
        vm.detach();
        return true;
    }
}