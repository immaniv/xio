#define N_READ   0
#define N_WRITE  1
#define N_MIXED  2

#define DEFAULT_BLOCK_SIZE 	65536
#define DEFAULT_DEV_SIZE	16
#define DEFAULT_IO_MODE		N_READ
#define DEFAULT_ITER		16
#define DEFAULT_NTHREADS	8
#define DEFAULT_READ_RATIO	100
#define DEFAULT_SEQ_RATIO	100

#define SEQUENTIAL		0
#define	RANDOM			1
#define	MIXED			2


struct dev_opts {
	int fd;
	char *devpath;
	int bs;
	int size;
	int mode;
	int iter;
	int nthreads;
	char *buf;
	int devselect;
	int indefinite;
	int type;
	int read_ratio;
	int seq_ratio;
	int total_extents;
	int verbose;
};

struct thread_opts {
	struct dev_opts *opts;
	int thread_id;
	int t_type;
	int t_mode;
	pthread_t tid;
};

void sigint_handler();
void sigkill_handler();
void sigterm_handler();
void usage(void);
void parse_args(int argc, char **argv, struct dev_opts *opts);
void cleanup(pthread_mutex_t *mutex, struct dev_opts *opts);
void *io_thread (void *arg);
int io_cmd (int device_fd, char *io_buf, int bufsize, int type);
