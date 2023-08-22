function read_energy(core) {
    const fs = require('fs');
    var path = "/sys/class/hwmon/hwmon5/energy" + core + "_input";

    return Number(fs.readFileSync(path, 'utf-8'));
}

function print_energy_consumed(before, after, exp) {
    result = (after - before) * 2.3283064365386962890625e-10;
    console.log("=====================JAVASCRIPT======================");
    console.log("experiment: " + exp + " energy consumed: " + result + "J");
    console.log("=====================JAVASCRIPT======================");
}
function get_random_num(min, max) {
    return Math.floor(Math.random() * max);
}

function init_matrix(size) {
    console.log("Populating matrix with random values [1-999].");
    var matrix = Array.from(Array(size), () => new Array(size));

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            matrix[i][j] = get_random_num(1, 999);
        }
    }
    console.log("Matrix initialization done.");
    return matrix;
}

function init_zero_matrix(size) {
    var matrix = Array.from(Array(size), () => new Array(size));

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            matrix[i][j] = 0;
        }
    }
    return matrix;
}

function multiply_matrix(size, a, b) {
    console.log("Multiplying matrix.");
    var c = init_zero_matrix(size);

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            for (k = 0; k < size; k++) {
                c[i][j] += (a[i][k] * b[k][j]);
            }
        }
    }
    console.log("Multiplication done!");
    return c;
}

function print_matrix(size, c) {
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            console.log(c[i][j]);
        }
    }
}

function get_matrix_size() {
    return Number(process.argv[2]);
}


const before = read_energy(1);

var size = get_matrix_size();
var a = init_matrix(size);
var b = init_matrix(size);
var c = multiply_matrix(size, a, b);
const after = read_energy(1);

print_energy_consumed(before, after, "main");

