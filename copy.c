#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

extern int clos;

static int fallback_rw(int from_fd, int to_fd, char *buf, int buf_size)
{
	ssize_t written = 0;
	ssize_t amount = read(from_fd, buf, buf_size);

	if(amount == -1) {
		return -1;
	}

	while(amount > 0) {
		written = write(to_fd, buf + written, amount);

		if(written == -1)
			return -2;

		amount -= written;
	}

	return written;
}

int copy(int from_fd, int to_fd, int chunk_size)
{
	static int fallback = 0;

	/* The better approach: Zero-copy splice. */
	if(!fallback) {
		int res = splice(from_fd, NULL, to_fd, NULL, chunk_size, 0);
		/* Something went wrong. Fall back to dumb read/write. */
		if(res == -1) {
			perror("Error in splice");
			fallback = 1;
		} else
			return res;
	}

	char buf[chunk_size];

	int res = fallback_rw(from_fd, to_fd, buf, chunk_size);

	if(res < 0) {
		perror("Error in fallback_rw");
			return -1;
	}

	return res;
}
