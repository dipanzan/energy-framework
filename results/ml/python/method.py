def read_energy(core):
    path = "/sys/class/hwmon/hwmon5/energy" + str(core) + "_input"
    with open(path) as file:
        value = int(file.read())
        return value


def print_energy_consumed(before, after, exp):
    result = (after - before) * 2.3283064365386962890625e-10
    print("=====================PYTHON======================")
    print(exp + ": " + str(result) + "J")
    print("=====================PYTHON======================")


def execute(func, exp):
    before = read_energy(1)
    result = func()
    after = read_energy(1)
    print_energy_consumed(before, after, exp)
    return result