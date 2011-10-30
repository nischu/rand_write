#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

int clos = 0;
unsigned long long int total = 0;
time_t start = 0;

struct thread_data {
	char *file;
	int fd;
};

void alrm(int sig) {
	alarm(30);
	if(sig != SIGALRM)
		return;
	time_t diff = time(NULL) - start;
	double mbs = total/(1024.0 * 1024.0 * diff);
	double written_mb = total / (1024.0 * 1024.0);
	fprintf(stderr, "Written: %.2lf MB (%.2lf MB/s)\n", written_mb, mbs);
}



void *run(void *data) {
	struct thread_data t = *(struct thread_data*) data;

	int rd = open(t.file, O_RDONLY);
	unsigned long long int total = 0;

	while(!clos) {
		int ret = splice(rd, NULL, t.fd, NULL, 16 * 4096, SPLICE_F_MORE);
		if(ret >= 0)
			total += ret;
		else
			break;
	}
	close(rd);
	return (void*)total;
}

int main(int argc, char *argv[]) {
	pthread_t *threads;
	int *rets;
	int n;

	if(argc != 3) {
		fprintf(stderr, "Aufruf: %s <Eingabedatei> <Anzahl Threads>\n", argv[0]);
		return -1;
	}

	struct thread_data t;
	t.file = argv[1];
	n = atoi(argv[2]);

	threads = malloc(n * sizeof(pthread_t));
	rets = malloc(n * sizeof(int));

	int p[2];
	pipe(p);
	t.fd = p[1];
	for(int i = 0; i < n; i++) {
		rets[i] = pthread_create(&threads[i], NULL, run, (void*)&t);
	}

	signal(SIGALRM, alrm);
	start = time(NULL);
	alarm(2);
	for(;;) {
		int res = splice(p[0], NULL, 1, NULL, 4096, 0);
		if(res >= 0)
			total += res;
		else {
			clos = 1;
			break;
		}
	}

	perror("Write finished");

	char buf[16 * 4096];
	read(p[0], buf, 16 * 4096);

	for(int i = 0; i < n; i++) {
		int to;
		pthread_join(threads[i], (void**)&to);
		fprintf(stderr, "\tThread %i wrote %llu bytes (%i %%)\n",
			i + 1, to, (total == 0 ? 0 : to*100/total));
	}

	fprintf(stderr, "Total bytes written: %llu\n", total);
	close(p[0]);
	close(p[1]);
	free(rets);
	free(threads);
	return 0;
}
