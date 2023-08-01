/**
 * finding_filesystems
 * CS 341 - Spring 2023
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

void* get_block(file_system* fs, inode* parent, uint64_t idx){
  data_block_number* block;
  if (idx < NUM_DIRECT_BLOCKS){
    block = parent -> direct;
  } else {
    block = (data_block_number*)(fs -> data_root + parent -> indirect);
    idx -= NUM_DIRECT_BLOCKS;
  }
  void* res = (void*) (fs -> data_root + block[idx]);
  return res;
}

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    inode* node = get_inode(fs, path);
    if (node == NULL){
        errno = ENOENT;
        return -1;
    }
    uint16_t ut = node->mode >> RWX_BITS_NUMBER;
    node -> mode = new_permissions | (ut << RWX_BITS_NUMBER);
    clock_gettime(CLOCK_REALTIME, &(node -> ctim));
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    inode* node = get_inode(fs, path);
    if (node == NULL) {
        errno = ENOENT;
        return -1;
    }
    if (owner != ((uid_t)-1)) {
        node -> uid = owner;
    }
    if (group != ((gid_t)-1)) {
        node -> gid = group;
    }
    clock_gettime(CLOCK_REALTIME, &(node -> ctim));
    return -1;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    inode* node = get_inode(fs, path);
    if (node != NULL) {
        return NULL;
    }
   
    const char* filename;
    inode* parent = parent_directory(fs, path, &filename);
    if (!valid_filename(filename)) {
        return NULL;
    }
    if (parent == NULL || !is_directory(parent)) {
        return NULL;
    }
    inode_number new_index = first_unused_inode(fs);
    if (new_index == -1) {
        return NULL;
    }
    inode* new_node = fs -> inode_root + new_index;
    init_inode(parent, new_node);
    minixfs_dirent md;
    md.inode_num = new_index;
    md.name = (char*) filename;
    if ((parent -> size / sizeof(data_block)) >= NUM_DIRECT_BLOCKS) {
        return NULL;
    }
    int offset = parent -> size % sizeof(data_block);
    if (!offset && (add_data_block_to_inode(fs, parent) == -1)) {
        return NULL;
    }
    void* start = get_block(fs, parent, (parent -> size / sizeof(data_block))) + offset;
    memset(start, 0, FILE_NAME_ENTRY);
    make_string_from_dirent(start, md);
    parent -> size += MAX_DIR_NAME_LEN;
    return new_node;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
        size_t num = 0;
        char *map = GET_DATA_MAP(fs->meta);
        uint64_t i = 0;
        for (; i < fs->meta->dblock_count; i++) {
            if (map[i] != 0) {
                num++;
            }
        }
        char* info = block_info_string(num);
        size_t size = strlen(info);
        if (*off > (long) size) {
            return 0;
        }
        if (count > size - *off) {
            count = size - *off;
        }
        memmove(buf, info + *off, count);
        *off += count;
        return count;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
        inode* node = get_inode(fs, path);
    if (node == NULL) {
        node = minixfs_create_inode_for_path(fs, path);
        if (node == NULL) {
            errno = ENOSPC;
            return -1;
        }
    }
    size_t max_size = sizeof(data_block) * (NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS);
    if (count + *off > max_size) {
        errno = ENOSPC;
        return -1;
    }
    int required_block = (count + *off + sizeof(data_block) - 1) / sizeof(data_block);
    if (minixfs_min_blockcount(fs, path, required_block) == -1) {
        errno = ENOSPC;
        return -1;
    }
    size_t block_index = *off / sizeof(data_block);
    size_t block_offset = *off % sizeof(data_block);
    uint64_t size = 0;
    if (count > (sizeof(data_block) - block_offset)) {
        size = (sizeof(data_block) - block_offset);
    } else {
        size = count;
    }
    void* block = get_block(fs, node, block_index) + block_offset;
    memcpy(block, buf, size);
    *off += size;
    size_t write_count = size;
    block_index++;
    while (write_count < count) {
        if (count - write_count > sizeof(data_block)) {
            size = sizeof(data_block);
        } else {
            size = count - write_count;
        }
        block = get_block(fs, node, block_index);
        memcpy(block, buf + write_count, size);
        block_index++;
        write_count += size;
        *off += size;
    }
    if (count + *off > node -> size) {
        node -> size  = count + *off;
    }
    clock_gettime(CLOCK_REALTIME, &(node->mtim));
    clock_gettime(CLOCK_REALTIME, &(node->atim));
    return write_count;
    return -1;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    inode* node = get_inode(fs, path);
    if (node == NULL) {
        errno = ENOENT;
        return -1;
    }
    if ((uint64_t)*off > node -> size) {
        return 0;
    }
    size_t block_index = *off / sizeof(data_block);
    size_t block_offset = *off % sizeof(data_block);
    if (node -> size - *off < count) {
        count = node -> size - *off;
    }
    uint64_t size = 0;
    if ((sizeof(data_block) - block_offset) < count) {
        size = (sizeof(data_block) - block_offset);
    } else {
        size = count;
    }
    void* block = get_block(fs, node, block_index) + block_offset;
    memcpy(buf, block, size);
    *off += size;
    size_t read_count = size;
    block_index++;
    while (read_count < count) {
        if (count - read_count > sizeof(data_block)) {
            size = sizeof(data_block);
        } else {
            size = count - read_count;
        }
        block = get_block(fs, node, block_index);
        memcpy(buf + read_count, block, size);
        block_index++;
        read_count += size;
        *off += size;
    }
    clock_gettime(CLOCK_REALTIME, &(node -> atim));
    return read_count;
    return -1;
}
