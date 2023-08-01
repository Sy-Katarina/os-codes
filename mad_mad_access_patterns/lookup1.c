/**
 * mad_mad_access_patterns
 * CS 341 - Spring 2023
 */
#include "tree.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/

int search(FILE* file, char* word, long offset) {
  if (offset == 0) {
    return 0;
  }
  BinaryTreeNode node;
  fseek(file, offset, SEEK_SET);
  fread(&node, sizeof(BinaryTreeNode), 1, file);
  char c;
  int l = 0;
  while ((c = fgetc(file)) != '\0' && c != EOF) {
    l++;
  }
  fseek(file, offset + sizeof(BinaryTreeNode), SEEK_SET);
  char words[l];
  fread(words, l, 1, file);
  if (strcmp(word, words) == 0){
    printFound(words, node.count, node.price);
    return 1;
  }
  if (strcmp(word, words) < 0){
    if (search(file,  word, node.left_child)){
      return 1;
    }
  }
  if (strcmp(word, words) > 0){
    if (search(file,  word, node.right_child)){
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
    FILE* file = fopen(filename, "r");
    if (!file) {
      openFail(filename);
      exit(2);
    }
    char buffer[4];
    fread(buffer, 1, 4, file);
    if (strcmp(buffer, "BTRE")) {
      formatFail(filename);
      exit(2);
    }
    
    for (int i = 2; i < argc; i++) {
      if (!search(file, argv[i], (long)4)) {
        printNotFound(argv[i]);
      }
    }
    fclose(file);
    return 0;
}
