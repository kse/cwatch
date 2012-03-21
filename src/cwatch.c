#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/inotify.h>
#include <search.h>

#include <getopt.h>
#include "cwatch.h"

/* TODO
 * Accept watches for more than one file
 * Maybe a warning about monitoring folders
 * All watches options?
 * Make monitoring a single file work better than oneshot
 */

int
main(int argc, char **argv) {
	int oneshot = 0, verbose = 0;

	// All round variable
	int c;

	int watch_count = 0;

	int wdes;

	// System command to execute
	char *command = NULL;

	// inotify watch mask
	uint32_t mask = 0;

	int option_index = 0;
	static struct option long_options[] = {
		{"create" ,   no_argument,       0, 'n'},
		{"modify" ,   no_argument,       0, 'm'},
		{"close"  ,   no_argument,       0, 'c'},
		{"delete" ,   no_argument,       0, 'd'},
		{"access" ,   no_argument,       0, 'a'},
		{"verbose",   no_argument,       0, 'r'},
		{"oneshot",   no_argument,       0, '1'},
		{"execute",   required_argument, 0, 'e'},
		{0,           0,                 0, 0  }     
	};

	char *filename;
	int inotify_fd;

	extern char *optarg;

	while(1) {

		c = getopt_long(argc, argv, "nme:cda1rv",
				long_options, &option_index);

		if(c == -1) 
			break;

		switch(c) {
			case '1':
				oneshot = 1;
				break;
			case 'a':
				mask = mask | IN_ACCESS;
				break;
			case 'n':
				mask = mask | IN_CREATE;
				break;
			case 'c':
				mask = mask | IN_CLOSE_WRITE;
				break;
			case 'd':
				mask = mask | IN_DELETE_SELF | IN_DELETE;
				break;
			case 'm':
				mask = mask | IN_MODIFY;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'e':
				command = strdup(optarg);
				break;
			default:
				fprintf(stderr, "Usage information\n");
				exit(EXIT_FAILURE);
				break;
		}
	}

	if(mask == 0)
		mask = mask | IN_MODIFY;

	// If we don't have atleast two arguments complain and exit.
	if(argc < optind + 1) {
		// Print usage instead
		fprintf(stderr, "Not enough options to work with.\n");
		exit(EXIT_FAILURE);
	}

	filename = strdup(argv[optind]);

	// Redo checking if file exists, should use stat() instead,
	// if it's even needed.
	/*
	if(access(filename, F_OK) == -1) {
		fprintf(stderr, "No such file: %s\n", filename);
		free(filename);
		exit(EXIT_FAILURE);
	}
	*/

	inotify_fd = inotify_init();
	if(inotify_fd == -1) { 
		fprintf(stderr, "Error initializing inotify: %m\n");
		free(filename);
		exit(EXIT_FAILURE);
	}

	// Use c as a pointer into watched files.
	c = 0;

	W_DATA **watched_files = malloc((argc - (optind + 1))*sizeof(W_DATA*));
	while(optind < argc) {
		wdes = inotify_add_watch(inotify_fd, argv[optind], mask);
		W_DATA *w = malloc(sizeof(W_DATA));
		w->wdes = wdes;
		w->fname = strdup(argv[optind]);

		watched_files[c++] = w;

		optind++;
		watch_count++;
	}


	while(1) {
		struct inotify_event *event = in_event(inotify_fd);

		// Optionally notify about events
		if(verbose == 1) {
			//if((event->mask & IN_ISDIR) != 0)
			if(event->len > 0)
				printf("Received event for file: %s\n", event->name);
			else {
				printf("Event for unknown filename\n");
			}
		}
		
		// Execute the execution argument
		if(command != NULL) {
			int ret = system(command);
			(void)ret;
		}
		
		for(c = 0; c < watch_count; c++) {
			if(watched_files[c]->wdes == event->wd) {
				//printf("Replacing watch for %s\n", watched_files[c]->fname);
				inotify_rm_watch(inotify_fd, event->wd);

				wdes = inotify_add_watch(inotify_fd, 
						watched_files[c]->fname, mask);

				watched_files[c]->wdes = wdes;
			}
		}

		free(event);

		if(oneshot == 1)
			break;
	}

	// Free  stuff and exit
	for(c = 0; c < watch_count; c++) {
		free(watched_files[c]->fname);
		free(watched_files[c]);
	}

	free(watched_files);

	close(inotify_fd);
	free(filename);
	exit(EXIT_SUCCESS);
}

struct inotify_event* in_event(int fd) {
	ssize_t buf_size = 32768;

	// Space allocation for reading inotify events
	char buf[buf_size];
	ssize_t read_size;

	struct inotify_event *event = malloc(sizeof(struct inotify_event));

	unsigned int i;
	
	read_size = read(fd, buf, buf_size);

	if(read_size == -1) {
		fprintf(stderr, "Error reading inotify watch: %m\n");
		exit(EXIT_FAILURE);
	}

	event->wd = (int)buf[0];
	event->mask = (uint32_t)buf[sizeof(int)];
	event->cookie = (uint32_t)buf[sizeof(int) + sizeof(uint32_t)];
	event->len = (uint32_t)buf[sizeof(int) + 2*sizeof(uint32_t)];

	if(event->len > 0) {
		event = realloc(event, sizeof(struct inotify_event)+event->len);
		for(i = 0; i < event->len; i++) {
			event->name[i] = buf[sizeof(struct inotify_event) + i];
		}
	}

	return event;
}
