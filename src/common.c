#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <string.h>
#include <ctype.h>
#include <sys/statvfs.h>
#include <signal.h>

#include "common.h"

char HELP[] = "\
-d $DEVICE - /dev/ path to the device\n\
-b $BLKSIZE - in bytes (default is 65536 bytes - min is 512 bytes, max is 4194304 bytes)\n\
-s $DEVSIZE - in MB (default is 16 MB - min 4 MB)\n\
-n $THREADS - number of IO threads (default is 8 - min is 1, max is 32)\n\
-i $ITERATIONS - number of times to run (default is 16 - min is 16)\n\
-t $TYPE - S for SEQUENTIAL (default), R for RANDOM\n\
-m $MODE - R for READ (default), W for WRITE, M for MIXED\n\
-M $MODE_RATIO - ratio of READ mode IO (default is 100)\n\
-I run indefinitely and report periodic stats\n\
-h this help\n\n";

/* 
Future Options

-T $TYPE_RATIO - ratio of SEQUENTIAL type IO (default is 100)\n\
-t $TYPE - S for SEQUENTIAL (default), R for RANDOM, M for MIXED\n\
*/

void cleanup (pthread_mutex_t *mutex, struct dev_opts *opts)
{
	pthread_mutex_destroy(mutex);
	if (opts != NULL) {
        	close(opts->fd); 
		munmap(opts->buf, opts->bs);
	}
}


void sigint_handler()
{
	sigterm_handler();
}

void sigterm_handler()
{
	int n;
	extern struct dev_opts iodev;
	extern pthread_mutex_t mutexsum;
	extern struct thread_opts *topts;
	fprintf(stdout, "Abort received. Cleaning up ..\n");
	for (n = 0; n < iodev.nthreads; n++) {
		fprintf(stdout, "Cancelling thread: %d\n", n);
		pthread_cancel(topts[n].tid);
	}
	cleanup(&mutexsum, &iodev);
	exit(2);
}

void sigkill_handler()
{
	sigterm_handler();
}

void usage (void) 
{
	extern char *__progname;
	fprintf(stdout, "Usage: %s -d $DEVICE -b $BLOCK_SIZE -s $DEVICE_SIZE -n $IO_THREADS\n%s", __progname, HELP);
}
void parse_args (int argc, char **argv, struct dev_opts *opts)
{
	int carg;

	if (argc < 2) {
		usage();
		exit(1);
	}

	opterr = 0;
	
	while ((carg = getopt(argc, argv, "hIvd:m:b:t:s:n:i:M:T:")) != -1)
		switch (carg) {
			case 'M':
				opts->read_ratio = atoi(optarg);
				if (opts->read_ratio > 100 || opts->read_ratio < 0) {
					usage();
					exit(1);
				}
				break;
			case 'T':
				opts->seq_ratio = atoi(optarg);
				if (opts->seq_ratio > 100 || opts->seq_ratio < 0) {
					usage();
					exit(1);
				}
				break;
			case 'd':
				opts->devpath = optarg;
				opts->devselect = 1;
				break;
			case 'b':
				opts->bs = atoi(optarg);
				if (opts->bs < 512 || opts->bs > (1024*4096)) {
					usage();
					exit(1);
				}
				break;
			case 's':
				opts->size = atoi(optarg);
				if (opts->size < 4) {
					usage();
					exit(1);
				}
				break;
			case 'n':
				opts->nthreads = atoi(optarg);
				if (opts->nthreads > 32 || opts->nthreads < 1) {
					usage();
					exit(1);
				}
				break;	
			case 'i':
				opts->iter = atoi(optarg);
				if (opts->iter < 16) {
					usage();
					exit(1);
				}
				break;
			case 'I':
				opts->indefinite = 1;
				break;
			case 'm':
				if (optarg[0] == 'R')
					opts->mode = N_READ;
				else if (optarg[0] == 'W')
					opts->mode = N_WRITE;
				else if (optarg[0] == 'M')
					opts->mode = MIXED;
				else {
					usage();
					exit(1);
				}
				break;
			case 't':
				if (optarg[0] == 'S' || optarg[0] == 's') 
					opts->type = SEQUENTIAL;
				else if (optarg[0] == 'R' || optarg[0] == 'r')
					opts->type = RANDOM;
				else if (optarg[0] == 'M' || optarg[0] == 'm')
					opts->type = MIXED;
				else {
					usage();
					exit(1);
				}
				break;
			case 'h':
				usage();
				exit(1);
			case 'v':
				opts->verbose = 1;
				break;
			default:
				usage();
				exit(1);
		}

	if (!opts->devselect) {
		usage();
		exit(1);
	}
			
}
