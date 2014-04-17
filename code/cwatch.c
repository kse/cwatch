#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include <unistd.h>
#include <string.h>

#include <sys/inotify.h>
#include <search.h>

#include <getopt.h>
#include "cwatch.h"

#include <regex.h>

/* TODO
 * Use a hash to store watch descriptors, instead of an array.

 * In command to execute, add escape to match triggered file.

 * See if we by matching the watch descriptor, can get a filename out of
   triggered files.

 * Full length names for passed options, they are easier to remember.

 * Add all inotify events to passable options.

 * Make monitoring a single file work better than oneshot (What did i mean?)

 * At some point in the future, make it possible to have a .cwatch file, that
   specifies matched patterns and actions on files.

 */

char *usage = "Usage: cwatch [OPTIONS] files/directories [OPTIONS]\n"
"OPTIONS:\n"
"\t-e CMD | --execute CMD        Execute CMD using a system() call.\n"
"\t-r REG | --regexp  REG        Only trigger when files match regexp.\n"
"\t -1    | --oneshot            Only trigger once\n"
"\t -v    | --verbose            Print more information\n"
"\n"
"\tSupported inotify watch types are:\n"
"\t --create (-n), --modify (-m), --close (-c), --delete (-d), --access (-a).\n";

int oneshot = 0, verbose = 0, use_regex = 0;

char *command = NULL;
regex_t regexp;

void loop(int inotify_fd, W_DATA **watched_files, int watch_count, uint32_t mask) {
	int c, wdes;

	while(1) {
		struct inotify_event *event = in_event(inotify_fd);
		char *name = NULL;

		// Optionally notify about events
		if(verbose == 1) {
			if(event->len > 0)
				printf("Received event for file: %s\n", event->name);
			else {
				printf("Event for unknown filename\n");
			}
		}

		if(event->len == 0) {
			for(c = 0; c < watch_count; c++) {
				if(watched_files[c]->wdes == event->wd) {

					// Because overwrites, and does not update the file, the
					// watch is lost on first save. Readding the watch seems to
					// work.
					wdes = inotify_add_watch(inotify_fd, 
							watched_files[c]->fname, mask);

					watched_files[c]->wdes = wdes;
					name = watched_files[c]->fname;
				}
			}
		} else {
			name = event->name;
		}

		(void)name;
		if(command != NULL) {
			if(use_regex) {
				if(regexec(&regexp, name, 0, NULL, 0) == 0) {
					system(command);
				} else {
					printf("Too bad\n");
				}
			} else {
				int ret = system(command);
				(void)ret;
			}
		}

		if(oneshot == 1)
			break;
	}
}


int
main(int argc, char **argv) {

	int res = 0;
	int c;
	int watch_count = 0;
	int wdes;

	// inotify watch mask
	uint32_t mask = 0;

	int option_index = 0;
	static struct option long_options[] = {
		{"create" ,   no_argument,       0, 'n'},
		{"modify" ,   no_argument,       0, 'm'},
		{"close"  ,   no_argument,       0, 'c'},
		{"delete" ,   no_argument,       0, 'd'},
		{"access" ,   no_argument,       0, 'a'},
		{"verbose",   no_argument,       0, 'v'},
		{"oneshot",   no_argument,       0, '1'},
		{"regex"  ,   required_argument, 0, 'r'},
		{"execute",   required_argument, 0, 'e'},
		{0,           0,                 0,  0 }     
	};

	int inotify_fd;
	extern char *optarg;

	while(1) {
		c = getopt_long(argc, argv, "nme:cda1r:v",
				long_options, &option_index);

		if(c == -1) 
			break;

		switch(c) {
			case '1':
				oneshot = 1;
				mask |= IN_ONESHOT;
				break;
			case 'a':
				mask |= IN_ACCESS;
				break;
			case 'n':
				mask |= IN_CREATE;
				break;
			case 'c':
				mask |= IN_CLOSE_WRITE;
				break;
			case 'd':
				mask |= IN_DELETE_SELF | IN_DELETE;
				break;
			case 'm':
				mask |= IN_MODIFY;
				break;
			case 'v':
				printf("Verbose\n");
				verbose = 1;
				break;
			case 'e':
				command = strdup(optarg);
				break;
			case 'r':
				res = regcomp(&regexp, optarg, REG_EXTENDED | REG_ICASE | REG_NOSUB);
				use_regex = 1;

				if(res != 0) {
					fprintf(stderr, "Unable to compile regexp: %d\n", res);
					exit(EXIT_FAILURE);
				}
				break;
			default:
				fprintf(stderr, "%s", usage);
				exit(EXIT_FAILURE);
				break;
		}
	}

	if(mask == 0) {
		mask |= IN_MODIFY;
	}

	// If we don't have atleast two arguments complain and exit.
	if(argc < optind + 1) {
		// This is retarded. Print some usage options instead.
		fprintf(stderr, "Not enough options.\n");
		exit(EXIT_FAILURE);
	}

	inotify_fd = inotify_init();
	if(inotify_fd == -1) { 
		fprintf(stderr, "Error initializing inotify: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Use c as a pointer into watched files.
	c = 0;

	// Allocate simple array storage for the amount of files we want to watch.
	W_DATA **watched_files = malloc((argc - (optind + 1))*sizeof(W_DATA*));

	// Add a watch to all files on the command line.
	while(optind < argc) {
		wdes = inotify_add_watch(inotify_fd, argv[optind], mask);
		W_DATA *w = malloc(sizeof(W_DATA));
		w->wdes = wdes;
		w->fname = strdup(argv[optind]);

		watched_files[c++] = w;

		optind++;
		watch_count++;
	}

	loop(inotify_fd, watched_files, watch_count, mask);

	// Free  stuff and exit
	for(c = 0; c < watch_count; c++) {
		free(watched_files[c]->fname);
		free(watched_files[c]);
	}

	free(watched_files);

	close(inotify_fd);
	exit(EXIT_SUCCESS);
}

// Read inotify event from filehandle to memory allocated once. If memory leaks
// are to be avoided, free the returned memory after the last usage.
struct inotify_event* in_event(int fd) {
	static struct inotify_event *event = NULL;

	if(event == NULL) {
		event = malloc(sizeof(struct inotify_event) + NAME_MAX + 1);
	}

	ssize_t read_size = read(fd, event, sizeof(struct inotify_event) + NAME_MAX + 1);

	if(read_size == -1) {
		fprintf(stderr, "Error reading inotify watch: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	return event;
}
