namespace MatrixProgram
{
    class Matrix
    {
        static Int64 ReadEnergy(int core)
        {
            string path = "/sys/class/hwmon/hwmon5/energy" + core + "_input";
            Int64 value = Int64.Parse(File.ReadLines(path).First());
            return value;
        }

        static void PrintEnergyConsumed(Int64 before, Int64 after, string exp)
        {
            double result = (after - before) * 2.3283064365386962890625e-10;
            Console.WriteLine("=====================C#======================");
            Console.WriteLine("experiment: " + exp + " energy consumed: " + result + "J");
            Console.WriteLine("=====================C#======================");
        }


        static int GetRandomNumber(int min, int max)
        {
            Random random = new Random();
            return random.Next(min, max);
        }

        static int[,] InitMatrix(int size)
        {
            Console.WriteLine("Populating matrix with random values [1-999].");
            int[,] matrix = new int[size, size];

            for (int i = 0; i < size; i++)
            {
                for (int j = 0; j < size; j++)
                {
                    matrix[i, j] = GetRandomNumber(1, 999);
                }
            }
            Console.WriteLine("Matrix initialization done.");

            return matrix;
        }

        static int[,] InitZeroMatrix(int size)
        {
            int[,] matrix = new int[size, size];
            return matrix;
        }

        static void PrintMatrix(int size, int[,] matrix)
        {
            for (int i = 0; i < size; i++)
            {
                for (int j = 0; j < size; j++)
                {
                    Console.WriteLine(matrix[i, j]);
                }
            }
        }

        static int GetMatrixSize()
        {
            int size = Int32.Parse(Environment.GetCommandLineArgs()[1]);
            return size;
        }

        static int[,] MultiplyMatrix(int size, int[,] a, int[,] b)
        {
            int[,] c = InitZeroMatrix(size);

            Console.WriteLine("Multiplying matrix.");
            for (int i = 0; i < size; i++)
            {
                for (int j = 0; j < size; j++)
                {
                    for (int k = 0; k < size; k++)
                    {
                        c[i, j] += (a[i, k] * b[k, j]);
                    }
                }
            }
            Console.WriteLine("Multiplication done!");
            return c;
        }

        static void Main(string[] args)
        {
            long before = ReadEnergy(1);
            
            int size = GetMatrixSize();
            var a = InitMatrix(size);
            var b = InitMatrix(size);
            var c = MultiplyMatrix(size, a, b);

            long after = ReadEnergy(1);
            PrintEnergyConsumed(before, after, "main");

        }
    }
}