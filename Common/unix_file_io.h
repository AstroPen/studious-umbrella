
#ifndef _UNIX_FILE_IO_H_
#define _UNIX_FILE_IO_H_

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FILE_IO_ERROR_MSG_1 "File not found."
#define FILE_IO_ERROR_MSG_2 "File stat not found."
#define FILE_IO_ERROR_MSG_3 "Failed to allocate file."
#define FILE_IO_ERROR_MSG_4 "Error on read."

// TODO move this out somewhere maybe? Or at least pre-define it so that it doesn't need to be inlined up here.
static uint8_t *read_entire_file(const char *filename, PushAllocator *allocator, int *error, int alignment = 8) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    if (error) *error = 1;
    else assert(!FILE_IO_ERROR_MSG_1);
    return NULL;
  }

  struct stat stat_buf;
  int stat_error = fstat(fd, &stat_buf);
  if (stat_error < 0) {
    if (error) *error = 2;
    else assert(!FILE_IO_ERROR_MSG_2);
    return NULL;
  }

  auto size = stat_buf.st_size;
  auto buffer = alloc_size(allocator, size, alignment);
  if (!buffer) {
    if (error) *error = 3;
    else assert(!FILE_IO_ERROR_MSG_3);
    return NULL;
  }

  size_t bytes_read = 0;
  while (bytes_read < size) {
    bytes_read += read(fd, buffer, size - bytes_read);
    if (bytes_read <= 0) {
      if (error) *error = 4;
      else assert(!FILE_IO_ERROR_MSG_4);
      // NOTE : The memory is lost here, so only use temporary allocators 
      // to store file data from this function
      return NULL;
    }
  }

  return (uint8_t *) buffer;
}

static uint8_t *read_entire_file(const char *filename, PushAllocator *allocator, int alignment = 8) {
  return read_entire_file(filename, allocator, NULL, alignment);
}

#endif //_UNIX_FILE_IO_H_

