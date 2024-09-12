# ext2_recovery

This program is designed to recover data from a corrupted EXT2 filesystem, focusing on three major types of corruption: inode bitmap corruption, block bitmap corruption, and unreferenced indirect block pointers. The recovery tool is built with the use of libext2fs.

# Features

  **Inode Bitmap Corruption Recovery**:
        The inode bitmap keeps track of which inodes are in use. In case of corruption in this bitmap, the recovery program attempts to scan the filesystem’s inode table and reconstruct the bitmap by determining which inodes are still valid and in use, based on their data blocks and links.

**Block Bitmap Corruption Recovery**:
        The block bitmap marks which data blocks are used and free. When corrupted, this program inspects all the data blocks referenced by the inodes in the inode table and reconstructs the block bitmap, making sure to mark blocks that contain valid data as occupied.

**Recovery from Unreferenced Indirect Block Pointers**:
        Indirect block pointers are used to reference data blocks in larger files. If these pointers are unreferenced or lost, the program scans the filesystem to identify any indirect blocks that are not connected to an inode and restores their links appropriately.

# Key Steps in Recovery

**Superblock and Group Descriptor Inspection**:
        The program starts by reading and validating the superblock and group descriptor tables. If any inconsistencies are found, it attempts to repair them by cross-referencing known good values.

**Inode Table Traversal**:
        After validating the superblock, the recovery tool scans the inode table for each block group. It verifies the structure of each inode and attempts to recover corrupted inodes by checking the consistency between their size, block pointers, and file types.

**Rebuilding Inode and Block Bitmaps**:
        By scanning all the inodes and their associated data blocks, the tool rebuilds the inode and block bitmaps from scratch, ensuring that valid inodes and blocks are properly marked as allocated, while unallocated ones are marked as free.

**Indirect Block Pointer Recovery**:
        The program handles files that use single, double, and triple indirect block addressing by scanning indirect block tables and restoring any lost or unreferenced indirect block pointers.


# Usage

The file to be used for inode and block bitmap recovery is main.c, and the the file for unreferenced indirect blocks is named, unreferenced_indirect_blocks.c, accordingly.
To use these files on your system, run the following command 

       sudo apt install e2fsprogs e2fslibs-dev


This will install libext2fs, which is used by the program.
