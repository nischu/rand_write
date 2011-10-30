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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "thread.h"
#include "copy.h"

extern int clos;

void *run(void *data) {
	struct thread_data t = *(struct thread_data*) data;

	int rd = open(t.file, O_RDONLY);
	if(rd < 0) {
		perror("Could not open input file");
		return NULL;
	}

	unsigned long long int *total = malloc(sizeof(unsigned long long int));
	if(total == NULL)
		return NULL;

	while(!clos) {
		int ret = copy(rd, t.fd, 4096);
		if(ret >= 0)
			*total += ret;
		else
			break;
	}
	close(rd);
	return total;
}
