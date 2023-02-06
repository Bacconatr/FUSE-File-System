#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include <libgen.h>
#include <bsd/string.h>
#include <stdint.h>
#include <stdlib.h>

#include "helpers/storage.h"
#include "helpers/slist.h"


#include "helpers/blocks.h"
#include "helpers/inode.h"
#include "helpers/directory.h"

// initializes storage
void
storage_init(const char *path) {
    blocks_init(path);
    directory_init();
}

//gets attributes at given path of the given stat 
int storage_stat(const char *path, struct stat *st) {
    int inum = tree_lookup(path);
    if (inum < 0) {
     //   printf("+ storage_stat(%s) -> %d\n", path, inum);
        return inum;
    }

    inode_t *node = get_inode(inum);
   // printf("+ storage_stat(%s) -> 0; inode %d\n", path, inum);
 

    // sets up stat structure
    st->st_uid = getuid();
    st->st_mode = node->mode;
    st->st_size = node->size;
    st->st_nlink = node->refs;
    return 0;
}

// reads size bytes from specified path into buffer
int
storage_read(const char *path, char *buf, size_t size, off_t offset) {
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }
    inode_t *node = get_inode(inum);
    printf("+ storage_read(%s); inode %d\n", path, inum);

    int numRead = 0;

    // iterates until enough bytes are read
    while (numRead < size) {
        int bnum = inode_get_bnum(node, offset + numRead); //buffer plus amount of bytes read  = block
        char *block = blocks_get_block(bnum); //block info from block cast 
        char *readBlock = block + ((offset + numRead)); 


        int bytesToRead = 0;
        int lob = size - numRead;
        int bl = block + 4096 - readBlock;

       if(lob <= bl) { //blocks left 
              bytesToRead = lob; //bytes to read is bytes left 
         } else {
              bytesToRead = bl; //blocks left to read is bytes to read 
       }
        printf(" + reading from block: %d\n", bnum);
        memcpy(buf + numRead, readBlock, bytesToRead);//copy readblock data to the size of bytes to read onnto number of bytes read(buffered)
        numRead += bytesToRead;
    }

    return size;
}

// writes size bytes from path starting at offset into buf --> inverse of read 
int
storage_write(const char *path, const char *buf, size_t size, off_t offset) {
   
      int trv = storage_truncate(path, offset + size);
    if (trv < 0) {
        return trv;
    }

    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    inode_t *node = get_inode(inum);
    printf("+ storage_read(%s); inode %d\n", path, inum);


    int numWrit = 0;

    // iterates until enough bytes are written
    while (numWrit < size) {
        int bnum = inode_get_bnum(node, offset + numWrit);
        char *bb = blocks_get_block(bnum);
        char *writBlock = bb + ((offset + numWrit) % 4096);

        int bytesToWrit = 0;
        int lob = size - numWrit;
        int bl = bb + 4096 - writBlock;
        if(lob <= bl) {
            bytesToWrit = lob;
        } else {
            bytesToWrit = bl;
        }
        printf("+ writing to block: %d\n", bnum);
        memcpy(writBlock, buf + numWrit, bytesToWrit); ///
        numWrit += bytesToWrit;
    }

    return size;
}

// truncates file to given length
int
storage_truncate(const char *path, off_t size) {
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    inode_t *node = get_inode(inum);
    if (size >= node->size) { //if greater than grow 
        int rv = grow_inode(node, size);
        return rv;
    } else {
        int rv = shrink_inode(node, size); //shrink
        return rv;
    }
}

// parse the parent of a path 
void find_parent(const char *path, char *dir) {
    strcpy(dir, path);


//iterator through the path starting at end until "/" as delimiter
// "/" == reached parent->name 
    int wordLen = 0;
    for (int i = strlen(path); path[i] != '/'; i--) {
        wordLen++;
    }
    //change dir to reflect the parent dir of the path
    if (wordLen == strlen(path)) {
        dir[1] = '\0';
    } else {
        dir[strlen(path) - wordLen] = '\0';
    }
}

