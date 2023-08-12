package com.dipanzan;

import com.dipanzan.annotation.Energy;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Scanner;

import static com.dipanzan.Method.printEnergyConsumed;
import static com.dipanzan.Method.readEnergy;

public class Matrix {
    private static int NUM_THEADS = 0;
    private static int TIME = 0;
    private static int SIZE = 8192;
    private final static int MAX = 10;
    private final static int MIN = 0;

    Scanner in = new Scanner(System.in);


    public Matrix(int threads, int time) {
        NUM_THEADS = threads;
        TIME = time;
    }

    private void load() {
//        long before = readEnergy(1);
        System.out.println(Thread.currentThread().getName() + " running");
        run();
//        long after = readEnergy(1);
//        printEnergyConsumed(1, before, after);

    }

    public void run2() {
        long before = readEnergy(1);
        List<Thread> threads = new ArrayList<>(NUM_THEADS);
        for (int i = 0; i < NUM_THEADS; i++) {
            threads.add(new Thread(this::load));
            threads.get(i).start();
        }

        for (int i = 0; i < TIME; i++) {
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                //
            }
        }
        threads.forEach(Thread::stop);
        long after = readEnergy(1);
        printEnergyConsumed(1, before, after);
    }

    public void run() {
//        initSize();
        int[][] a = new int[SIZE][SIZE];
        int[][] b = new int[SIZE][SIZE];

        initMatrix(a, MAX, MIN);
        initMatrix(b, MAX, MIN);

        long[][] c = matrixMultiply(a, b);
//        printMatrix(c);
//        do {
//        } while (runAgain());
    }

    private boolean runAgain() {
        System.out.print("Run again: [y|Y] or [n|N]: ");
        String input = in.nextLine().stripLeading().stripTrailing();
        return input.matches("y|Y|yes|Yes");
    }

    private void initSize() {
        System.out.print("Enter matrix size: ");
        SIZE = Integer.parseInt(in.nextLine());
    }

    private void printMatrix(int[][] matrix) {
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                System.out.print(matrix[i][j] + " ");
            }
            System.out.println("");
        }
    }

    private long[][] matrixMultiply(int[][] a, int[][] b) {
        System.out.println("Multiplying matrix.");
        long[][] c = new long[SIZE][SIZE];

        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                for (int k = 0; k < SIZE; k++) {
                    c[i][j] += (long) a[i][k] * b[k][j];
                }
            }
        }
        System.out.println("Multiplication done!");
        return c;
    }


    private void initMatrix(int[][] matrix, int max, int min) {

        System.out.println("Populating matrix with random values [0-999].");
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                matrix[i][j] = random(max, min);
            }
        }
        System.out.println("Matrix initialization done.");
        System.out.println("Thanks Man ;)");
    }

    private int random(int max, int min) {
        return (int) (Math.random() * (max - min + 1)) + min;
    }
}