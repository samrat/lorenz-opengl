void *file_contents(const char *filename, GLint *length) {
  FILE *f = fopen(filename, "r");
  void *buffer;

  if (!f) {
    fprintf(stderr, "Unable to open %s for reading\n", filename);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  *length = ftell(f);
  fseek(f, 0, SEEK_SET);

  buffer = malloc(*length+1);
  *length = fread(buffer, 1, *length, f);
  fclose(f);
  ((char*)buffer)[*length] = '\0';

  return buffer;
}
