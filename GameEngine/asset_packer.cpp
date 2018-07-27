
#include <common.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include <vector_math.h>
#include "pixel.h"
#include "custom_stb.h"
#include "asset_interface.h"
#include "packed_assets.h"


// For each layout type :
// - Get TextureLayoutType
// assert(layout_type >= 0 && layout_type < LAYOUT_COUNT);
// TextureLayout *layout = assets->texture_layout + layout_type;
// 
// For each animation :
// - Get AnimationType with frame_count and duration
// layout->animation_frame_counts[animation] = frame_count;
// layout->animation_time[animation] = duration;
//
// For each facing direction :
// - Get start_index
// layout->animation_start_index[animation] = start_index;
//

// For each texture group :
// - Get TextureGroupID
// TextureGroup *group = assets->texture_groups + group_id;
// group->bitmap;
// group->layout;
// group->sprite_width;
// group->sprite_height;
// group->sprite_depth;
// group->render_id; // TODO init in OpenGL
// group->sprite_count;
// group->sprite_offset;
// group->has_normal_map;
//

void usage(char *program_name) {
  printf("Usage : %s\n", program_name);
  printf("Usage : %s <header filename>\n", program_name);
  exit(0);
}

// TODO move string functions to Common
struct lstring {
  char *str;
  uint32_t len;

  inline char &operator[](uint32_t i) {
    assert(i < len);
    return str[i];
  }

  inline lstring operator+(uint32_t i) {
    return {str + i, len - i};
  }

  // NOTE : This is in place of bool, which acts as a number and messes with 
  // some of the other operators. Since there isn't much you can do with a void*, 
  // this is a good alternative.
  inline operator void*() {
    return (void *)(len > 0 && str);
  }
};

inline lstring length_string(char *cstr) {
  if (!cstr) return {};
  uint32_t len = 0;
  while (cstr[len]) {
    len++;
  }
  return {cstr, len};
}

struct hstring {
  union {
    struct {
      char *str;
      uint32_t len;
    };
    lstring lstr;
  };
  uint32_t hash;

  inline operator lstring() { return lstr; }
};

struct Stream {
  lstring data;
  uint32_t cursor;
};

static Stream get_stream(uint8_t *data, uint32_t len) {
  return {(char *)data, len, 0};
}

