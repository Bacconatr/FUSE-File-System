#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>


#include "helpers/inode.h"
#include "helpers/blocks.h"
inode_t* get_inode(int inum)
{
    if (inum < 128) {
	inode_t* nodes= (inode_t*)((uint8_t*) blocks_get_block(0) + BLOCK_BITMAP_SIZE);
    return &(nodes[inum]);
}
}

// allocates first available inode slot
int alloc_inode()
{
	
	 for (int i = 0; i < 128; ++i) { //for every inode slot 
        inode_t* node = get_inode(i);
        if (node->mode == 0) {
            memset(node, 0, sizeof(inode_t));
            node->refs = 0;
            node->mode = 010644; //file
            node->size = 0;
	        node->block = alloc_block(); //alloc
            printf("+ alloc_inode() -> %d\n", i);
            return i;
        }
    }

    return -1;
	
   
}

// frees the given inode
void free_inode(int inum) {
	printf("+ free_inode(%d)\n", inum);

	inode_t* node = get_inode(inum);

	if(node->refs > 0) {
	    puts("cannot free");
	    exit(1);
	}
	free_block(node->block); //free
	memset(node, 0, sizeof(inode_t)); //set mem to inode 
}

// shrinks given inode to size
int shrink_inode(inode_t* node, int size) {
	// update the size of the node
	node->size = size; //size = size 
	return 0;
}

// grows given inode to specified size
int grow_inode(inode_t* node, int size) {

	node->size = size; // size = size 
	return 0;
}

	

//get block number of the given indo and fbnum
int inode_get_bnum(inode_t* node, int fbnum)
{
   int blockIndx = fbnum / 4096;
     if(blockIndx == 0) {
		return node->block;
	} 
} 
