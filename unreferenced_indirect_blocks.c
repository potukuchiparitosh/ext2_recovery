#include <ext2fs/ext2fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a function to read a block
int read_block(ext2_filsys fs, blk_t block, char *buffer) {
        return io_channel_read_blk(fs->io, block, 1, buffer);
}

// Define a function to check if a block is an indirect pointer block
int is_indirect_block(char *block_buf, ext2_filsys fs) {
        uint32_t *pointers = (uint32_t *)block_buf;
        for (int i = 0; i < fs->blocksize / sizeof(uint32_t); i++) {
                if (pointers[i] != 0 && (pointers[i] < fs->super->s_first_data_block || pointers[i] > fs->super->s_blocks_count))
                        return 0; // Not a valid indirect block
        }
        return 1;
}

// Function to count the number of non-zero pointers in a block
int count_pointers(char *block_buf, ext2_filsys fs) {
        uint32_t *pointers = (uint32_t *)block_buf;
        int count = 0;
        for (int i = 0; i < fs->blocksize / sizeof(uint32_t); i++) {
                if (pointers[i] != 0)
                        count++;
        }
        return count;
}

// Define a function to process unreferenced indirect pointer blocks
void process_unreferenced_blocks(ext2_filsys fs) {
        char *block_buf = malloc(fs->blocksize);
        if (!block_buf) {
                fprintf(stderr, "Failed to allocate block buffer\n");
                return;
        }

        /*
           The function below traverses all the blocks in the filesystem.
           STEPS:
          1. It checks whether the blocks bitmap bit is set or not. If it is not set, we read the block into a char *buf. This is because if the block bitmap is not set for this block number it means that this block is unreferenced.
          2. We check if the block that we have is a pointer block(indirect block), by range testing. If this is the case, we count the number of pointers inside this block. This will give us an idea as to how many blocks are pointed to by this pointer block.
          3. We traverse through the list of inodes and test if this is the inode with missing blocks because of which the file size in the i_size field in the inode structure and the number of indirect blocks do not add up. The way this need could be satisfied it by searching the block list for an indirect block which points to a number of blocks which is sufficient to satisfy the need of the inode to achieve the file size required.
          4. Once we find an inode which perfectly matches the size of data pointed to by the indirect block we are currently at(in the outer loop) we can consider it a match. The next thing we need to do is point the inodes i_block[INDIRECT] field to to point to this block number(that we are currently at in the outer loop). 
          5. We then write this updated inode to the disk using the function ext2fs_write_inode
          5. One more thing remaining is to mark the indirect block as being used in the block bitmap and also mark all the blocks pointed to by the indirect block as being used, in the block bitmap

          
           */
        for (blk_t block = fs->super->s_first_data_block; block < fs->super->s_blocks_count; block++) {
                if (!ext2fs_test_block_bitmap(fs->block_map, block)) {
                        if (read_block(fs, block, block_buf) != 0) {
                                fprintf(stderr, "Failed to read block %lu\n", block);
                                continue;
                        }

                        if (is_indirect_block(block_buf, fs)) {
                                int pointer_count = count_pointers(block_buf, fs);

                                // Now we need to find an inode with missing blocks
                                struct ext2_inode inode;
                                for (ext2_ino_t ino = 1; ino <= fs->super->s_inodes_count; ino++) {
                                        if (ext2fs_read_inode(fs, ino, &inode) != 0) {
                                                fprintf(stderr, "Failed to read inode %lu\n", ino);
                                                continue;
                                        }

                                        int required_blocks = (inode.i_size + fs->blocksize - 1) / fs->blocksize;
                                        int actual_blocks = 0;
                                        for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
                                                if (inode.i_block[i] != 0)
                                                        actual_blocks++;
                                        }

                                        if (inode.i_block[EXT2_IND_BLOCK] == 0) {
                                                actual_blocks += pointer_count;
                                        }

                                        if (required_blocks > actual_blocks && (required_blocks - actual_blocks) == pointer_count) {
                                                // We have found a matching inode
                                                inode.i_block[EXT2_IND_BLOCK] = block;
                                                //Above me made the change to the inode, in its i_block array
                                                //And below we write this inode with the updated indirect block data to the disk
                                                if (ext2fs_write_inode(fs, ino, &inode) != 0) {
                                                        fprintf(stderr, "Failed to write inode %lu\n", ino);
                                                } else {
                                                        // Mark the block and the pointed blocks as used in the bitmap
                                                        /*
                                                           Here we mark the block and the pointed blocks as used in the block bitmap
                                                           */
                                                        ext2fs_mark_block_bitmap(fs->block_map, block);
                                                        uint32_t *pointers = (uint32_t *)block_buf;
                                                        for (int i = 0; i < fs->blocksize / sizeof(uint32_t); i++) {
                                                                if (pointers[i] != 0) {
                                                                        ext2fs_mark_block_bitmap(fs->block_map, pointers[i]);
                                                                }
                                                        }
                                                }
                                                break;
                                        }
                                }
                        }
                }
        }

        free(block_buf);
}

int main(int argc, char *argv[]) {
        errcode_t retval;
        ext2_filsys fs;
        char *device = "example.img";

        // Open the filesystem
        retval = ext2fs_open(device, EXT2_FLAG_RW, 0, 0, unix_io_manager, &fs);
        if (retval) {
                fprintf(stderr, "Failed to open filesystem: %s\n", error_message(retval));
                return 1;
        }

        // Process unreferenced blocks
        process_unreferenced_blocks(fs);

        // Cleanup
        ext2fs_close(fs);

        return 0;
}