// djb2 by Dan Bernstein, found on stackoverflow
static hstring hash_string(uint8_t *str, uint32_t len, int end_char) {
  if (!str) return {};
  if (!len) return {};

  uint32_t hash = 5381; int c;

  uint32_t i;
  for (i = 0; i < len; i++) {
    c = str[i];
    if (c == end_char) return {(char *)str, i, hash};
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return {(char *)str, len, hash};
}

inline hstring hash_string(const char *str, uint32_t len, int end_char = -1) {
  return hash_string((uint8_t *) str, len, end_char);
}

inline hstring hash_string(const char *cstr, char end_char = '\0') {
  return hash_string(cstr, UINT_MAX, end_char);
}

inline hstring hash_string(lstring str) {
  return hash_string(str.str, str.len, -1);
}

inline hstring hash_string(lstring str, char end_char) {
  return hash_string(str.str, str.len, end_char);
}


static bool str_equal(lstring a, lstring b) {
  if (a.len != b.len) return false;
  if (a.str == b.str) return true;
  for (uint32_t i = 0; i < a.len; i++) {
    if (a.str[i] != b.str[i]) return false;
  }
  return true;
}

static bool str_equal(hstring a, hstring b) {
  if (a.hash != b.hash) return false;
  return str_equal(a.lstr, b.lstr);
  return true;
}

static lstring pop_line(Stream *stream) {
  assert(stream); assert(stream->data);
  lstring str = stream->data + stream->cursor;
  uint32_t len = 0;
  while (len < str.len && str[len] != '\n') len++;
  stream->cursor += len + 1;
  return {str.str, len};
}

static hstring pop_hline(Stream *stream) {
  assert(stream); assert(stream->data);
  lstring remainder = stream->data + stream->cursor;
  hstring result = hash_string(remainder, '\n');
  return result;
}

#define KEYWORD_LIST \
  ENUM_OR_STR( VERSION ), \
  ENUM_OR_STR( SET ), \
  ENUM_OR_STR( LAYOUT ), \
  ENUM_OR_STR( ANIMATION ), \
  ENUM_OR_STR( BITMAP ), \

#undef ENUM_OR_STR
#define ENUM_OR_STR(e) e

enum SpecKeyword {
  NONE,
  KEYWORD_LIST
};

#undef ENUM_OR_STR
#define ENUM_OR_STR(e) #e

const char *keyword_strings[] = {KEYWORD_LIST};
#undef ENUM_OR_STR

// TODO use a hash table
struct StringTable {
  hstring keyword_hstrings[count_of(keyword_strings)];
} string_table_;

StringTable * const string_table = &string_table_;

static void init_string_table() {
  uint32_t keyword_count = count_of(keyword_strings);
  for (uint32_t i = 0; i < keyword_count; i++) {
    string_table->keyword_hstrings[i] = hash_string(keyword_strings[i]);
  }
}

static void print_hstring(hstring str) {
  char tmp[256] = {};
  assert(str.len < 256);
  mem_copy(str.str, tmp, str.len);
  printf("hstring : '%s', len : %u, hash : %u\n", tmp, str.len, str.hash);
}

static void print_lstring(lstring str) {
  char tmp[256] = {};
  assert(str.len < 256);
  mem_copy(str.str, tmp, str.len);
  printf("lstring : '%s', len : %u\n", tmp, str.len);
}

static SpecKeyword keyword_lookup(StringTable *table, hstring str) {
  uint32_t keyword_count = count_of(keyword_strings);
  for (uint32_t i = 0; i < keyword_count; i++) {
    if (str_equal(string_table->keyword_hstrings[i], str)) return SpecKeyword(i + 1);
  }
  return NONE;
}


struct Token {
  union {
    hstring hstr;
    lstring lstr;
    int i;
    float f;
  };
  lstring remainder;
};

inline lstring remove_whitespace(lstring line) {
  uint32_t i;
  for (i = 0; i < line.len; i++) {
    if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n') continue;
    break;
  }
  return line + i;
}

static Token pop_token(lstring line) {
  line = remove_whitespace(line);

  uint32_t i;
  for (i = 0; i < line.len; i++) {
    if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n') break;
  }
  Token result;
  result.lstr = {line.str, i};
  result.remainder = line + i;
  return result;
}

static Token pop_hash(lstring line) {
  // TODO make a hash_token that splits by all whitespace for speed
  Token token = pop_token(line);
  hstring hstr = hash_string(token.lstr);
  lstring remainder = line + hstr.len;
  Token result;
  result.hstr = hstr; result.remainder = remainder;
  return result;
}

// TODO I may eventually want this to work without requiring the length first
static int parse_int(lstring line, int *error = NULL) {
#define PARSE_INT_ERROR(cond, msg) \
  if ((cond)) { if (error) *error = 1; else assert(!(msg)); return 0; }

  PARSE_INT_ERROR(!line, "Null string.");
  PARSE_INT_ERROR(line.len >= 10, "Integer is too large.");

  bool is_negative = false;
  if (line[0] == '-') {
    is_negative = true;
    line = remove_whitespace(line + 1);
  }

  PARSE_INT_ERROR(!line, "Singleton minus.");

  int result = 0;
  for (uint32_t i = 0; i < line.len; i++) {

    PARSE_INT_ERROR(line[i] > '9' || line[i] < '0', "Received non-numeric character.");

    int digit = line[i] - '0';
    result *= 10; result += digit;
  }

  if (is_negative) result = -result;
  return result;
#undef PARSE_INT_ERROR
}

static Token pop_int(lstring line) {
  Token token = pop_token(line);

  int i = parse_int(token.lstr);
  Token result;
  result.i = i; result.remainder = token.remainder;
  return result;
}

// TODO this needs to return some sort of general result or something...
static SpecKeyword parse_line(lstring line) {
  Token keyword = pop_hash(line);
  SpecKeyword keyword_num = keyword_lookup(string_table, keyword.hstr);
  switch (keyword_num) {
    case NONE : {
    } break;
    case VERSION : {
      Token version_num = pop_int(keyword.remainder);
      assert(version_num.i == 0);
    } break;
    
    default : {
      assert(!"Unhandled keyword.");
    } break;
  }
  return keyword_num;
}

int main(int argc, char *argv[]) {
  if (argc > 2) usage(argv[0]);
  // TODO I may actually want it to just read all the files ending in .spec
  char *build_filename = "assets/build.spec";
  if (argc == 2) build_filename = argv[1];

  printf("Packing assets from %s\n", build_filename);

#if 0
  init_string_table();

  auto allocator_ = new_push_allocator(2048 * 4);
  auto allocator = &allocator_;

  // TODO read build file for info about what files to read and what to do with them
  File build_file_ = open_file(build_filename);
  File *build_file = &build_file_;

  int error = read_entire_file(build_file, allocator);

  auto build_stream_ = get_stream(build_file->buffer, build_file->size);
  auto build_stream = &build_stream_;

  lstring version_ln = pop_line(build_stream);
  assert(parse_line(version_ln) == VERSION);

  close_file(build_file);
#endif

  // TODO - load_png on sprite sheet
  //      - hard code the dimensions and other stats to put in Packed structs
  //      - write structs to pack_file
  PixelBuffer buffer = load_image_file("assets/lttp_link.png");
  assert(is_initialized(&buffer));
  /*
  struct PackedTextureLayout {
    uint32_t layout_type;
    uint32_t animation_count;
    PackedAnimation animations[0];
  };
  struct PackedAnimation {
    uint16_t animation_type;
    uint16_t frame_count;
    float duration;
    uint16_t animation_start_index[4]; // DIRECTION_COUNT
  };
  struct PackedTextureGroup {
    uint64_t bitmap_offset; // In bytes from start of data
    uint32_t width; // In pixels
    uint32_t height; 
    uint16_t texture_group_id;
    int16_t sprite_count; // Negative value indicates it has a normal map
    uint16_t sprite_width; // In pixels
    uint16_t sprite_height;
    float offset_x;
    float offset_y;
    float offset_z;
    float sprite_depth; // TODO I'd like to be able to specify this more generally
  };
  struct PackedAssetHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t total_size; // ??
    uint32_t layout_count;
    uint32_t texture_group_count;
    uint32_t data_offset;
  };
  */
  uint32_t layout_count = 1;
  uint32_t animation_count = 1;
  uint32_t group_count = 1;
  uint64_t pre_data_size = 
    sizeof(PackedAssetHeader) + 
    sizeof(PackedTextureLayout) * layout_count + 
    sizeof(PackedAnimation) * animation_count +
    sizeof(PackedTextureGroup) * group_count;

  // TODO for convenience, I should make a dynamic push allocator
  PushAllocator dest_allocator_ = new_push_allocator(pre_data_size);
  auto dest_allocator = &dest_allocator_;

  PackedAssetHeader *asset_header = alloc_struct(dest_allocator, PackedAssetHeader);
  asset_header->magic = 'PACK';
  asset_header->version = 0;
  asset_header->total_size = pre_data_size;
  asset_header->layout_count = 1;
  asset_header->texture_group_count = 1;
  asset_header->data_offset = pre_data_size;

  PackedTextureLayout *character_layout = alloc_struct(dest_allocator, PackedTextureLayout);
  character_layout->layout_type = LAYOUT_CHARACTER;
  character_layout->animation_count = 1;

  PackedAnimation *anim0 = alloc_struct(dest_allocator, PackedAnimation);
  anim0->animation_type = ANIM_IDLE;
  anim0->frame_count = 1;
  anim0->duration = 1;
  //anim0->animation_start_index = {}; // All zeroes for now

  PackedTextureGroup *link_group = alloc_struct(dest_allocator, PackedTextureGroup);
  link_group->width = buffer.width;
  link_group->height = buffer.height;
  link_group->texture_group_id = TEXTURE_GROUP_LINK;
  link_group->layout_type = LAYOUT_CHARACTER;
  link_group->sprite_count = 1;
  link_group->sprite_width = buffer.width; // TODO needs to be per sprite
  link_group->sprite_height = buffer.height;
  link_group->flags = 0; // No normal map for now
  link_group->min_blend = LINEAR_BLEND;
  link_group->max_blend = NEAREST_BLEND;
  link_group->s_clamp = CLAMP_TO_EDGE;
  link_group->t_clamp = CLAMP_TO_EDGE;
  link_group->offset_x = 0; // Copied from game.cpp
  link_group->offset_y = 0.40;
  link_group->offset_z = 0.06;
  link_group->sprite_depth = 0.3;
  link_group->bitmap_offset = 0; //dest_allocator->bytes_allocated; // TODO maybe consider alignment
  
  uint64_t bitmap_size = uint64_t(buffer.width) * buffer.height * 4;
  asset_header->total_size += bitmap_size;


  // TODO I might want to make my own version of this, but this is fine for now
  FILE *pack_file = fopen("assets/packed_assets.pack", "wb");
  assert(pack_file);
  auto written = fwrite(dest_allocator->memory, 1, dest_allocator->bytes_allocated, pack_file);
  assert(written == dest_allocator->bytes_allocated);
  written = fwrite(buffer.buffer, 1, bitmap_size, pack_file);
  assert(written == bitmap_size);
  printf("Packed in %ld bytes\n", ftell(pack_file));
  printf("Calculated size is %u bytes\n", asset_header->total_size);
  fclose(pack_file);


  printf("Packing successful.\n");
  return 0;
}

