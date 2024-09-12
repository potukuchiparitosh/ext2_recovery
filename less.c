#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdint.h>
#include "less.h"
int read_superblock(int fd, struct ext2_super_block *sb)
{
        if(lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET)) {
                perror("Failed to seek superblock");
                return -1;
        }
        if(read(fd, sb, SUPERBLOCK_SIZE) != SUPERBLOCK_SIZE) {
                perror("Failed to read superblock");
                return -1;
        }
        return 0;
}
int read_block_group_desc(int fd, struct ext2_group_desc *egd, struct ext2_super_block *sb, int group_number)
{
        int blocksize = 1024 << sb->s_log_block_size;
        int blockgroup_size = blocksize * sb->s_blocks_per_group;//Size of a block group in bytes
        int block_group_desc_offset_local = (SUPERBLOCK_OFFSET + SUPERBLOCK_SIZE + blocksize - 1)/blocksize *blocksize;//in bytes
        int blockgroup_desc_offset = blockgroup_size * (group_number - 1) + block_group_desc_offset_local;

        if(lseek(fd, blockgroup_desc_offset, SEET_SET) < 0) {
                perror("Failed to seek to block group descriptor");
                return -1;
        } 
        if(read(fd, egd, sizeof(struct ext2_group_desc)) != sizeof(struct ext2_group_desc)) {
                perror("Failed to read block group descriptor");
                return -1;
        }
        return 0;
}
int test_inode_in_bitmap(struct ext2_super_block sb, int inode_number, uint8_t *inode_bitmap, int block_group_number)
{
        /*
inode_number : This is the number of the inode that needs to be tested
inode_bitmap : This is a pointer to the array that contains the inode bitmap
block_group_number : This is current block group that we are calling this function from(while traversing the inodes in this block group)
         */
        int inode_number_local = (inode_number % sb.s_inodes_per_group);
        byte_index = inode_number_local / 8;
        bit_index = inode_number_local % 8;
        if(((bitmap[byte_index] << (bit_index - 1)) >> 7))
                return 0;
        else
                return 1;

}
void set_inode_in_bitmap(struct ext2_super_block sb, int inode_number, uint8_t *inode_bitmap, int block_group_number)
{
        /*
inode_number : This is the number of the inode that needs to be tested
inode_bitmap : This is a pointer to the array that contains the inode bitmap
block_group_number : This is current block group that we are calling this function from(while traversing the inodes in this block group)
         */
        int inode_number_local = (inode_number % sb.s_inodes_per_group);
        byte_index = inode_number_local / 8;
        bit_index = inode_number_local % 8;
        bitmap[byte_index] = bitmap[byte_index] | (1 << (7 - bit_index));

}
int main(int argc, char *argv[])
{
        int fd = open(argv[1], O_RDONLY);
        int number_of_block_groups, i, inode_number_global;
        struct ext2_super_block sb;
        struct ext2_group_desc egd;
        struct ext2_inode current_inode;
        uint8_t *inode_bitmap, *block_bitmap;


        if(fd < 0) {
                perror("Failed to open disk image\n");
                return 1;
        }
        if(read_superblock(fd, &sb) < 0) {
                close(fd);
                return 1;
        }
        int blocksize = 1024 << sb->s_log_block_size;
        inode_bitmap = malloc(blocksize);
        if(sb.s_magic != 0xEF53) {
                fprintf(stderr, "Invalid ext2 filesystem (magic number did not match)\n");
                close(fd);
                return 1;
        }


        number_of_block_groups = sb.s_blocks_count / sb.s_blocks_per_group;
        for ( i = 0; i < number_of_block_groups; i++) {


                if(read_block_group_desc(fd, &egd, &sb, i) != sizeof(struct ext2_group_desc)) {
                        perror("Failed to read block gruop descriptor");
                        return -1;
                }

                //Read inode_bitmap

                bitmap_offset = egd.bg_inode_bitmap * blocksize;
                if(lseek(fd, bitmap_offset, SEEK_SET) < 0) {
                        perror("Could not lseek to inode bitmap");
                        return -1;
                }
                if(read(fd, inode_bitmap, blocksize) != blocksize) {
                        perror("Failed to read blocksize number of bytes during inode bitmap read");
                        return -1;
                }

                //Read block_bitmap

                bitmap_offset = egd.bg_block_bitmap * blocksize;
                if(lseek(fd, bitmap_offset, SEEK_SET) < 0) {
                        perror("Could not lseek to block bitmap");
                        return -1;
                }
                if(read(fd, block_bitmap, blocksize)) {
                        perror("Failed to read block bitmap");
                        return -1;
                }
                //Traverse the inode_table and compare the values with the values in the inode_bitmap

                for ( j = 0; j < sb.s_inodes_per_group; j++) {
                        current_inode_offset = egd.bg_inode_table + j * sizeof(struct ext2_inode); 
                        inode_number_global++;//Keeps track of the current inode's number
                        if(lseek(fd, current_inode_offset,SEEK_SET) < 0) {
                                perror("Failed to lseek to the current_inode_offset");
                                return -1;
                        }
                        if(read(fd, current_inode, sizeof(struct ext2_inode)) != sizeof(struct ext2_inode)) {
                                perror("Failed to read inode structure");
                                return -1;
                        }

                        if(current_inode.i_mode != 0 && !test_inode_in_bitmap(sb, inode_number_global, inode_bitmap, i)) {
                                set_inode_in_bitmap(sb, inode_number_global, inode_bitmap, i);
                        }
                }

                /*
                   To traverse the list of blocks in each block group we must go to each inode and traverse the blocks pointed to by this inode. Lets start
                   */

                inode_number_global = 0;
                for ( j = 0; j < sb.s_inodes_per_group; j++) {
                        inode_number_global++;
                        current_inode_offset = egd.bg_inode_table + j * sizeof(struct ext2_inode);
                        if (lseek(fd, current_inode_offset, SEEK_SET) < 0) {
                                perror("Failed to lseek");
                                return -1;
                        }
                        if(read(fd, current_inode, sizeof(struct ext2_inode))) {
                                perror("Failed to read");
                                return -1;
                        }
                        for ( k = 0; k < 12 ; k++) {// For direct blocks
                                if(!memcmp(current_inode.i_block[k],signature, 32) && test_block_in_bitmap())
                                       set_block_in_bitmap(); 
                        } 

                }






        

        }

        return 0; 
}
