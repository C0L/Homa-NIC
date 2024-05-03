 /* Copyright (c) 2023 Homa Developers
 * SPDX-License-Identifier: BSD-1-Clause
 */

#include <stdio.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>

#include "dist.h"

/** @rand_gen: random number generator. */
static std::mt19937 rand_gen(std::chrono::system_clock::now().time_since_epoch().count());

int main(int argc, char**argv) {
	int max_message_length = 100000000;

	dist_point_gen generator(argv[1], max_message_length);

	for (size_t i = 0; i < 1'000'000; i++) {
		generator(rand_gen);
	}

	for (size_t i = 0; i < atoi(argv[2]); i++) {
		printf("%d\n", generator(rand_gen));
	}
}
