# Project 2: File System

This project was created for the CS3650 Computer Systems class. Our goal was to implement a file system using inodes in order to keep track of where data is being stored. This file system, using the FUSE API, implements the same functionality as a normal filesystem

- [Makefile](Makefile)   - Targets are explained in the assignment text
- [README.md](README.md) - This README
- [nufs.c](nufs.c)       - The main file of the file system driver
- [test.pl](test.pl)     - Tests to exercise the file system

## Running the tests

You might need install an additional package to run the provided tests:

```
$ sudo apt-get install libtest-simple-perl
```

Then using `make test` will run the provided tests.


