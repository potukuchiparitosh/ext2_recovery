#include<stdio.h>
#include<stdlib.h>
#include<ext2fs/ext2.h>

int is_data_block(char *block)
{
        int ret;
        ret = memcmp(block, signature, 32);
        if(ret)
                return 1;
        else
                return 0;
}
int is_directory_block(char *block, size_t blocksize)
{
        struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *)block;
        size_t offset = 0;
        while(offset < blocksize) {
                if(dir_entry->rec_len == 0)
                        return 0;
                offset += dir_entry->rec_len;
                dir_entry =(struct ext2_dir_entry *) (block + offset);
        }
        return 1;

}
int is_pointer_block(char *block,size_t blocksize)
{
        uint32_t block_buffer = (uint32_t *)block_buffer;
        size_t num_pointers = blocksize/4;
        size_t i = 0;
        for( i = 0; i < num_pointers; i++) {
                if(block_buffer[i] < 1 || block_buffer[i] > MAX_BLOCK_NUM) {
                        return 0;
                }
        }
        return 1;
} 
void iterate_till_depth(blk_t blocknr, size_t blocksize)
{
        uint32_t *buffer;
        uint32_t *block_buffer = (uint32_t *)block_buf;
        char *new_buffer;
        int retval;
        size_t i;
        size_t numblocks = blocksize/4;
        retval = io_channel_read_blk(fs->io , blocknr, 1, buffer); 

        for ( i = 0; i < num_blocks; i++) {
                retval = io_channel_read_blk(fs->io, buffer[i], 1, new_buffer);
                if(is_pointer_block(new_buffer, blocksize)) {
                        ext2fs_mark_block_bitmap(fs->block_map, buffer[i]);
                        iterate_till_depth(buffer[i], blocksize);
                } else {
                        ext2fs_mark_block_bitmap(fs->block_map, buffer[i]);
                }
        }
}
        /*
           Let us see what we are trying to do here:
           In this function we are given the block number. We get the corresponding block into a buffer and test using the function
           is_pointer_block whether the block that we are looking at is a pointer block or not. If it is a pointer block we need 
           take the following steps:
           1. Send the block number of this block ahead through a function called iterate_over_depth
           2. Inside this function we read the corresponding block for the block number sent.
           3. Next we iterate through the contents of this block, setting the bitmap for each block address we encounter.
           4. In addition to setting the bitmap values for each block address, we also read the corresponding block into a new 
           buffer and test whether this block is a pointer block too. If yes, we call the function iterate_over_depth on this 
           block number, otherwise we dont do anything.
           */
int every_block(ext2_filsys fs, blk_t *blocknr, int blockcnt, void *private) 
{
        char block_buf[BLOCK_SIZE];
        int retval;
        retval = io_channel_read_blk(fs->io, *blocknr, 1 , block_buf);
        if(is_data_block(block_buf) && !test)
                ext2fs_mark_block_bitmap(fs->block_map, *blocknr);
        else if(is_directory_block(block_buf, BLOCK_SIZE) && !test)
                ext2fs_mark_block_bitmap(fs->block_map, *blocknr);
        else if(is_pointer_block(block_buf, blocksize)) {
                ext2fs_mark_block_bitmap(fs->block_map, *blocknr);
                iterate_till_depth(*blocknr, blocksize); 
        }
        return 0;



}
int main(int argc, char *argv[])
{
        ext2_inode_scan scan;
        ino_t ino = 1;
        inode *inode;
        errcode_t ret;
        int retval;
        ext2_filsys fs;
        retval = ext2fs_open(argv[1], EXT2_FLAG_RW, 0, 0, unix_io_manager, fs);
        retval = ext2fs_read_inode_bitmap(fs);
        ret = ext2fs_open_inode_scan(fs, 0, &scan);
        /*Starting inode recovery*/
        for ( i = 0; i<fs->group_desc_count; i++) {

                ret = ext2fs_inode_scan_goto_blockgroup(scan, i);
                while(ino != 0) {
                        ret = ext2fs_get_next_inode(scan, &ino, inode);
                        if((inode->i_mode != 0) && !ext2fs_test_inode_bitmap(fs->inode_bitmap, ino))
                                ext2fs_mark_inode_bitmap(fs->inode_bitmap, ino);//void function 
                }
                ino = 1;

        }
        /*Inode recovery completed*/
        /*Block Bitmap Recovery*/
        retval = ext2fs_read_block_bitmap(fs);
        block_buf = malloc(fs->blocksize);
        /*Go through the inodes inside each block group*/
        for( i =0; i < fs->group_desc_count; i++) {
                ret = ext2fs_inode_scan_goto_blockgroup(scan, i);
                ino = 1;
                while(ino != 0) {
                        ret = ext2fs_get_next_inode(scan, &ino, inode);
                        //At this point we should have the inode number in the ino variable, using which we can traverse the blocks associated with this inode, using the function ext2fs_block_iterate
                        ret = ext2fs_block_iterate(fs, ino, 0, block_buf, every_block, NULL);
                }
        }
        return 0;
}
