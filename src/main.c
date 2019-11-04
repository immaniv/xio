#define _XOPEN_SOURCE 600
#define _GNU_SOURCE 

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


double IOPs, MBps; 
double min_lat, max_lat, avg_lat;
double r_IOPs, r_MBps, r_avg_lat;
double w_IOPs, w_MBps, w_avg_lat;
pthread_mutex_t mutexsum, rmutex, wmutex;
void *status;
pthread_mutex_t *wait_mutex;
pthread_attr_t attr;
pthread_t *numthreads;
int exit_status = 0;
struct dev_opts iodev;
struct thread_opts *topts;
int r_err, w_err;

int main (int argc, char *argv[])
{
	int i;
	struct dev_opts *iodevlist;
	int read_threads, write_threads, tnum;

	read_threads = 0;
	write_threads = 0;
	
	iodev.bs = DEFAULT_BLOCK_SIZE;
	iodev.size = DEFAULT_DEV_SIZE;
	iodev.iter = DEFAULT_ITER;
	iodev.mode = N_READ;
	iodev.nthreads = DEFAULT_NTHREADS;
	iodev.type = SEQUENTIAL;
	iodev.read_ratio = DEFAULT_READ_RATIO;
	iodev.seq_ratio = DEFAULT_SEQ_RATIO;
	iodev.devselect = 0;
	iodev.indefinite = 0;
	iodev.verbose = 0;
	r_err = 0;
	w_err = 0;

	parse_args(argc, argv, &iodev);

	if (iodev.mode == MIXED) {
		read_threads = (int) ((iodev.nthreads * iodev.read_ratio)/100);
		write_threads = (int) iodev.nthreads - read_threads;
	}
#ifdef DEBUG
		fprintf(stdout, "READ Threads: %d, WRITE Threads: %d\n", read_threads, write_threads);
		fprintf(stdout, "IO Mode: %d\n", iodev.mode);
#endif


	topts = (struct thread_opts *) malloc (sizeof(struct thread_opts) * iodev.nthreads);
	
	if ((iodev.fd = open(iodev.devpath, O_RDWR|O_DIRECT)) < 0) {
		fprintf(stdout, "Unable to open device: %s\n", iodev.devpath);
		perror("open");
		exit(1);
	}

	iodev.buf = mmap(NULL, iodev.bs, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);

	if (iodev.buf == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	memset(iodev.buf, 0, iodev.bs);
        if ((mlock(iodev.buf, iodev.bs)) == -1) {
		perror("mlock");
		exit(1);
	}

	numthreads = (pthread_t *) malloc(sizeof(pthread_t)*iodev.nthreads);
	wait_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t)*iodev.nthreads);

	
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigterm_handler);
	signal(SIGKILL, sigkill_handler);

	pthread_mutex_init(&mutexsum, NULL);
	pthread_mutex_init(&rmutex, NULL);
	pthread_mutex_init(&wmutex, NULL);
         
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (tnum = 0; tnum < iodev.nthreads; tnum++)  {
		topts[tnum].thread_id = tnum;
		topts[tnum].opts = &iodev;

		if (iodev.mode == MIXED) {
			if (tnum < read_threads)	
				topts[tnum].t_mode = N_READ;
			else
				topts[tnum].t_mode = N_WRITE;
		} else {
			topts[tnum].t_mode = iodev.mode;
		}
#ifdef DEBUG
		fprintf(stdout, "Launching thread %d, mode = %c, type = %s\n", tnum, (topts[tnum].t_mode)?'W':'R', "TBD");
#endif
   		pthread_create(&numthreads[tnum], &attr, io_thread, (void *) &topts[tnum]); 
   	}
	

	pthread_attr_destroy(&attr);

	for(i = 0; i < iodev.nthreads; i++) {
		pthread_join(numthreads[i], &status);
  	}

	fprintf(stdout, "dev: %s | n_threads: %02d | mode: %s | type: %s | blksize: %d (B) | iops: %.02f | MB/s: %.02f | svc_time: %.02f (ms)\n",
		iodev.devpath, 
		iodev.nthreads, 
		(iodev.mode) ? ((iodev.mode == N_WRITE) ? "W":"M"):"R", 
		(iodev.type) ? ((iodev.type == 1) ? "RND":"MIX"):"SEQ",	
		iodev.bs, 
		IOPs,
		MBps,
		(avg_lat/1000.0)/iodev.nthreads);

	if (iodev.verbose) 	
		fprintf(stdout, "[READ] IO/s: %.02f, MB/s %.02f, Errors: %d\n[WRITE] IO/s: %.02f, MB/s %.02f, Errors: %d\n",
			r_IOPs, r_MBps, r_err, 
			w_IOPs, w_MBps, w_err);	

	cleanup(&mutexsum, &iodev);
	cleanup(&wmutex, NULL);
	cleanup(&rmutex, NULL); 
	pthread_exit(NULL);

	return 0;
}   
