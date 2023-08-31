import random
import sys

def read_energy(core):
    path = "/sys/class/hwmon/hwmon5/energy" + str(core) + "_input"
    with open(path) as file:
        value = int(file.read())
        return value

def print_energy_consumed(before, after, exp):
    result = (after - before) * 2.3283064365386962890625e-10
    print("=====================PYTHON======================")
    print("experiment: " + exp + " energy consumed: " + str(result) + "J")
    print("=====================PYTHON======================")

def random_num(min, max):
    return random.randint(min, max)

def init_matrix(size, min, max):
    print("Populating matrix with random values [1-999].")
    matrix = []
    for i in range(size):
        matrix.append([])
        for j in range(size):
            matrix[i].append(random_num(min, max))
    print("Matrix initialization done!")
    return matrix


def init_empty_matrix(size):
    matrix = []
    for i in range(size):
        matrix.append([])
        for j in range(size):
            matrix[i].append(0)
    return matrix


def multiply_matrix(size, a, b):
    print("Multiplying matrix.")
    matrix = init_empty_matrix(size)
    for i in range(size):
        for j in range(size):
            for k in range(len(b)):
                matrix[i][j] += a[i][k] * b[k][j]
    print("Multiplication done!")
    return matrix


def main():
    if len(sys.argv) != 2:
        raise Exception("matrix size not provided!")
    size = int(sys.argv[1])

    a = init_matrix(size, 1, 999)
    b = init_matrix(size, 1, 999)
    multiply_matrix(size, a, b)


if __name__ == '__main__':
    before = read_energy(1)
    main()
    after = read_energy(1)

    print_energy_consumed(before, after, "main")
