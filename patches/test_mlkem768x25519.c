#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include "ssh-sk.h"
#include "sshkey.h"
#include "kex.h"
#include "sshbuf.h"
#include "digest.h"
#include "crypto_api.h"

#define OQS_ENABLE_KEM_ml_kem_768 1
#include "liboqs/include/oqs/kem_ml_kem.h"

int
testf() {
	int ret;
	struct kex * kex = kex_new();
	kex->hash_alg = 2;
	kex_kem_mlkem768x25519_keypair(kex);

	const struct sshbuf * client_blob = kex->client_pub;
	struct sshbuf ** server_blobp = (struct sshbuf **) malloc(sizeof (struct sshbuf **));
	struct sshbuf ** server_shared_secretp = (struct sshbuf **) malloc(sizeof (struct sshbuf **));
	kex_kem_mlkem768x25519_enc(kex, client_blob, server_blobp, server_shared_secretp);

	const struct sshbuf * server_blob = *server_blobp;
	struct sshbuf ** client_shared_secretp = (struct sshbuf **) malloc(sizeof (struct sshbuf **));
	ret = kex_kem_mlkem768x25519_dec(kex, server_blob, client_shared_secretp);

	// free
	return ret;
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

