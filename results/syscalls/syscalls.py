import csv, subprocess

cmd = "/home/dipanzan/CProjects/stress-ng/stress-ng"
csv_path = "/home/dipanzan/IdeaProjects/energy-framework/kernel/results/"


def kernel_version(mitigations):
    kernel = subprocess.run("uname -r", shell=True, universal_newlines=True, stdout=subprocess.PIPE)
    return kernel.stdout.splitlines()[0] if mitigations else kernel.stdout.splitlines()[0] + "-mitigations-off"


def create_csv(mitigations, columns):
    f = open(csv_path + kernel_version(mitigations) + ".csv", 'w')
    writer = csv.writer(f)
    writer.writerow(columns)
    return writer


def read_syscalls_energy(taskset=4, syscall=1, timeout="10s"):
    while True:
        p = subprocess.run(
            f"{cmd} -q --abort --taskset {taskset} --syscall {syscall} --aggressive --maximize --ignite-cpu --timeout {timeout}",
            shell=True, universal_newlines=True, stdout=subprocess.PIPE)
        dataset = p.stdout.splitlines()[:-1]  # removing that WARNING syscall last line
        if (len(dataset) == 273):
            return dataset


def extract_syscalls_energy(dataset):
    row = []
    for line in dataset:
        _, energy = line.split(": ")
        row.append(energy)
    return row


def extract_syscalls_names(dataset):
    columns = []
    for line in dataset:
        syscall, _ = line.split(": ")
        columns.append(syscall)
    return columns


def run_experiments(mitigations=True, runs=1000):
    dataset = read_syscalls_energy()
    columns = extract_syscalls_names(dataset)
    csv_writer = create_csv(mitigations, columns)

    for i in range(runs):
        dataset = read_syscalls_energy()
        row = extract_syscalls_energy(dataset)
        csv_writer.writerow(row)


def main():
    run_experiments(False, 2000)


if __name__ == "__main__":
    main()
