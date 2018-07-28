
#include <common.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include <vector_math.h>
#include "pixel.h"
#include "custom_stb.h"
#include "asset_interface.h"
#include "packed_assets.h"


static void print_error(char *msg, char *filename, u32 line_num) {
  printf("Error on line %u of %s : %s.\n", line_num, filename, msg);
  exit(1); // TODO should maybe close more cleanly
}

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

inline bool has_remaining(Stream *stream) {
  return stream->cursor < stream->data.len;
}

#if 0
static hstring pop_hline(Stream *stream) {
  assert(stream); assert(stream->data);
  lstring remainder = stream->data + stream->cursor;
  hstring result = hash_string(remainder, '\n');
  return result;
}
#endif

#define KEYWORD_LIST \
  ENUM_OR_STR( VERSION ), \
  ENUM_OR_STR( SET ), \
  ENUM_OR_STR( LAYOUT ), \
  ENUM_OR_STR( ANIMATION ), \
  ENUM_OR_STR( BITMAP ) \

#undef ENUM_OR_STR
#define ENUM_OR_STR(e) e

enum SpecKeyword {
  NONE,
  KEYWORD_LIST,
  
  UNHANDLED_COMMAND,
  UNHANDLED_ATTRIBUTE,
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

enum TokenType {
  TOKEN_ERROR,
  TOKEN_HSTRING,
  TOKEN_LSTRING,
  TOKEN_INT,
  TOKEN_FLOAT,
  TOKEN_KEYWORD,
};

enum TokenError : u32 {
  ERROR_NOT_SET,

  EMPTY_STRING,
  UNEXPECTED_ADDITIONAL_ARGUMENTS,

  // NUMBERS :
  NON_NUMERIC_CHARACTER,
  UNEXPECTED_FLOATING_POINT,
  INTEGER_TOO_LARGE,
  SINGLETON_MINUS,

  INVALID_ATTRIBUTE_NAME,
  INVALID_COMMAND_NAME,
};

static char *to_string(TokenError error) {
  switch (error) {
    case EMPTY_STRING : return "Expected additonal token";
    case UNEXPECTED_ADDITIONAL_ARGUMENTS : return "Received unexpected additional arguments";
    case NON_NUMERIC_CHARACTER : return "Received unexpected non-numeric character";
    case UNEXPECTED_FLOATING_POINT : return "Received unexpected floating point";
    case INTEGER_TOO_LARGE : return "Integer argument was too large";
    case SINGLETON_MINUS : return "Received a minus sign with no number following it";
    case INVALID_ATTRIBUTE_NAME : return "Received an unrecognized attribute name";
    case INVALID_COMMAND_NAME : return "Received an invalid command name";
    default : return "Unknown error";
  }
}

static void print_error(TokenError error, char *filename, u32 line_num) {
  assert(error);
  auto msg = to_string(error);
  printf("Error parsing line %u of %s : %s.\n", line_num, filename, msg);
  exit(1); // TODO should maybe close more cleanly
}


struct Token {
  // TODO consider making this private?
  union {
    hstring hstr;
    lstring lstr;
    int i;
    float f;
    SpecKeyword keyword;
    TokenError error;
  };
  lstring remainder;
  TokenType type;

  inline Token set(hstring arg, lstring rem = {}) { 
    type = TOKEN_HSTRING; hstr = arg; remainder = rem; return *this;
  }
  inline Token set(lstring arg, lstring rem = {}) { 
    type = TOKEN_LSTRING; lstr = arg; remainder = rem; return *this;
  }
  inline Token set(int arg, lstring rem = {}) { 
    type = TOKEN_INT; i = arg; remainder = rem; return *this;
  }
  inline Token set(float arg, lstring rem = {}) { 
    type = TOKEN_FLOAT; f = arg; remainder = rem; return *this;
  }
  inline Token set(SpecKeyword arg, lstring rem = {}) { 
    type = TOKEN_KEYWORD; keyword = arg; remainder = rem; return *this;
  }
  inline Token set(TokenError arg, lstring rem = {}) { 
    type = TOKEN_ERROR; error = arg; remainder = rem; return *this;
  }

  inline operator bool() { return (type != TOKEN_ERROR); }

  inline explicit operator hstring() { 
    assert(!type || type == TOKEN_HSTRING); return hstr; }
  inline explicit operator lstring() { 
    assert(!type || type == TOKEN_LSTRING); return lstr; }
  inline explicit operator int() { 
    assert(!type || type == TOKEN_INT); return i; }
  inline explicit operator float() { 
    assert(!type || type == TOKEN_FLOAT); return f; }
  inline explicit operator SpecKeyword() { 
    assert(!type || type == TOKEN_KEYWORD); return keyword; }
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
  lstring str = {line.str, i};
  Token result;

  if (str) result.set(str, line + i);
  else result.set(EMPTY_STRING, line + i);

