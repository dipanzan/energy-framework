package com.dipanzan;

import java.util.Scanner;

public class Matrix {
    private static int SIZE = 0;
    private final static int MAX = 10;
    private final static int MIN = 0;

    Scanner in = new Scanner(System.in);

    public static void main(String[] args) {
        System.out.println("Hello world!");

        new Matrix().run();
    }

    public void run() {
        do {
            initSize();
            int[][] a = new int[SIZE][SIZE];
            int[][] b = new int[SIZE][SIZE];

            initMatrix(a, MAX, MIN);
            initMatrix(b, MAX, MIN);

            int[][] c = matrixMultiply(a, b);
            printMatrix(c);
        } while (runAgain());
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

    private int[][] matrixMultiply(int[][] a, int[][] b) {
        int[][] c = new int[SIZE][SIZE];

        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                for (int k = 0; k < SIZE; k++) {
                    c[i][j] += a[i][k] * b[k][j];
                }
            }
        }
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
    }

    private int random(int max, int min) {
        return (int) (Math.random() * (max - min + 1)) + min;
    }
}