
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

public class Method {
    @FunctionalInterface
    public interface Execute {
        Object execute();
    }

    public static Object execute(Execute e, String exp) {
        Object o;
        long before = readEnergy(1);
        o = e.execute();
        // oops
        long after = readEnergy(1);
        printEnergyConsumed(before, after, exp);
        return o;
    }

    public static void printEnergyConsumed(long before, long after, String exp) {
        double result = (after - before) * 2.3283064365386962890625e-10;
        System.out.println("\n=======================JAVA===========================");
        System.out.println(exp + ": " + result + "J");
        System.out.println("=======================JAVA===========================");
    }

    public static long readEnergy(int core) {
        try {
            String read = Files.readString(Paths.get("/sys/class/hwmon/hwmon5/energy" + core + "_input"));
            read = read.replace("\n", "");
            return Long.parseLong(read);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
