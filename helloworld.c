// author: nanxis2
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Function to encrypt the input file
void encrypt_file(const char *infilename, const char *keyfilename,
                  const char *encryptfilename) {
  // Open input file for reading and writing
  int input_file = open(infilename, O_RDWR);
  if (input_file == -1) {
    fprintf(stderr, "Error: Could not open input file %s\n", infilename);
    exit(1);
  }

  // Get size of input file
  off_t input_file_size = lseek(input_file, 0, SEEK_END);
  lseek(input_file, 0, SEEK_SET);

  // Open output files for writing
  FILE *key_file = fopen(keyfilename, "wb");
  if (!key_file) {
    fprintf(stderr, "Error: Could not open output file %s\n", keyfilename);
    exit(1);
  }

  FILE *encrypt_file = fopen(encryptfilename, "wb");
  if (!encrypt_file) {
    fprintf(stderr, "Error: Could not open output file %s\n", encryptfilename);
    exit(1);
  }

  // Map input file into memory
  char *input_buffer = mmap(NULL, input_file_size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, input_file, 0);
  if (input_buffer == MAP_FAILED) {
    fprintf(stderr, "Error: Could not map input file into memory\n");
    exit(1);
  }

  // Generate random key
  unsigned char *key = (unsigned char *)malloc(input_file_size);
  FILE *urandom = fopen("/dev/urandom", "r");
  fread(key, sizeof(unsigned char), input_file_size, urandom);
  fclose(urandom);

  // Write random key to output file1
  fwrite(key, sizeof(unsigned char), input_file_size, key_file);

  // Encrypt input file using key and write to output file2
  for (off_t i = 0; i < input_file_size; i++) {
    input_buffer[i] ^= key[i];
  }
  fwrite(input_buffer, sizeof(char), input_file_size, encrypt_file);

  // Overwrite input file with zeroes
  memset(input_buffer, 0, input_file_size);

  // Unmap input file from memory
  if (munmap(input_buffer, input_file_size) == -1) {
    fprintf(stderr, "Error: Could not unmap input file from memory\n");
    exit(1);
  }

  // Close input and output files
  close(input_file);
  fclose(key_file);
  fclose(encrypt_file);

  // Free memory used by key
  free(key);
}

// function for decrypting the file
void decrypt(const char *out_filename, const char *key_filename,
             const char *encrypted_filename) {
  // Verify that the input files exist and are the same size
  struct stat stat_key, stat_encrypted;
  if (stat(key_filename, &stat_key) != 0 ||
      stat(encrypted_filename, &stat_encrypted) != 0) {
    printf("Hello World\n");
    exit(EXIT_SUCCESS);
  }
  if (stat_key.st_size != stat_encrypted.st_size) {
    printf("Hello World\n");
    exit(EXIT_SUCCESS);
  }
  size_t key_size = stat_key.st_size;

  // Open the key files for reading
  int key_fd = open(key_filename, O_RDONLY);
  int encrypted_fd = open(encrypted_filename, O_RDONLY);
  if (key_fd == -1 || encrypted_fd == -1) {
    perror("Error opening key file");
    exit(EXIT_FAILURE);
  }

  // Open the original file for writing
  int output_fd = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (output_fd == -1) {
    perror("Error opening output file");
    exit(EXIT_FAILURE);
  }

  // Map the key files into memory
  char *key_data = mmap(NULL, key_size, PROT_READ, MAP_SHARED, key_fd, 0);
  char *encrypted_data = mmap(NULL, key_size, PROT_READ, MAP_SHARED, encrypted_fd, 0);
  if (key_data == MAP_FAILED || encrypted_data == MAP_FAILED) {
    perror("Error mapping key file into memory");
    exit(EXIT_FAILURE);
  }

  // Decrypt the data and write it to the output file
  char *output_data = malloc(key_size);
  for (size_t i = 0; i < key_size; i++) {
    output_data[i] = key_data[i] ^ encrypted_data[i];
  }
  write(output_fd, output_data, key_size);

  // Unmap the key file and encrypted file from memory
  if (munmap(key_data, key_size) == -1 ||
      munmap(encrypted_data, key_size) == -1) {
    perror("Error unmapping key files from memory");
    exit(EXIT_FAILURE);
  }

  // Close the files and delete the key files
  close(key_fd);
  close(encrypted_fd);
  close(output_fd);
  remove(key_filename);
  remove(encrypted_filename);
}

int main(int argc, char **argv) {
  if (strcmp(argv[0], "./encrypt") == 0 && argc == 4) {
    const char *infile = argv[1];
    const char *keyfile = argv[2];
    const char *encryptfile = argv[3];
    encrypt_file(infile, keyfile, encryptfile);
  } else if (strcmp(argv[0], "./decrypt") == 0 && argc == 4) {
    const char *infile = argv[1];
    const char *keyfile = argv[2];
    const char *encryptfile = argv[3];
    decrypt(infile, keyfile, encryptfile);
  } else {
    printf("Hello World");
    exit(EXIT_SUCCESS);
  }
  return 0;
}
