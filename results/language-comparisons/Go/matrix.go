package main

import (
	"fmt"
	"math/rand"
	"time"
);

func getRandomNumber(min int, max int) int {
	rand.Seed(time.Now().UnixNano())
	return rand.Intn(max - min) + min
}

func initMatrix(size int) [][]uint8 {
	matrix := [size][size]uint8{}
	return matrix
}

func main() {
    fmt.Println("Hello, World!")
	getRandomNumber(1, 999)
	initMatrix(10)
}	