/**
 * mad_mad_access_patterns
 * CS 341 - Spring 2023
 */
#include "tree.h"
#include "utils.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

int search(long offset, char* buffer, char* word) {
  if(offset == 0) {
    return 0;
  } 
  BinaryTreeNode* node = (BinaryTreeNode *) (buffer + offset);
  if (!strcmp(word, node->word)){
    printFound(node->word, node->count, node->price);
    return 1;
  }
  if (strcmp(word, node->word) < 0) {
    if (search(node->left_child, buffer, word)){
      return 1;
    }
  }
  if (strcmp(word, node->word) > 0) {
    if (search(node->right_child, buffer, word)){
      return 1;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
      printArgumentUsage();
      exit(1);
    }
    char* filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
      openFail(filename);
      exit(2);
    }
    struct stat s;
    if (fstat(fd, &s)) {
      openFail(filename);
      exit(2);
    }
    char* buffer = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == (void*) -1) {
      mmapFail(filename);
      exit(2);
    }
    if (strncmp(buffer, "BTRE", 4)) {
      formatFail(filename);
      exit(2);
    }
    
    for (int i = 2; i < argc; i++) {
      if (!search((long)4, buffer, argv[i])) {
        printNotFound(argv[i]);
      }
    }
    close(fd);
    return 0;
}

