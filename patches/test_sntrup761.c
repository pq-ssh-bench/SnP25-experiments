#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "crypto_api.h"

int
testf() {
	unsigned char pk[crypto_kem_sntrup761_PUBLICKEYBYTES];
	unsigned char sk[crypto_kem_sntrup761_SECRETKEYBYTES];
	unsigned char c[crypto_kem_sntrup761_CIPHERTEXTBYTES];
	unsigned char k[crypto_kem_sntrup761_BYTES];

	crypto_kem_sntrup761_keypair(pk, sk);
	crypto_kem_sntrup761_enc(c, k, pk);
	return crypto_kem_sntrup761_dec(k, c, sk);
}

int
main(int argc, char **argv) {
	int default_reps = 100;
	int reps;
	if (argc > 1) {
		reps = atoi(argv[1]);
		if (reps <= 0) reps = default_reps;
	} else {
		reps = default_reps;
	}
	double times[reps];
	// need to skip the first (few) timings because they take longer (cpu)
	int timing_offset = 1;
	int ret = 0;

	for (int i = 0; i < reps + timing_offset; i++) {
		// start clock
		clock_t begin = clock();
		// test code
		ret += testf();
		// stop clock and record time
		clock_t end = clock();
		double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		if (i < timing_offset) continue;
		times[i - timing_offset] = time_spent;
		printf("%f\n", time_spent);
	}
	return ret;
}

