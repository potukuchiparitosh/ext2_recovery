#include <ext2fs/ext2fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct inode_map {
        ext2_ino_t inode;
        ext2_ino_t parent_inode;
        struct inode_map *next;
} inode_map_t;

void add_inode_relationship(inode_map_t **head, ext2_ino_t inode, ext2_ino_t parent_inode) {
        inode_map_t *new_node = malloc(sizeof(inode_map_t));
        new_node->inode = inode;
        new_node->parent_inode = parent_inode;
        new_node->next = *head;
        *head = new_node;
}

ext2_ino_t find_parent_inode(inode_map_t *head, ext2_ino_t inode) {
        inode_map_t *current = head;
        while (current != NULL) {
                if (current->inode == inode) {
                        return current->parent_inode;
                }
                current = current->next;
        }
        return 0;
}

int map_directory(ext2_filsys fs, ext2_ino_t dir_ino, int entry, struct ext2_dir_entry *dirent, int offset, int blocksize, char *buf, void *private) {
        inode_map_t **inode_map = (inode_map_t **)private;

        if (dirent->name_len > 0) {
                if (strncmp(dirent->name, ".", dirent->name_len) != 0 && strncmp(dirent->name, "..", dirent->name_len) != 0) {
                        add_inode_relationship(inode_map, dirent->inode, dir_ino);
                }
        }
        return 0;
}

void build_inode_map(ext2_filsys fs, ext2_ino_t ino, inode_map_t **inode_map) {
        ext2_inode_scan scan;
        struct ext2_inode inode;
        errcode_t retval;

        retval = ext2fs_open_inode_scan(fs, 0, &scan);
        if (retval) {
                fprintf(stderr, "Failed to open inode scan: %s\n", error_message(retval));
                return;
        }

        while ((retval = ext2fs_get_next_inode(scan, &ino, &inode)) == 0) {
                if (LINUX_S_ISDIR(inode.i_mode)) {
                        retval = ext2fs_dir_iterate(fs, ino, 0, NULL, map_directory, inode_map);
                        if (retval) {
                                fprintf(stderr, "Failed to iterate directory: %s\n", error_message(retval));
                        }
                }
        }

        ext2fs_close_inode_scan(scan);
}

int verify_and_correct_directory_entries(ext2_filsys fs, ext2_ino_t dir_ino, inode_map_t *inode_map) {
        struct ext2_inode inode;
        errcode_t retval;
        blk_t *block_nr;
        int block_cnt;
        char block_buf[fs->blocksize];

        retval = ext2fs_read_inode(fs, dir_ino, &inode);
        if (retval) {
                fprintf(stderr, "Failed to read inode: %s\n", error_message(retval));
                return retval;
        }

        retval = ext2fs_block_iterate(fs, dir_ino, 0, block_buf, (block_iterate_func_t)map_directory, &inode_map);
        if (retval) {
                fprintf(stderr, "Failed to iterate blocks: %s\n", error_message(retval));
                return retval;
        }

        for (block_cnt = 0; block_cnt < EXT2_NDIR_BLOCKS; block_cnt++) {
                block_nr = inode.i_block + block_cnt;
                if (*block_nr == 0) continue;

                retval = io_channel_read_blk(fs->io, *block_nr, 1, block_buf);
                if (retval) {
                        fprintf(stderr, "Failed to read block: %s\n", error_message(retval));
                        continue;
                }

                struct ext2_dir_entry *dirent = (struct ext2_dir_entry *)block_buf;
                while ((char *)dirent < block_buf + fs->blocksize) {
                        if (strncmp(dirent->name, ".", dirent->name_len) == 0 && dirent->inode != dir_ino) {
                                printf("Fixing '.' entry in directory inode %u\n", dir_ino);
                                dirent->inode = dir_ino;
                        } else if (strncmp(dirent->name, "..", dirent->name_len) == 0) {
                                ext2_ino_t parent_ino = find_parent_inode(inode_map, dir_ino);
                                if (parent_ino && dirent->inode != parent_ino) {
                                        printf("Fixing '..' entry in directory inode %u\n", dir_ino);
                                        dirent->inode = parent_ino;
                                }
                        }

                        dirent = (struct ext2_dir_entry *)((char *)dirent + dirent->rec_len);
                }

                retval = io_channel_write_blk(fs->io, *block_nr, 1, block_buf);
                if (retval) {
                        fprintf(stderr, "Failed to write block: %s\n", error_message(retval));
                }
        }

        return 0;
}

int main(int argc, char *argv[]) {
        ext2_filsys fs;
        errcode_t retval;
        const char *device = "example.img";
        ext2_ino_t root_ino = EXT2_ROOT_INO;
        inode_map_t *inode_map = NULL;

        retval = ext2fs_open(device, EXT2_FLAG_RW, 0, 0, unix_io_manager, &fs);
        if (retval) {
                fprintf(stderr, "Failed to open filesystem: %s\n", error_message(retval));
                return 1;
        }

        build_inode_map(fs, root_ino, &inode_map);

        retval = verify_and_correct_directory_entries(fs, root_ino, inode_map);
        if (retval) {
                fprintf(stderr, "Failed to verify and correct directory entries: %s\n", error_message(retval));
        }

        // Cleanup inode map
        inode_map_t *current = inode_map;
        while (current != NULL) {
                inode_map_t *tmp = current;
                current = current->next;
                free(tmp);
        }

        ext2fs_close(fs);
        return 0;
}

