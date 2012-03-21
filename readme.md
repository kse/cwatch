# cwatch #

## Intro ##

Cwatch is a simple program written in C, which can be used to monitor files,
and perform a shell command when the file has been altered.

## Disclaimer and info ##

The program uses inotify, and was written because i wanted to make
it possible to easily compile .tex documents when i save them.

I, ofcourse, accept no responsibilities for anything you do with this program.

Oh, and don't monitor a folder and execute a command on that changes the
same folder on edit.
That's not pleasant.

## Compilation ##

Simple checkout the source, type 'make', and hope everything works.

## Usage ##

> cwatch [options] FILES

Options are:

*	-1  Oneshot, only runs once.
*	-e *command* Command to execute
*	-a  Execute when file is accessed
*	-n  Execute when file is created
*	-c  Execute when file is closed
*	-d  Execute when file is deleted
*	-m  Execute when file is modified (default)
*	-v  Prints when events are received

### Example ###

How i use it:

	cwatch -e "make latex" handin.tex
