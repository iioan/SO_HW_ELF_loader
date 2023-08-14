/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h> // for signal handling
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <sys/mman.h> // for mmapping

#include "exec_parser.h"

#define PAGE_SIZE 4096 // dimensiunea paginii

static so_exec_t *exec;
static int fd;

/* Gaseste segmentul care contine adresa 'address' */
/* daca nu se gaseste niciun segment, returneaza NULL */
so_seg_t *find_mySegment(char *address) {
        for(int i = 0; i < exec->segments_no; i++) {
		if(address >= (char *)exec->segments[i].vaddr && address < (char *)exec->segments[i].vaddr + exec->segments[i].mem_size)
			return &exec->segments[i];
	}
	return NULL;
}

/* Implementarea handler-ului  */
static void segv_handler(int signum, siginfo_t *info, void *context) {
	if(signum != SIGSEGV) {
		perror("wrong signal");
		signal(signum, SIG_DFL);
	}
	/* SEGV_ACCER = permisiuni nevalide pentru obiectul mapat */
	if(info->si_code == SEGV_ACCERR) {
		perror("access error");
		signal(SIGSEGV, SIG_DFL);
	}
	/* SEGV_MAPERR = adresa nu este mapata */
	if(info->si_code == SEGV_MAPERR) {
                char *fault = info->si_addr;
		so_seg_t *mySegment = find_mySegment(fault);
		if(mySegment == NULL)
		{
			perror("segment not found");
			signal(SIGSEGV, SIG_DFL);
		}
		char *vaddr = (char *)mySegment->vaddr; // adresa virtuala a segmentului
		int pageNo = (fault - vaddr) / PAGE_SIZE; // numarul paginii din segment care contine adresa fault
		int pageOffset = pageNo * PAGE_SIZE; // offsetul paginii din segment
		void *writingPos = vaddr + (int)pageOffset; // adresa de scriere a paginii
		void *fileInterval = vaddr + mySegment->file_size; // adresa de sfarsit a intervalului de scriere din fisier
		char *mapped = mmap(writingPos, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, fd, mySegment->offset + pageOffset);
		if(mapped == MAP_FAILED) {
			perror("map failed");
			exit(-1);
		}
		/* daca adresa de scriere este in afara intervalului dimensiunii în cadrul fișierului, scriem 0 */
		int length = fileInterval - writingPos;
		if (length < 0)
			length = 0;
		int fill = PAGE_SIZE - length;
		if (fill > 0)
			memset(writingPos + length, 0, fill);
		/* atribui permisiunile cerute de segment */
		int permision = mprotect(mapped, PAGE_SIZE, mySegment->perm);
		if(permision == -1) {
			perror("mprotect failed");
			exit(-1);
		}
	}
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	fd = open(path, O_RDONLY);
	if(fd == -1)
		return -1;
	exec = so_parse_exec(path);
	if (!exec)
		return -1;
	so_start_exec(exec, argv);
	close(fd);
	return -1;
}

