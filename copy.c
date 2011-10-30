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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

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
#ifdef SPLICE_F_MOVE
	if(!fallback) {
		int res = splice(from_fd, NULL, to_fd, NULL, chunk_size, 0);
		/* Something went wrong. Fall back to dumb read/write. */
		if(res == -1) {
			perror("Error in splice");
			fallback = 1;
		} else
			return res;
	}
#endif
	char buf[chunk_size];

	int res = fallback_rw(from_fd, to_fd, buf, chunk_size);

	if(res < 0) {
		perror("Error in fallback_rw");
			return -1;
	}

	return res;
}
