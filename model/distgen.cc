/* Copyright (c) 2023 Homa Developers
 * SPDX-License-Identifier: BSD-1-Clause
 */

#include <stdio.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>

#include <cmath>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "dist.h"

/** @rand_gen: random number generator. */
static std::mt19937 rand_gen(std::chrono::system_clock::now().time_since_epoch().count());

int main(int argc, char**argv) {

    int lfile = open(argv[3], O_RDWR | O_APPEND | O_CREAT, 0644);
                                         
    dist_point_gen generator(argv[1], 1000000);

    std::cerr << "mean: " << generator.get_mean() << std::endl;

    for (size_t i = 0; i < 1'000'000; i++) {
	generator(rand_gen);
    }

    std::cerr << "generating..." << std::endl;

    // uint32_t * lens = (uint32_t*) malloc(atol(argv[2]) * sizeof(uint32_t));
    // std::cerr << atol(argv[2]) << std::endl;
    uint64_t lim = atol(argv[2]);
    uint32_t lens[16384];
    for (uint64_t i = 0; i < lim; i++) {
	// Flush?
	if (i != 0 && (i % 16384 == 0 || i == lim-1)) {
	    write(lfile, lens, 16384 * sizeof(uint32_t));
	}

	float sample = generator(rand_gen);
	uint32_t length = ceil(sample / 64.0);
	lens[i % 16384] = length;
    }

    close(lfile);
}
