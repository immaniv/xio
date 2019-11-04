#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "common.h"

void *io_thread (void *arg)
{
	
	extern pthread_mutex_t mutexsum, rmutex, wmutex;
	extern double IOPs, MBps;
	extern double r_IOPs, r_MBps, r_avg_lat, r_err;
	extern double w_IOPs, w_MBps, w_avg_lat, w_err;
	extern double min_lat, max_lat, avg_lat;

	struct dev_opts *n_iodev;
	struct timespec startt, endt, iostartt, ioendt;
	int n, iter, nbytes, id, myfd;
	double diff, iodiff;
	double lIOps, lMBps, lavg_lat;
	struct thread_opts *myopts;
	int total_extent;
	int lr_err, lw_err;

	myopts = (struct thread_opts *) arg;
	
	n_iodev = (struct dev_opts *) myopts->opts;
	id = (int) myopts->thread_id;

	/* 
	signal(SIGINT, sigint_handler);
        signal(SIGTERM, sigterm_handler);
        signal(SIGKILL, sigkill_handler);
	*/

	pthread_t myid;
	myid = pthread_self();
	myopts->tid = myid;

	myfd = dup(n_iodev->fd);

	total_extent = ((n_iodev->size*1024*1024)/n_iodev->bs);

#ifdef DEBUG
	fprintf(stdout, "Starting thread %d in mode %c, type %s\n", id, (myopts->t_mode)?'W':'R', "TBD");
#endif
	
RUN_INDEFINITELY:
	lr_err = 0;
	lw_err = 0;	
	lIOps = 0.0;
	lMBps = 0.0;
	lavg_lat = 0.0;
	diff = 0.0;
	iodiff = 0.0;
	n = 0;
        iter = 0;
        nbytes = 0;

	if (n_iodev->type == SEQUENTIAL) {
		clock_gettime(CLOCK_MONOTONIC, &startt);
		for (iter = 0; iter < n_iodev->iter; iter++) {
			lseek(myfd, 0, SEEK_SET);
			int total_extent = ((n_iodev->size*1024*1024)/n_iodev->bs);
	
			if (myopts->t_mode == N_READ) {
				for (n = 0; n < total_extent; n++) {
					clock_gettime(CLOCK_MONOTONIC, &iostartt);
					if ( nbytes = (read(myfd, n_iodev->buf, n_iodev->bs)) != n_iodev->bs) {
						lr_err++;
						perror("read");
					}
					clock_gettime(CLOCK_MONOTONIC, &ioendt);
					iodiff += (((ioendt.tv_sec*1000000)+(ioendt.tv_nsec/1000)) - \
				         	((iostartt.tv_sec*1000000)+(iostartt.tv_nsec/1000)));
				}
			} else if (myopts->t_mode == N_WRITE) {
				for (n = 0; n < total_extent; n++) {
					clock_gettime(CLOCK_MONOTONIC, &iostartt);
					if ( nbytes = (write(myfd, n_iodev->buf, n_iodev->bs)) != n_iodev->bs) { 
						lw_err++;
						perror("write");
					}
					clock_gettime(CLOCK_MONOTONIC, &ioendt);
					iodiff += (((ioendt.tv_sec*1000000)+(ioendt.tv_nsec/1000)) - \
				         	((iostartt.tv_sec*1000000)+(iostartt.tv_nsec/1000)));
				}
			}
			iodiff /= n;
		}
		clock_gettime(CLOCK_MONOTONIC, &endt);
	} else if (n_iodev->type == RANDOM) {
		clock_gettime(CLOCK_MONOTONIC, &startt);
		for (iter = 0; iter < n_iodev->iter; iter++) {
			lseek(myfd, 0, SEEK_SET);
			int total_extent = ((n_iodev->size*1024*1024)/n_iodev->bs);
			if (myopts->t_mode == N_READ) {
				for (n = 0; n < total_extent; n++) {
					lseek(myfd, (n_iodev->bs * (rand() % total_extent)), SEEK_SET);
					clock_gettime(CLOCK_MONOTONIC, &iostartt);
					if ( nbytes = (read(myfd, n_iodev->buf, n_iodev->bs)) != n_iodev->bs) {
						lr_err++;
						perror("read");
					}
					clock_gettime(CLOCK_MONOTONIC, &ioendt);
					iodiff += (((ioendt.tv_sec*1000000)+(ioendt.tv_nsec/1000)) - \
				         	((iostartt.tv_sec*1000000)+(iostartt.tv_nsec/1000)));
				}
			} else if (myopts->t_mode == N_WRITE) {
				for (n = 0; n < total_extent; n++) {
					lseek(myfd, (n_iodev->bs * (rand() % total_extent)), SEEK_SET);
					clock_gettime(CLOCK_MONOTONIC, &iostartt);
					if ( nbytes = (write(myfd, n_iodev->buf, n_iodev->bs)) != n_iodev->bs) { 
						lw_err++;
						perror("write");
					}
					clock_gettime(CLOCK_MONOTONIC, &ioendt);
					iodiff += (((ioendt.tv_sec*1000000)+(ioendt.tv_nsec/1000)) - \
				         	((iostartt.tv_sec*1000000)+(iostartt.tv_nsec/1000)));
				}
			}
			iodiff /= n;
		}
		clock_gettime(CLOCK_MONOTONIC, &endt);
	}

	diff = (((endt.tv_sec*1000000)+(endt.tv_nsec/1000)) - \
	       ((startt.tv_sec*1000000)+(startt.tv_nsec/1000)))/1000000.0;

	lIOps = n/(diff/iter); 
	lMBps = ((n/(diff/iter))*n_iodev->bs)/(1024*1024);
	lavg_lat = (iodiff/iter);
	
	if (n_iodev->verbose || n_iodev->indefinite) {
		fprintf(stdout, "thread id: %d | blksize: %d (B) | mode: %s | type: %s | iops: %.02f | MB/s: %.02f | svc_time: %.2f (ms)\n", \
			id, \
			n_iodev->bs, \
			(myopts->t_mode) ? "W":"R", \
			(n_iodev->type)?((n_iodev->type == 1)?"RND":"MIX"):"SEQ", \
			lIOps, \
			lMBps, \
			lavg_lat/1000);
	}
	
	if (n_iodev->indefinite) 
		goto RUN_INDEFINITELY;

	pthread_mutex_lock(&mutexsum);

	if (myopts->t_mode == N_READ) {
		pthread_mutex_lock(&rmutex);
		r_IOPs += lIOps;
		r_MBps += lMBps;
		r_avg_lat += lavg_lat;
		r_err += lr_err;
	} else  {
		pthread_mutex_lock(&wmutex);
		w_IOPs += lIOps;
		w_MBps += lMBps;
		w_avg_lat += lavg_lat;
		w_err += lw_err;
	}

	IOPs += lIOps;
	MBps += lMBps;
	avg_lat += lavg_lat;
	
	if (myopts->t_mode == N_READ) 
		pthread_mutex_unlock(&rmutex);
	else 
		pthread_mutex_unlock(&wmutex);

	pthread_mutex_unlock(&mutexsum);

	pthread_exit((void*) 0);	

}
