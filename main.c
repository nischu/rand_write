/*
    Copyright (C) by Nico Schümann prog (AT) nico22.de 2011

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "thread.h"
#include "copy.h"

volatile int clos = 0;
unsigned long long int total = 0;
time_t start = 0;

/*
 * Output how much we have written each 30 seconds.
 */
void alrm(int sig) {
	alarm(30);
	if(sig != SIGALRM)
		return;
	time_t diff = time(NULL) - start;
	double mbs = total/(1024.0 * 1024.0 * diff);
	double written_mb = total / (1024.0 * 1024.0);
	fprintf(stderr, "Written: %.2lf MB (%.2lf MB/s)\n", written_mb, mbs);
}

void hup(int sig) {
	clos = 1;
}

int main(int argc, char *argv[]) {
	pthread_t *threads;
	int n = 4;
	char file[1024] = "/dev/urandom";
	int c;

	while((c = getopt(argc, argv, "hf:n:")) != -1) {
		switch(c) {
			case 'f':
				strncpy(file, optarg, 1024);
				break;
			case 'n':
				n = atoi(optarg);
				break;
			case '?':
			case 'h':
				fprintf(stderr, "Parameters:\n\t"
						"-n number_of_threads "
						"(default: 4)\n\t-f file "
						"(default: /dev/urandom)\n");
				return 0;
			default:
				abort();
		}
	}

	struct thread_data t;
	t.file = file;

	if(n <= 0) {
		fprintf(stderr, "Staring zero threads: Exiting!\n");
		return -1;
	}
	fprintf(stderr, "Starting %i threads (use -n parameter to change)\n",
			n);

	/* Create n threads */
	threads = malloc(n * sizeof(pthread_t));

	int p[2];
	pipe(p);
	t.fd = p[1];
	int i;
	for(i = 0; i < n; i++) {
		int ret = pthread_create(&threads[i], NULL, run, (void*)&t);
		if(ret != 0) {
			fprintf(stderr, "Something went wrong when creating "
				"threads. Exiting.");
			return -1;
		}
	}

	signal(SIGALRM, alrm);
	signal(SIGHUP, hup);
	signal(SIGTERM, hup);
	signal(SIGINT, hup);
	start = time(NULL);
	alarm(2);

	while(clos != 1) {
		int res = copy(p[0], 1, 4096);
		if(res >= 0)
			total += res;
		else {
			clos = 1;
			break;
		}
	}

	perror("Write finished");

	/* Read what's still in the pipe */
	char buf[16 * 4096];
	read(p[0], buf, 16 * 4096);

	/* Kill all threads */
	for(i = 0; i < n; i++) {
		unsigned long long int *to;
		pthread_join(threads[i], (void**)&to);
		if(to != NULL) {
			fprintf(stderr, "\tThread %i wrote %llu bytes "
					"(%llu %%)\n",
			i + 1, *to, (total == 0 ? 0 : *to * 100/total));
			free(to);
		} else {
			fprintf(stderr, "\t Thread %i failes.\n", i + 1);
		}
	}

	fprintf(stderr, "Total bytes written: %llu\n", total);
	close(p[0]);
	close(p[1]);
	free(threads);
	return 0;
}
