
#include <common.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include "pixel.h"

struct PackedAssetHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t size; // ??
  uint32_t packed_asset_count; // ??
  uint32_t bitmap_count;
  uint32_t bitmap_offset;
};

void usage(char *program_name) {
  printf("Usage : %s <header filename>\n", program_name);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) usage(argv[0]);
  char *header_filename = argv[1];

  printf("Packing assets from %s\n", header_filename);

  auto allocator_ = new_push_allocator(2048 * 4);
  auto allocator = &allocator_;

  int error;
  uint8_t *header_file = read_entire_file(header_filename, allocator, &error);
  print_io_error(error, header_filename);

  // TODO read header for info about what files to read and what to do with them

  //printf("Packing successful.\n");
  return 0;
}

