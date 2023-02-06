// Inode manipulation routines.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include "blocks.h"

typedef struct inode {
  int refs;  // reference count
  int mode;  // permission & type
  int size;  // bytes
  int block; // single block pointer (if max file size <= 4K)
  int ents; //entry points 
} inode_t;

void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int alloc_inode();
void free_inode();
int grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);
int inode_get_bnum(inode_t *node, int fbnum);

#endif
