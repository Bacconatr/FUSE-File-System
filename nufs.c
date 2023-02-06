#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <bsd/string.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define FUSE_USE_VERSION 26

#include <fuse.h>

#include "helpers/storage.h"
#include "helpers/directory.h"
#include "helpers/slist.h"


// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask) {
    int rv = 0;
    int inum = tree_lookup(path);

    if (inum < 0) {
        rv = -ENOENT;
    } else {
        
    printf("access(%s, %04o) -> %d\n", path, mask, rv);
    return rv;
}
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
// This is a crucial function.
int
nufs_getattr(const char *path, struct stat *st) {
    int rv = storage_stat(path, st);
    printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
    return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi) {
                
    struct stat stat;
    char stat_path[128];
    int rv;

    slist_t *items = storage_list(path);
    slist_t *list = items;
        while(list!=NULL) {

        
        printf("+ looking at path: '%s'\n", list->data);

        // if we are looking at root node, do default:
        if (strcmp(path, "/") == 0) {
            stat_path[0] = '/';
        
            rv = storage_stat(stat_path, &stat);
            filler(buf, list->data, &stat, 0);
        } else { //not root node 

            strncpy(stat_path, path, strlen(path));       //copy lenth of path into statpath
            stat_path[strlen(list->data) + strlen(path)] = '\0';

            rv = storage_stat(stat_path, &stat); //storage_stat (getattr)
            filler(buf, list->data, &stat, 0); //fuse function 
        }
        list = list->next;
    }

    s_free(items); //free recursively 

    printf("readdir(%s) -> %d\n", path, rv);
    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
    int rv = storage_mknod(path, mode); 
    printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode) {
    mode = 040000;
    int rv = nufs_mknod(path, mode, 0);
    printf("mkdir(%s) -> %d\n", path, rv);
    return rv;
}


//basically remove?
int
nufs_unlink(const char *path) {
    int rv = storage_unlink(path);
    printf("unlink(%s) -> %d\n", path, rv);
    return rv;
}


//link from to to 
int
nufs_link(const char *from, const char *to) {
    int rv = storage_link(to, from);
    printf("link(%s => %s) -> %d\n", from, to, rv);
    return rv;
}

//remove directory 
int
nufs_rmdir(const char *path) {
    int inum = tree_lookup(path);
    inode_t *node = get_inode(inum);

    int mode = node->mode;

    if (mode != 040755) { //not root directory number 
        return -1; //cant rmdir a file 
    }

    char *data = blocks_get_block(node->block);
    
    int rv = nufs_unlink(path);

    printf("rmdir(%s) -> %d\n", path, rv);

    return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to) {
    int rv = storage_rename(from, to);
    printf("rename(%s => %s) -> %d\n", from, to, rv);
    return rv;
}

//changes the given permissions 
int
nufs_chmod(const char *path, mode_t mode) {
    int rv = -1;

    int inum = tree_lookup(path);
    inode_t *node = get_inode(inum); //inode at path
    node->mode = mode; //set mode = to inode mode (change the permisssions)
   

    printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

int
nufs_truncate(const char *path, off_t size) {
    int rv = storage_truncate(path, size);
    printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
    return rv;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi) {
    int rv = nufs_access(path, 0);
    printf("open(%s) -> %d\n", path, rv);
    return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int rv = storage_read(path, buf, size, offset);
    printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int rv = storage_write(path, buf, size, offset);
    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}



void
nufs_init_ops(struct fuse_operations *ops) {
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access = nufs_access;
    ops->getattr = nufs_getattr;
    ops->readdir = nufs_readdir;
    ops->mknod = nufs_mknod;
    ops->mkdir = nufs_mkdir;
    ops->link = nufs_link;
    ops->unlink = nufs_unlink;
    ops->rmdir = nufs_rmdir;
    ops->rename = nufs_rename;
    ops->chmod = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open = nufs_open;
    ops->read = nufs_read;
    ops->write = nufs_write;

   
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[]) {
    assert(argc > 2 && argc < 6);
    storage_init(argv[--argc]);
    nufs_init_ops(&nufs_ops);
    return fuse_main(argc, argv, &nufs_ops, NULL);
}


