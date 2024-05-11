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

    dist_point_gen generator(argv[1], 0, 0);

    for (size_t i = 0; i < 1'000'000; i++) {
	generator(rand_gen);
    }

    for (size_t i = 0; i < atoi(argv[2]); i++) {
	float sample = generator(rand_gen);
	uint32_t length = ceil(sample / 64.0);
	write(lfile, &length, sizeof(uint32_t));
    }

    close(lfile);
}