// given the full path, parses the child of it returning the child 
char * find_child(const char *path, char *sec) {
    strcpy(sec, path);

    //start at the eopath and continute until "/" is reached 
    int wordLen = 0;
    for (int i = strlen(path) - 1; path[i] != '/'; i--) { //loop through until / 
        wordLen++;
    }
    //add the name of the dir to the path of the sec
    sec += strlen(path) - wordLen;
    return sec;
}

// makes a file/directory at the specified path
int
storage_mknod(const char *path, int mode) {

    if (tree_lookup(path) > -1) {
        printf("mknod fail: already exist\n");
        return -EEXIST;
    }

    char *newPath = malloc(strlen(path));
    memset(newPath, 0, strlen(path));
    newPath[0] = '/'; // root

    slist_t *items = s_explode(path + 1, '/'); //splice by / (like in shell)

    // iterate until no path elements remain
    for (slist_t *alist = items; alist != NULL; alist = alist->next) {
        int subNum = directory_lookup(get_inode(tree_lookup(newPath)), alist->data);
        int dirNum = tree_lookup(newPath);

        // if last element has been reached
        if (alist->next == NULL && subNum < 0) {
            int newInum = alloc_inode();
            inode_t *newNode = get_inode(newInum); // boring
            newNode->mode = mode;

            if (newNode->mode == 040755) { //if directory  
                char *selfname = ".";
                char *parentname = "..";

                directory_put(newNode, parentname, tree_lookup(newPath)); 
                directory_put(newNode, selfname, newInum);
            }

            int rv = directory_put(get_inode(tree_lookup(newPath)), alist->data, newInum);
            if (rv != 0) {
                free(newPath);
                return -1;
            }

            newNode->refs = 1;
            free(newPath);
            return 0;
        }    else {

            int newInum = alloc_inode(); //allocate new inode
            inode_t *newNode = get_inode(newInum); // 
            newNode->mode = 040755; //set to root 

            //basically directory_init()
            char *selfname = ".";
            char *parentname = "..";

            directory_put(newNode, parentname, dirNum);
            directory_put(newNode, selfname, newInum);
            directory_put(get_inode(dirNum), alist->data, newInum);
            newNode->refs = 1;

            if (strcmp(newPath, "/") == 0) { //if /
                strcat(newPath, alist->data); //copy data
                continue;
            } 
        }

    }


}

// list all elements at the given path
slist_t *
storage_list(const char *path) {
    return directory_list(path);
}



// unlinks file/dir at given path from its parent
int
storage_unlink(const char *path) {
    char *dir = malloc(strlen(path));
    char *sub = malloc(strlen(path));

    find_parent(path, dir);
    sub = find_child(path, sub);

    int dirnum = tree_lookup(dir); //iindex num for directory 
    int subnum = tree_lookup(path); //index num for child 

    inode_t *dirnode = get_inode(dirnum); //inode for directory parent
    inode_t *subnode = get_inode(subnum); //indode for sub parent 

    int rv = directory_delete(dirnode, sub); //delete sub 
    return rv;
}

// creates link from from to to
int
storage_link(const char *from, const char *to) {

    int toNum = tree_lookup(to);
    if (toNum < 0) {
        return toNum;
    }

    char *fromParent = malloc(strlen(from));
    char *fromChild = malloc(strlen(from));
    find_parent(from, fromParent);
    fromChild = find_child(from, fromChild);

    int fromParentNum = tree_lookup(fromParent); // directory of link
    inode_t *fromParentNode = get_inode(fromParentNum);


    int rv = directory_put(fromParentNode, fromChild, toNum);

    return 0;
}

// renames the from file to to from \mv 
int
storage_rename(const char *from, const char *to) {
    storage_link(to, from);
    storage_unlink(from);
    return 0;
}


