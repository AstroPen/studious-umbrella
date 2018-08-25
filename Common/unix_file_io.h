
#ifndef _UNIX_FILE_IO_H_
#define _UNIX_FILE_IO_H_

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

enum File_IO_Error : int {
  NO_ERROR,
  FILE_NOT_FOUND,
  FILE_STAT_NOT_FOUND,
  FAILED_TO_ALLOCATE_FILE,
  ERROR_ON_READ,
  ERROR_ON_CLOSE,
};

// TODO put these into an array
#define FILE_IO_ERROR_MSG_1 "File not found."
#define FILE_IO_ERROR_MSG_2 "File stat not found."
#define FILE_IO_ERROR_MSG_3 "Failed to allocate file."
#define FILE_IO_ERROR_MSG_4 "Error on read."
#define FILE_IO_ERROR_MSG_5 "Error on close."

struct File {
  char *filename;
  uint8_t *buffer;
  PushAllocator error_stream;
  u32 error_count;
  int descriptor;
  u32 size;
  u32 loaded_size;
  int last_error;
  bool assert_on_error;
};

static void print_io_error(int error, const char *filename = NULL) {
  if (!error) return;
  char *error_message;

  switch (error) {
    case FILE_NOT_FOUND          : error_message = FILE_IO_ERROR_MSG_1; break;
    case FILE_STAT_NOT_FOUND     : error_message = FILE_IO_ERROR_MSG_2; break;
    case FAILED_TO_ALLOCATE_FILE : error_message = FILE_IO_ERROR_MSG_3; break;
    case ERROR_ON_READ           : error_message = FILE_IO_ERROR_MSG_4; break;
    case ERROR_ON_CLOSE          : error_message = FILE_IO_ERROR_MSG_5; break;
    default : error_message = "Invalid error value."; break;
  }

  if (filename) {
    printf("IO error in %s : %s\n", filename, error_message);
  } else {
    printf("IO error in file : %s\n", error_message);
  }
}

inline void push_string(PushAllocator *allocator, char *str, uint32_t len) {
  char *error_str = ALLOC_ARRAY(allocator, char, len);
  mem_copy(str, error_str, len);
}

inline void log_error(File *file, File_IO_Error error) {
  assert(file);
  if (!error) return;
  file->error_count++;
  if (file->assert_on_error) {
    print_io_error(error, file->filename);
    assert(!"Panic on error."); // TODO something better here?
  }
  if (is_initialized(&file->error_stream)) {
    // TODO should push_string instead
    print_io_error(error, file->filename);
  }
}

// TODO allow user to specify read/write
static File open_file(char *filename, PushAllocator error_stream = {}) {
  File result = {};
  int fd = open(filename, O_RDONLY);
  result.filename = filename;
  result.error_stream = error_stream;
  result.descriptor = fd;
  if (!is_initialized(&result.error_stream)) result.assert_on_error = true; // TODO allow this to not happen?

  if (fd < 0) {
    log_error(&result, FILE_NOT_FOUND);
    return result;
  }

  struct stat stat_buf;
  int stat_error = fstat(fd, &stat_buf);
  if (stat_error < 0) { 
    log_error(&result, FILE_STAT_NOT_FOUND);
    return result;
  }
  result.size = stat_buf.st_size;
  return result;
}

// TODO finish this
static void close_file(File *file) {
  int error = close(file->descriptor);
  if (error) log_error(file, ERROR_ON_CLOSE);
}

static int read_entire_file(File *file, PushAllocator *allocator, int alignment = 8) {
  assert(file);
  int fd = file->descriptor;

  if (fd < 0) { 
    log_error(file, FILE_NOT_FOUND); 
    return FILE_NOT_FOUND; 
  }

  auto size = file->size; 
  if (!size) {
    log_error(file, FILE_STAT_NOT_FOUND); 
    return FILE_STAT_NOT_FOUND; 
  }

  auto buffer = alloc_size(allocator, size, alignment);
  if (!buffer) {
    log_error(file, FAILED_TO_ALLOCATE_FILE);
    return FAILED_TO_ALLOCATE_FILE;
  }
  file->buffer = (uint8_t *)buffer;
  file->size = size;
  file->loaded_size = size;

  size_t bytes_read = 0;
  while (bytes_read < size) {
    auto read_result = read(fd, buffer, size - bytes_read);
    if (read_result <= 0) {
      log_error(file, ERROR_ON_READ);
      file->loaded_size = bytes_read;
      // NOTE : The memory is lost here, so only use temporary allocators 
      // to store file data from this function
      return ERROR_ON_READ;
    }
    bytes_read += read_result;
  }
  return 0;
}

static uint8_t *read_entire_file(const char *filename, PushAllocator *allocator, int *error, int alignment = 8) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    if (error) *error = 1;
    else FAILURE(FILE_IO_ERROR_MSG_1, filename);
    return NULL;
  }

  struct stat stat_buf;
  int stat_error = fstat(fd, &stat_buf);
  if (stat_error < 0) {
    if (error) *error = 2;
    else FAILURE(FILE_IO_ERROR_MSG_2);
    return NULL;
  }

  auto size = stat_buf.st_size;
  auto buffer = alloc_size(allocator, size, alignment);
  if (!buffer) {
    if (error) *error = 3;
    else FAILURE(FILE_IO_ERROR_MSG_3);
    return NULL;
  }

  size_t bytes_read = 0;
  while (bytes_read < size) {
    auto read_result = read(fd, buffer, size - bytes_read);
    bytes_read += read_result;
    if (read_result <= 0) {
      if (error) *error = 4;
      else FAILURE(FILE_IO_ERROR_MSG_4);
      // NOTE : The memory is lost here, so only use temporary allocators 
      // to store file data from this function
      return NULL;
    }
  }

  ASSERT(close(fd) == 0);
  if (error) *error = 0;
  return (uint8_t *) buffer;
}

static uint8_t *read_entire_file(const char *filename, PushAllocator *allocator, int alignment = 8) {
  return read_entire_file(filename, allocator, NULL, alignment);
}

#endif //_UNIX_FILE_IO_H_

