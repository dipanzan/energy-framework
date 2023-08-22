import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

public class Matrix {

    public static long readEnergy(int core) {
        try {
            String read = Files.readString(Paths.get("/sys/class/hwmon/hwmon5/energy" + core + "_input"));
            read = read.replace("\n", "");
            return Long.parseLong(read);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public static void printEnergyConsumed(long before, long after, String exp) {
        double result = (after - before) * 2.3283064365386962890625e-10;

        System.out.println("=====================JAVA======================");
        System.out.println("experiment: " + exp + " energy consumed: " + result + "J");
        System.out.println("=====================JAVA======================");
    }

    public static void main(String[] args) {
        if (args.length != 1) {
            throw new RuntimeException("matrix size not provided!");
        }

        int size = Integer.parseInt(args[0]);

        long before = readEnergy(1);
        
        int[][] a = initMatrix(size, 1, 999);
        int[][] b = initMatrix(size, 1, 999);
        long[][] c = matrixMultiply(size, a, b);

        long after = readEnergy(1);
        printEnergyConsumed(before, after, "main");
    }

    private static int random(int min, int max) {
        return (int) (Math.random() * (max - min + 1)) + min;
    }

    private static long[][] matrixMultiply(int size, int[][] a, int[][] b) {
        System.out.println("Multiplying matrix.");
        long[][] c = initZeroMatrix(size);

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                for (int k = 0; k < size; k++) {
                    c[i][j] += (long) a[i][k] * b[k][j];
                }
            }
        }
        System.out.println("Multiplication done!");
        return c;
    }

    private static long[][] initZeroMatrix(int size) {
        long[][] matrix = new long[size][size];
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                matrix[i][j] = 0;
            }
        }
        return matrix;
    }

    private static int[][] initMatrix(int size, int min, int max) {
        System.out.println("Populating matrix with random values [1-999].");

        int[][] matrix = new int[size][size];
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                matrix[i][j] = random(min, max);
            }
        }
        System.out.println("Matrix initialization done.");
        return matrix;
    }
}
