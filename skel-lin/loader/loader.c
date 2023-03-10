/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// for signals
#include <signal.h>
// for sleep signal ( debugging )
#include <unistd.h>
// for mmap
#include <sys/mman.h>
// for open
#include <fcntl.h>

#include "exec_parser.h"

// structura executabil
static so_exec_t *exec;

// descriptor fisier
static int fd;

// variabila flag
static int flag;

// functie de cautare a segmentului care a cauzat page fault-ul
// primeste ca parametru info->si_addr ( adresa erorii )
so_seg_t *find_error(void *address)
{
	// contor pentru segmente
	int i = 0;
	// segmentul returnat
	so_seg_t *segment;
	// parcurgere fiecare segment
	for (int i = 0; i < exec->segments_no; i++) {
		// adresa segment inceput
		void *start_address = (void *)exec->segments[i].vaddr;
		// adresa segment final ( = adresa start + memorie segment )
		void *end_address = start_address + exec->segments[i].mem_size;
		// daca adresa se afla intre adresa de start si adresa de final a unui segment
		if (start_address <= address && address <= end_address) {
			// retinem adresa segmentului gasit
			segment = &(exec->segments[i]);
			// iesim din for
			break;
		}
	}

	// daca se gaseste un segment, returnam adresa
	if (i < exec->segments_no)
		return segment;

	// altfel, se intoarce valoarea NULL ( nu exista segment )
	return NULL;

}

// initializare zona data pentru a tine cont de paginile mapate
void set_data(void)
{
	// parcurgere fiecare segment
	for (int i = 0 ; i < exec->segments_no ; i++) {
		// preluare adresa segment ( pentru usurinta scriere cod )
		so_seg_t *segment = &(exec->segments[i]);
		// dimensiunea segmentului ( in mod normal in loc de mem_size ar trebui file_size )
		int custom_dim = segment->mem_size / getpagesize() + 1;
		int *custom = malloc(custom_dim * sizeof(int));

		for (int j = 0 ; j < custom_dim ; j++)
			((int *)custom)[j] = 0;
		exec->segments[i].data = custom;
	}

}

// handler ul custom
static void segv_handler(int signum, siginfo_t *info, void *context)
{

	// ** CAUTARE SEGMENT CORESPUNZATOR ERORII **

	// cautarea erorii ( info->si_addr ) intr un segment
	so_seg_t *segment = find_error(info->si_addr);

	if (segment == 0) {
		// modificare tratare semnal pentru handler ul default
		sigaction(signum, (void *)SIG_DFL, NULL);
		// trigger la semnal pentru a intra in handler
		raise(signum);
	}



	// ** STABILIRE VALORI PENTRU SEGMENTUL GASIT CU CARE LUCRAM **

	// size ul unei pagini in memorie
	int size_of_page = getpagesize();
	// valoarea de start a segmentului gasit
	void *start = (void *)segment->vaddr;
	// index ul unei pagini din segmentul curent
	int index_page = (info->si_addr - start) / size_of_page;



	// ** VERIFICARE MAPARE PAGINA **

	// initializare zona "data" o singura data ca masca de biti
	if (flag == 0) {
		flag = 1;
		set_data();
	}

	if (*((int *)segment->data + index_page) != 0) {
		// modificare tratare semnal pentru handler ul default
		sigaction(signum, (void *)SIG_DFL, NULL);
		// trigger la semnal pentru a intra in handler
		raise(signum);
	}



	// ** MAPARE PAGINA DE LA CARE A VENIT PAGE FAULT-UL **

	// adresa paginii la care mapam
	void *start_map = start + size_of_page * index_page;

	// - acordam toate permisiunile ( desi tehnic PERM_W e singurul necesar )
	// - ca flag uri avem nevoie de :
	//   * MAP_FIXED pentru a mapa EXACT la start_map, nu pentru a-l folosi ca si hint
	//   * MAP_SHARED pentru a vedea si celelalte procese ce mapeaza acesta regiune maparea paginii
	//   * MAP_ANON (MAP_ANONYMOUS) pentru a initializa contentul cu 0, are nevoie de fd = -1
	// - offset ul e 0
	void *map_page = mmap(start_map, size_of_page, PERM_R | PERM_W | PERM_X, MAP_FIXED | MAP_SHARED | MAP_ANON, -1, 0);



	// ** COPIERE DATE DIN SEGMENTUL DIN FISIER **

	// adresa de unde trebuie sa citim ( la fel ca start_map doar ca e de tip int)
	// segment->offset ~= segment->vaddr
	int start_read = segment->offset + index_page * size_of_page;
	// dimensiunea segmentului
	int size_segment = segment->file_size;
	// locul unde se afla pagina in segment
	int place_page = index_page * size_of_page;

	// daca pagina se afla in zona unde se gasesc paginile ( file_size )
	if (place_page < size_segment) {
		// daca suntem siguri ca se copiaza toata pagina
		if (place_page + size_of_page < size_segment) {
			// mutam cursorul de unde incepem sa copiem datele paginii
			lseek(fd, start_read, SEEK_SET);
			// citim in pagina mapata cu dimensiunea unei pagini
			read(fd, map_page, size_of_page);
		}	else {
			// mutam cursorul de unde incepem sa copiem datele paginii
			lseek(fd, start_read, SEEK_SET);
			//	dimensiunea pe care o citim nu mai e dimensiunea unei pagini
			//  dimensiunea ramasa pentru bucata_pagina =
			//     = dimensiunea totala - dimensiunea alocata restul paginilor

			// dimensiune alocata pentru restul paginilor
			int size_used_segment = index_page * size_of_page;
			// dimensiune ramasa pentru bucata_pagina
			int size_of_part_page = size_segment - size_used_segment;
			// citim in pagina mapata cu dimensiunea unei bucata_pagina
			read(fd, map_page,	size_of_part_page);
		}

	}




	// ** PERMISIUNI **

	// restauram permisiunile paginii cu permisiunile segmentului din care face parte
	mprotect(map_page, size_of_page, segment->perm);
	// marcam pagina in zona data ca fiind mapata
	*((int *)segment->data + index_page) = 1;

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
	// preluat din "exec_parser.c"
	fd = open(path, O_RDONLY);

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
