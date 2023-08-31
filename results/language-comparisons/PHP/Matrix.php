<?php

function read_energy($core)
{
    $path = "/sys/class/hwmon/hwmon5/energy" . $core . "_input";
    $file = fopen($path, "r") or die("unable to read energy");
    $value = fread($file, filesize($path)) + 0.0;
    return $value;
}

function print_energy_consumed($before, $after, $exp)
{
    $result = ($after - $before) * 2.3283064365386962890625e-10;
    echo ("=====================PHP======================\n");
    echo ("experiment: " . $exp . " energy consumed: " . strval($result) . "J"."\n");
    echo ("=====================PHP======================\n");
}
function get_random_num($min, $max)
{
    return mt_rand($min, $max);
}

function init_random_matrix($size)
{
    echo ("Populating matrix with random values [1-999].\n");
    $matrix = array(array($size));
    for ($i = 0; $i < $size; $i++) {
        for ($j = 0; $j < $size; $j++) {
            $matrix[$i][$j] = get_random_num(1, 999);
        }
    }
    echo ("Matrix initialization done!\n");
    return $matrix;
}

function init_zero_matrix($size)
{
    return array_fill(0, $size, array_fill(0, $size, 0));
}

function get_matrix_size()
{
    $argv = $_SERVER['argv'];
    return intval($argv[1]);
}

function print_matrix($size, $matrix)
{
    for ($i = 0; $i < $size; $i++) {
        for ($j = 0; $j < $size; $j++) {
            echo (($matrix[$i][$j]) . "\n");
        }
    }
}
function multiply_matrix($size, $a, $b)
{

    $c = init_zero_matrix($size);
    echo ("Multiplying matrix.\n");
    for ($i = 0; $i < $size; $i++) {
        for ($j = 0; $j < $size; $j++) {
            for ($k = 0; $k < $size; $k++) {
                $c[$i][$j] += ($a[$i][$k] * $b[$k][$j]);
            }
        }
    }
    echo ("Multiplication done!\n");
    return $c;
}


$before = read_energy(1);

$size = get_matrix_size();
$a = init_random_matrix($size);
$b = init_random_matrix($size);
$c = multiply_matrix($size, $a, $b);

$after = read_energy(1);
print_energy_consumed($before, $after, "multiply_matrix");

?>