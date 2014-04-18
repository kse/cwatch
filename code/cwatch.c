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

 * When matching escape, possibly prefix folder name.

 * Add all inotify events to passable options.

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
	int c, wdes, offset;
	char buf[65536];
	static struct inotify_event *event = NULL;
	ssize_t read_size ;
	char *name = NULL;
	char *filled_command;

	while(1) {
		//struct inotify_event *event = in_event(inotify_fd);
		read_size = read(inotify_fd, buf, 65536);

		if(read_size == -1) {
			fprintf(stderr, "Error reading inotify watch: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		for(offset = 0; offset < read_size; offset += sizeof(struct inotify_event) + event->len) {
			event = (struct inotify_event*) &buf[offset];

			if(event->len == 0) {
				for(c = 0; c < watch_count; c++) {
					if(watched_files[c]->wdes == event->wd) {
						inotify_rm_watch(inotify_fd, event->wd);

						// Because overwrites, and does not update the file,
						// the watch is lost on first save. Readding the watch
						// seems to work.
						wdes = inotify_add_watch(inotify_fd, 
								watched_files[c]->fname, mask);

						watched_files[c]->wdes = wdes;
						name = watched_files[c]->fname;
					}
				}
			} else {
				name = event->name;
			}

			/*
			printf("Mask: %u\n",  mask);
			printf("Eventname: %s\n",  name);
			printf("Eventmask: %u\n", event->mask);
			*/

			if(event->mask == IN_IGNORED) {
				continue;
			}

			// Optionally notify about events
			if(verbose == 1) {
				printf("Received event for file: %s\n", name);
			}

			char *replace_index = strstr(command, "{}");
			if(replace_index != NULL) {

				filled_command = malloc(sizeof(char) * (strlen(command) + strlen(name) - 1));
				filled_command[0] = '\0';

				strncat(filled_command, command, (int)(replace_index - command));
				strcat(filled_command, name);
				strcat(filled_command, replace_index + 2);
			} else {
				filled_command = command;
			}

			if(filled_command != NULL) {
				
				if(use_regex) {
					if(regexec(&regexp, name, 0, NULL, 0) == 0) {
						system(filled_command);
					}
				} else {
					system(filled_command);
				}
			}

			if(replace_index != NULL) {
				free(filled_command);
			}

			if(oneshot == 1) {
				break;
			}
		}
	}
}


int
main(int argc, char **argv) {

	int res = 0;
	int c;
	int watch_count = 0;
	int wdes;

	// inotify watch mask
	uint32_t mask = IN_EXCL_UNLINK;

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

	if(mask == IN_EXCL_UNLINK) {
		mask |= IN_MODIFY;
	}

	// If we don't have atleast two arguments complain and exit.
	if(argc < optind + 1) {
		// This is retarded. Print some usage options instead.
		fprintf(stderr, "Not enough options.\n");
		fprintf(stderr, "%s", usage);
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
