/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "exec_parser.h"

#undef si_addr

static so_exec_t *exec;
static char *exec_name;

// functia returneaza indexul segmentului din care face parte adresa
int find_seg(void *addr)
{
	int i;

	for (i = 0; i < exec->segments_no; i++) {
		if (addr >= (void *)exec->segments[i].vaddr && addr <= (void *)exec->segments[i].vaddr + exec->segments[i].mem_size)
			return i;
	}
	return -1;
}

// functia aloca memorie pentru un vector de frecventa corespunzator paginilor
void init_data(void)
{
	int i, pagesize = getpagesize();

	for (i = 0; i < exec->segments_no; i++)	{
		int max_pages = exec->segments[i].mem_size / pagesize + 1;

		exec->segments[i].data = calloc(max_pages, sizeof(int));
	}
}

// functia returneaza numarul de pagini deja mapate
int get_nr_pages(so_seg_t *segment)
{
	int i, nr_pages = 0, pagesize = getpagesize();
	int max_pages;

	if (segment->mem_size % pagesize == 0)
		max_pages = segment->mem_size / pagesize;
	else
		max_pages = segment->mem_size / pagesize + 1;

	for (i = 0; i < max_pages; i++)	{
		if (*((int *)(segment->data) + i) == 1)
			nr_pages++;
	}

	return nr_pages;
}

// functia returneaza indexul paginii care trebuie mapata
int find_page(void *addr, void *vaddr, int index)
{
	int max_pages, pagesize = getpagesize();

	if (exec->segments[index].mem_size % pagesize == 0)
		max_pages = exec->segments[index].mem_size / pagesize;
	else
		max_pages = exec->segments[index].mem_size / pagesize + 1;

	for (int i = 0; i < max_pages; i++)
		if (addr >= vaddr + i * pagesize && addr < vaddr + (i + 1) * pagesize)
			return i;

	return -1;
}

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	/* TODO - actual loader implementation */

	void *si_addr = info->_sifields._sigfault.si_addr;
	int pagesize = getpagesize();

	// file-descriptor-ul fisierului
	int fd = open(exec_name, O_RDWR);

	// se afla segmentul in care se afla adresa
	int index = find_seg(si_addr);

	// in cazul unui acces invalid (adresa nu se gaseste in niciun segment)
	// se rulează handler-ul default de page fault
	if (index == -1)
		signal(signum, SIG_DFL);

	void *vaddr = (void *)exec->segments[index].vaddr;

	// se afla pagina care urmeaza sa fie mapata
	int index_page = find_page(si_addr, vaddr, index);

	// in cazul unui acces nepermis (pagina este deja mapata)
	// se rulează handler-ul default de page fault
	if (*((int *)(exec->segments[index].data) + index_page) == 1)
		signal(signum, SIG_DFL);

	// se mapeaza pagina gasita
	void *p = mmap(vaddr + index_page * pagesize, pagesize, PERM_W, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

	// maparea paginii se marcheaza in vectorul de frecventa
	*((int *)(exec->segments[index].data) + index_page) = 1;

	// se citeste din fisier
	if (index_page * pagesize < exec->segments[index].file_size) {
		int offset = exec->segments[index].offset;

		if ((index_page + 1) * pagesize < exec->segments[index].file_size) {
			lseek(fd, offset + index_page * pagesize, SEEK_SET);
			read(fd, p, pagesize);
		} else {
			lseek(fd, offset + index_page * pagesize, SEEK_SET);
			read(fd, p, exec->segments[index].file_size - index_page * pagesize);
		}
	}

	// se stabilesc permisiunile
	mprotect(p, pagesize, exec->segments[index].perm);
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
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	// se retine calea catre executabil
	exec_name = path;

	// se aloca memorie pentru vectorul de frecventa
	init_data();

	so_start_exec(exec, argv);

	return -1;
}
