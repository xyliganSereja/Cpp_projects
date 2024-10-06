#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr size_t BUFFER_SIZE = 1024;

// calculates p - prefix function for S = pattern+'\0'+data iteratively
// takes p[0...|pattern|-1], S[0...|pattern|-1] (pattern), S[i-1] and p[i-1]
// returns p[i]
size_t prefix_func(const size_t* p, const char* pattern, char last, const size_t prev) {
  size_t k = prev;
  while (k > 0 && last != pattern[k]) {
    k = p[k - 1];
  }
  if (last == pattern[k]) {
    ++k;
  }
  return k;
}

void finish(FILE* file, void* buffer) {
  fclose(file);
  free(buffer);
}

int main(int argc, char** argv) {
  if (argc != 3) {
    fputs("Incorrect number of arguments. <file_name> and <string> expected\n", stderr);
    return EXIT_FAILURE;
  }
  char* pattern = argv[2];
  FILE* file = fopen(argv[1], "rb");
  if (file == nullptr) {
    perror("Can't open input file");
    return EXIT_FAILURE;
  }
  size_t len = strlen(pattern);

  char buffer[BUFFER_SIZE];
  size_t* p = static_cast<size_t*>(malloc(len * sizeof(size_t)));
  if (p == nullptr) {
    perror("Can't allocate memory");
    finish(file, p);
    return EXIT_FAILURE;
  }
  p[0] = 0;
  for (size_t i = 1; i < len; i++) {
    p[i] = prefix_func(p, pattern, pattern[i], p[i - 1]);
  }
  size_t prev = 0;
  do {
    size_t read = fread(buffer, 1, BUFFER_SIZE, file);
    for (size_t i = 0; i < read; i++) {
      prev = prefix_func(p, pattern, buffer[i], prev);
      if (prev == len) {
        printf("Yes\n");
        finish(file, p);
        return EXIT_SUCCESS;
      }
    }
    if (ferror(file)) {
      perror("Input reading error");
      finish(file, p);
      return EXIT_FAILURE;
    }
  } while (!feof(file));

  printf("No\n");
  finish(file, p);
  return EXIT_SUCCESS;
}