  return result;
}

static Token pop_hash(lstring line) {
  // TODO make a hash_token that splits by all whitespace for speed
  Token token = pop_token(line);
  if (!token) return token;

  hstring hstr = hash_string(lstring(token));
  return Token().set(hstr, token.remainder);
}

// TODO I may eventually want this to work without requiring the length first
static Token parse_int(lstring line) {

  assert(line);
  if (line.len >= 10) return Token().set(INTEGER_TOO_LARGE);

  bool is_negative = false;
  if (line[0] == '-') {
    is_negative = true;
    line = remove_whitespace(line + 1);
  }

  if (!line) return Token().set(SINGLETON_MINUS);

  int result = 0;
  for (uint32_t i = 0; i < line.len; i++) {

    if (line[i] == '.') return Token().set(UNEXPECTED_FLOATING_POINT);
    if (line[i] > '9' || line[i] < '0') return Token().set(NON_NUMERIC_CHARACTER);

    int digit = line[i] - '0';
    result *= 10; result += digit;
  }

  if (is_negative) result = -result;
  return Token().set(result);
}

static Token pop_int(lstring line) {
  Token token = pop_token(line);
  if (!token) return token;

  Token result = parse_int(lstring(token));
  result.remainder = token.remainder;
  return result;
}

struct VersionArgs {
  uint32_t version_number;

  TokenError error;
};

// TODO I think the hstrings here should actually be interned for fast comparison
struct LayoutArgs {
  hstring name;
  TextureLayoutType type;

  TokenError error;
};

struct AnimationArgs {
  AnimationType type;
  uint16_t frames;
  float duration;
  Direction direction;

  TokenError error;
};

struct SetArgs {
  SpecKeyword attribute;
  hstring name;

  TokenError error;
};

// TODO I may want to allow people to set more than this...
struct BitmapArgs {
  lstring filename;
  TextureGroupID id;
  uint16_t sprite_count;
  uint16_t sprite_width;
  uint16_t sprite_height;

  TokenError error;
};

static VersionArgs parse_version_args(lstring args) {
  Token version_num = pop_int(args);

  VersionArgs result = {};
  if (!version_num) 
    result.error = version_num.error;

  else if (remove_whitespace(version_num.remainder)) 
    result.error = UNEXPECTED_ADDITIONAL_ARGUMENTS;

  else 
    result.version_number = int(version_num);

  return result;
}
static LayoutArgs parse_layout_args(lstring args) {
  Token name = pop_hash(args);

  LayoutArgs result = {};
  if (!name) {
    result.error = name.error;
    return result;
  }

  result.name = hstring(name);

  Token layout_type = pop_hash(name.remainder);
  if (!layout_type) {
    result.error = layout_type.error;
    return result;
  }

  if (remove_whitespace(layout_type.remainder)) {
    result.error = UNEXPECTED_ADDITIONAL_ARGUMENTS;
    return result;
  }

  //result.type = hstring(layout_type);

  //Token parsed_layout_type = layout_type_lookup(string_table, hstring(layout_type));

  return result;
}

// TODO this needs to return some sort of general result or something...
static Token parse_command(lstring line) {
  line = remove_whitespace(line);
  if (!line) return Token().set(NONE);
  if (line[0] == '#') return Token().set(NONE);

  Token keyword = pop_hash(line);
  if (!keyword) return keyword;

  SpecKeyword keyword_num = keyword_lookup(string_table, hstring(keyword));
  switch (keyword_num) {
    case NONE    : break;
    case VERSION : break; 
    case LAYOUT  : break; 

    default : {
      keyword_num = UNHANDLED_COMMAND;
    } break;
  }

  return Token().set(keyword_num, keyword.remainder);
}


int main(int argc, char *argv[]) {
  if (argc > 2) usage(argv[0]);
  // TODO I may actually want it to just read all the files ending in .spec
  char *build_filename = "assets/build.spec";
  if (argc == 2) build_filename = argv[1];

  printf("Packing assets from %s\n", build_filename);

#if 1
  init_string_table();

  auto allocator_ = new_push_allocator(2048 * 4);
  auto allocator = &allocator_;

  // TODO read build file for info about what files to read and what to do with them
  File build_file_ = open_file(build_filename);
  File *build_file = &build_file_;

  int error = read_entire_file(build_file, allocator);

  auto build_stream_ = get_stream(build_file->buffer, build_file->size);
  auto build_stream = &build_stream_;

  u32 line_number = 0;

  lstring version_ln = pop_line(build_stream);
  line_number++;

  Token command = parse_command(version_ln);
  if (!command) print_error(command.error, build_filename, line_number);
  if (SpecKeyword(command) != VERSION) {
    print_error("VERSION not specified", build_filename, line_number);
  }
  auto args = parse_version_args(command.remainder);
  if (args.error) print_error(args.error, build_filename, line_number);

  while (has_remaining(build_stream)) {
    lstring next_line = pop_line(build_stream);
    line_number++;

    Token command = parse_command(next_line);
    if (!command) print_error(command.error, build_filename, line_number);

    SpecKeyword command_num = SpecKeyword(command);
    switch (command_num) {
      case NONE    : { // Comment or newline, do nothing
      } break;
      case VERSION : {
        print_error("VERSION repeated", build_filename, line_number);
      } break; 
      case LAYOUT  : {
        auto args = parse_layout_args(command.remainder);
        if (args.error) print_error(args.error, build_filename, line_number);
      } break; 
      case UNHANDLED_COMMAND : {
        //print_error(INVALID_COMMAND_NAME, build_filename, line_number);
      } break;

      default : {
        print_lstring(next_line);
        assert(!"Invalid command from parse_command.");
      } break;
    }
  }

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

