#include "mksofs.h"

#include "superblock.h"
#include "exception.h"
//Include adicionado por Pedro Silva por causa do IPB
#include "inode.h"
//Include adicionado por Pedro Silva por causa do NULL_REFERENCE
#include "core.h"
//Include adicionado por Diogo Ferreira por causa do NULL_CLUSTER
#include "cluster.h"
//Include adicionado por Diogo Ferreira por causa do getuid e getgid
#include <unistd.h>

#include <errno.h>
#include <time.h>


/*
 * filling in the inode table:
 *   only inode 0 is in use (it describes the root directory)
 */
void fillInInodeTable(SOSuperBlock * p_sb)
{

  SOInode inode_table[IPB];
  uint32_t inode_number, i, block_number;
  uint32_t cluster_size = p_sb->csize*BLOCK_SIZE;

  /*Load the contents of all blocks of the table of inodes from devide storage into internal storage and fill them as free inodes*/

  for(block_number = 1; block_number<p_sb->itsize; block_number++){
    for(inode_number = 0; inode_number < IPB; inode_number++){

       inode_table[inode_number].mode = INODE_FREE; /* flag signaling the inode is free */
       inode_table[inode_number].refcount = 0;
       inode_table[inode_number].owner = 0;
       inode_table[inode_number].group = 0;
       inode_table[inode_number].size = 0;
       inode_table[inode_number].csize = 0;
       if (block_number==p_sb->itsize-1 && inode_number==IPB-1)
        inode_table[inode_number].next = NULL_REFERENCE;
       else
        inode_table[inode_number].next = block_number*IPB+(inode_number+1);  /* reference to the next inode in the double-linked list of free inodes*/
       inode_table[inode_number].ctime = 0;
       inode_table[inode_number].mtime = 0;
       for(i = 0; i < N_DIRECT; i++){
         inode_table[inode_number].d[i] = NULL_REFERENCE; /*put all direct and indirect references to the data clusters pointing null*/
       }
       for(i = 0; i < N_INDIRECT; i++){
          inode_table[inode_number].i1[i] = NULL_REFERENCE; /*put all direct and indirect references to the data clusters pointing null*/
       }
       inode_table[inode_number].i2 = NULL_REFERENCE;
    }
    soWriteRawBlock(p_sb->itstart+block_number, &inode_table);
  }


  /*Fill in inode 0 -> root directory*/
  inode_table[0].mode = INODE_RD_USER | INODE_WR_USER | INODE_EX_USER | INODE_RD_GROUP | INODE_WR_GROUP | INODE_EX_GROUP | INODE_RD_OTH | INODE_WR_OTH |INODE_EX_OTH | INODE_DIR;
  
  /*flag signaling if the inode describes a directory and its permissions*/
  inode_table[0].refcount = 2; /* references to the previous and own directory */
  inode_table[0].owner = getuid(); 
  inode_table[0].group = getgid();
  inode_table[0].size = cluster_size; /* the size of the first inode is one cluster's size */
  inode_table[0].csize = 1;
  inode_table[0].atime = time(NULL); /* time.h */
  
  inode_table[0].d[0] = 0; /* the first direct reference points to cluster 0 */

  /*referencias a clusters a NULL*/
  for(i = 1; i < N_DIRECT; i++){
    inode_table[0].d[i] = NULL_CLUSTER;
  }

  for(i = 1; i < N_INDIRECT; i++){
    inode_table[0].i1[i] = NULL_CLUSTER;
  }

  inode_table[0].i2 = NULL_CLUSTER;

  for(inode_number = 1; inode_number < IPB; inode_number++){

       inode_table[inode_number].mode = INODE_FREE; /* flag signaling the inode is free */
       inode_table[inode_number].refcount = 0;
       inode_table[inode_number].owner = 0;
       inode_table[inode_number].group = 0;
       inode_table[inode_number].size = 0;
       inode_table[inode_number].csize = 0;
       inode_table[inode_number].next = inode_number+1;  /* reference to the next inode in the double-linked list of free inodes*/
       inode_table[inode_number].ctime = 0;
       inode_table[inode_number].mtime = 0;
       for(i = 0; i < N_DIRECT; i++){
         inode_table[inode_number].d[i] = NULL_REFERENCE; /*put all direct and indirect references to the data clusters pointing null*/
       }
       for(i = 0; i < N_INDIRECT; i++){
          inode_table[inode_number].i1[i] = NULL_REFERENCE; /*put all direct and indirect references to the data clusters pointing null*/
       }
       inode_table[inode_number].i2 = NULL_REFERENCE;
    }
  soWriteRawBlock(p_sb->itstart, &inode_table);

} 
		 
	

