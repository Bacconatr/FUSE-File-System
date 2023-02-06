#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "helpers/directory.h"
#include "helpers/inode.h"


//initialize the directory
void directory_init()
{
    int inum = alloc_inode(); //allocate

   //create inode and initialize as directory
    inode_t* nnode = get_inode(inum);
    nnode->mode = 040755;
    nnode->refs = 0; // not yet initialised

    char* selfname = ".";
    char* parentname = "..";


    //let directory be called on as normal
    directory_put(nnode, selfname, inum); //place . in directory path
    directory_put(nnode, parentname, inum); //place .. in dorectory path


    nnode->refs = 0;

}
// gets the inum of the given entry in the given directory
int directory_lookup(inode_t* dd, const char* name)
{

    //info of the first block in the directory 
    char* block_info = blocks_get_block(inode_get_bnum(dd, 0));

//loop until entries are all gone through (check each part of the inode)
    for (int i = 0; i < dd->ents; ++i) {
        printf(" ++ lookup '%s' =? '%s' (%p)\n", name, block_info, block_info);


        //iterate thru data to get to next data point
        char* iterator(char* block_info) {
             while (*block_info != 0) {
                block_info++;
            } return block_info + 1;
            
        }
        //if info at the given point is equal to the given name 
        if (strcmp(block_info, name) == 0) {
            block_info = iterator(block_info); //change block info to info at this point
            int* inum = (int*)(block_info); //inode num at given block
            return *inum;
        }

        block_info = iterator(block_info); //otherwise change block info to info at given point 
        block_info += sizeof(int); 
    }
    return -ENOENT; //return fail if have gone through anything and name doesn't exist as given 
}



//gets the inum of the given path 
int tree_lookup(const char *path) {


        if(strcmp(path,"/") == 0) { //the path is just the root address
            return 0; //return 0 (first) 
        } 

        int dir = 0;
        slist_t *path_list = s_explode(path, '/'); //splice based on /
        path_list = path_list->next; //next + next.next 

        slist_t* curr = path_list;

        while(curr) { //while path exists 
        int inum = dir;
        inode_t* inode; 
        inode = get_inode(inum);
        dir = directory_lookup(inode, curr->data); //lookup inode data 
        if(dir == -1) {
            return -1; //return -1 if this fails 
        }
        curr = curr->next;
        }

        return dir; 
}
// inserts given name/inum into directory
int directory_put(inode_t* dd, const char* name, int inum)
{
    int length = strlen(name) + 1;
    char* data = blocks_get_block(inode_get_bnum(dd, 0));

    if (dd->size + length > 4096) {
        return -ENOENT;
    }

     //copy name up to length into data plus directory size //REMINDER:!!!!
    memcpy(data + dd->size, name, length);
    dd->size += length;

    memcpy(data + dd->size, &inum, sizeof(inum));
    dd->size += sizeof(inum);
    dd->ents += 1;

    inode_t* nnode = get_inode(inum); //add a reference to the inodes that r attached 
    nnode->refs += 1;
    return 0;
} 

// deletes given name from directory

 int directory_delete(inode_t* dd, const char* name)
{
    printf(" + directory_delete(%s)\n", name);
    printf("%d\n", dd->size);

    char* data = blocks_get_block(inode_get_bnum(dd, 0));
    char* text = data;
    char* end = 0;

    for (int i = 0; i < dd->ents; ++i) { //delete the name for teh amount of time it exists 
        if (strcmp(data, name) == 0) { //data is name (on the right spot )
            
            char* curr = data; 
            while (*curr != 0) {
             curr++;
            }
            end = curr + 1;
            
            int inum = *((int*)end);

            memmove(data, end, dd->size); //delete
            dd->ents -= 1; //one less entry in directory 

            inode_t* sub = get_inode(inum);

            sub->refs-=1; //one less ref in inode 

             if (sub->refs < 1) {
                free_inode(inum);
            }
            return 0;
        }
            char* curr = data;
            while (*curr != 0) {
             curr++;
            }
            data = curr + dd->ents;
        
    }
      return -ENOENT;
}

//creates a list of blocks essentialy 
slist_t*
directory_list(const char* path)
{
    int inum = tree_lookup(path);
    inode_t* dd = get_inode(inum);
    char* data = blocks_get_block(inode_get_bnum(dd, 0));
   

    printf("+ directory_list()\n");
    slist_t* alist = 0;

    for (int i = 0; i < 128; ++i) {  //for amount of inodes 
        data = data + 1;
        int bnum = *((int*) data);
        data += sizeof(int); //match size so divisible 

        printf(" - %d: %s [%d]\n", i, data, bnum);
        alist = s_cons(data, alist); //cons (linked list!!!)
    }

    return alist;
}

