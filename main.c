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
void verify_dot_and_dot_dot(blk_t blocknr, size_t blocksize)
{
        /*
           We enter this function once it is determined that the block that we will be working on in this function will be a block that contains directory entries
           The steps to verify and correct . and .. fields in the directory entries and ensure that they point to the correct 
           inode will be done in the following way:
           1. Read the block into a buffer using the function io_channel_read_blk.
           2. Traverse this block, rec_len at a time, and check which entries are . and ..
           3. If we find some entries like that, we simply fill the value of the inode number that is supposed to be in the structure ext2_dir_entry->inode.
           4. One thing to be noted is that the current inode number should be straightforward to pickup, as it comes from the inode whos blocks we are currently traversing. The parent inode is what is actually concerning and needs to be looked at.

           */
}
int every_block(ext2_filsys fs, blk_t *blocknr, int blockcnt, void *private) 
{
        char block_buf[BLOCK_SIZE];
        int retval;
        retval = io_channel_read_blk(fs->io, *blocknr, 1 , block_buf);//This function reads one block into the buffer
        if(is_data_block(block_buf) && !test)
                ext2fs_mark_block_bitmap(fs->block_map, *blocknr);
        else if(is_directory_block(block_buf, BLOCK_SIZE) && !test) {
                ext2fs_mark_block_bitmap(fs->block_map, *blocknr);
                verify_dot_and_dot_dot(*blocknr, blocksize);
        }
        else if(is_pointer_block(block_buf, blocksize)) {
                ext2fs_mark_block_bitmap(fs->block_map, *blocknr);
                iterate_till_depth(*blocknr, blocksize); 
        }
        return 0;
        /*
           Let us see what we are trying to do here:
           In this function we are given the block number. We get the corresponding block into a buffer and test using the function  is_pointer_block whether the block that we are looking at is a pointer block or not. If it is a pointer block we need take the following steps:
           1. Send the block number of this block ahead through a function called iterate_over_depth
           2. Inside this function we read the corresponding block for the block number sent.
           3. Next we iterate through the contents of this block, setting the bitmap for each block address we encounter.
           4. In addition to setting the bitmap values for each block address, we also read the corresponding block into a new buffer and test whether this block is a pointer block too. If yes, we call the function iterate_over_depth on this block number, otherwise we dont do anything.
           */



}
void unreferenced_pointer_blocks_matching(ext2_filsys fs) 
{

        /*
           The section here will be dedicated to correcting errors related to unreferenced indirect blocks. 
           The task to be done here is to associate the inode with the block which correctly fits in and satisfies the file size paramter associated with that inode.
           */
        /*
STEPS:
1. Traverse all the blocks in the filesystem, by starting at the first block as given in the super_block structure till the end , which I think is also given in the super block structure.
2. At each block test whether this block is being reflected in the block bitmap. If not, copy this into a buffer.
3. Check if this block is an indirect block. If it is, we need to calculate the number of blocks this block points to.
4. Now we have some information about a block that is unreferenced, and we can check if any inode requires this block to complete its its size requirement.
5. For this we traverse the inodes in the filesystem, For each inode we check the filesize and also check the current file size using information through i_block, since we can check how many data blocks are being used by this inode. We then calculate the shortage of data blocks and compare it with the size the block can provide, and whether we can satisfy this need.
6. If we can satisfy this need, the i_blocks is pointed to the block number that we currently hold, and this inode is written to the disk. 
7. The last thing to do would be to mark all the blocks that are being used now, be it direct or indirect, to be marked in the block bitmap.
           */


        for ( blk_t block = fs->super->s_first_data_block; block < fs->super_s_blocks_count; block++) {
                if(!ext2fs_test_block_bitmap(fs->block_bitmap, block)) {
                        if(read_block(fs, block, block_buf))
                                fprintf(stderr, "Failed to read block %lu\n", block);
                }
                if(is_pointer_block(block_buf, fs->blocksize)) {
                        int pointer_count = count_pointers(block_buf, fs);
                        //Now we need to find an inode with missing blocks.
                        //For this we traverse the inodes in the filesystem
                        struct ext2_inode inode;
                        for(ext2_ino ino = 1; ino <= fs->super->s_inodes_count; ino++) {
                                if(ext2fs_read_inode(fs,ino, &inode) != 0) {
                                        fprintf(stderr, "Failed to read inode %lu \n".ino);
                                        continue;
                                }

                                int required_blocks = (inode.i_size + fs->blocksize - 1)/fs->blocksize;
                                int actual_blocks = 0;
                                for (int i = 0;i<EXT2_NDIR_BLOCKS; i++) {
                                        //Number of direct blocks being used
                                        if(inode.i_block[i]!=0)
                                                actual_blocks++;
                                }
                                if (inode.i_block[EXT2_IND_BLOCK] == 0)
                                        actual_blocks += pointer_count;
                                //This is to add the number of blocks being pointed to by the indirect pointer. Remember we are currently inside the indirect pointers if condition, that is if the block that we are currently looking at is an indirect pointer. It is in this case that we are checking all the inodes in the filesystem and seeing which inodes needs more data blocks to satisfy its needs, as the block might have lost reference. So we want to make sure that we connect this block with the inode that fits exactly.
                                //Once we have done these calculations, we can actually compare the values of the required_size that is what the size should be, and the actual size according to the blocks that are associated with this inode.
                                if((required_size > actual_size) && (required_size - actual_size == pointer_count)) {
                                        //If it is a perfect match, we assign this block that we are dealing with to the i_block[INDIRECT]
                                        inode.i_block[EXT2_IND_BLOCK] = block;
                                        if(ext2fs_write_inode(fs, ino, inode)!=0) {
                                                fprintf(stderr, "Failed to write inode %lu\n", ino);
                                        }
                                        ext2fs_mark_block_bitmap(fs->block_map, block);
                                        uint32_t *pointers = (uint32_t *)block_buf;
                                        for (int i = 0; i < fs->blocksize / sizeof(uint32_t); i++) {
                                            if (pointers[i] != 0) {
                                                ext2fs_mark_block_bitmap(fs->block_map, pointers[i]);
                                            }
                                        }


                                }




                        }
                }
        }

        /*Pointer Recovery WIP*/



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
        /*Using this function we can read the inode bitmap into the fs structure that we are carrying everywhere
         */
        retval = ext2fs_read_inode_bitmap(fs);
        ret = ext2fs_open_inode_scan(fs, 0, &scan);
        /*Starting inode recovery*/
        //group_desc_count is the number of block groups present, since each block group has one group descriptor
        /*We need to go through the inode table of this particular block group. The way we do that would be to first place the 
        scan variable at the beginning of this block groups inode table list. Then use the next function to iterate over the list 
        and wait for the returned output arguments ino to become 0 so that we can conclude that we have completely exhausted this 
        block groups list.*/ 

                /*
        NOTE: This is a note on the way inode bitmaps work in ext2 filesystem. When we want to modify, read or compare the bitmap 
        values of the inode table, we do not need to worry about block groups because the function ext2fs_read_inode_bitmap(fs),
        combines all the block groups bitmaps and puts them under the fs->inode_table field. Since inode number are all unique, 
        we can simple use function to read write and compare from this consolidated bitmap itself.
                 */
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
        /*
           Okay so now I have a plan to tackle Block Bitmap Recovery which I will be detailing below:
           I will go through every inode in a block group using the values of group_desc_count and use the function 
           ext2fs_inode_scan_goto_blockgroup to place the pointer at the beginning of the inodes in a block group. 
           Then using the ext2fs_get_next_inode I will traverse the whole list of inodes in this block group and when completed 
           move on to the next block group.

           While I am at a particular inode, I need to traverse all the blocks associated with that inode.
           This will be done using the function, ext2fs_block_iterate. This function goes through each block associated with an 
           inode, and calls a function when inside each block.
           Inside this callback function(that will be called inside each block) I will check a few things and take action 
           according to that.
           If I find that the block that I am currently at is a data block, I will test if the block bitmap reflects that this 
           block is being used and has the correct bit set. If not I will do that.
           If I find that the block that I am currently at is a directory block, I will test if the block bitmap reflects that 
           this block is being used and has the correct bit set, if not I will do that.
           If I find that the block that I am currently at is a pointer block, I would have to set the bit for this block in the 
           bitmap. 
           The second thing to do would be to use the iterate over depth function to make sure that all pointers be it direct,
           indirect, double indirect and triple indirect are taken care of using the recursive function that mark all block number
           in a pointer block in the bitmap as being used.
         */
        retval = ext2fs_read_block_bitmap(fs);
        block_buf = malloc(fs->blocksize);
        /*Go through the inodes inside each block group*/
        for( i =0; i < fs->group_desc_count; i++) {
                ret = ext2fs_inode_scan_goto_blockgroup(scan, i);
                ino = 1;
                while(ino != 0) {
                        ret = ext2fs_get_next_inode(scan, &ino, inode);
                        /*At this point we should have the inode number in the ino variable, using which we can traverse the 
                        blocks associated with this inode, using the function ext2fs_block_iterate*/
                        ret = ext2fs_block_iterate(fs, ino, 0, block_buf, every_block, NULL);
                }
        }
        /* By the end of this loop we should have the block bitmap filled correctly as per the information in the filesystem*/
        /*Pointer Recovery starts*/

        return 0;
}
