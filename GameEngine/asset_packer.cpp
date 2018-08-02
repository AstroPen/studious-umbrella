
#include <common.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include <vector_math.h>
#include <lstring.h>
#include <dynamic_array.h>
#include "pixel.h"
#include "custom_stb.h"
#include "asset_interface.h"
#include "packed_assets.h"


static void print_error(char *msg, char *filename, u32 line_num) {
  printf("Error on line %u of %s : %s.\n", line_num, filename, msg);
  exit(1); // TODO should maybe close more cleanly
}

// TODO write a proper usage message
static void usage(char *program_name) {
  printf("Usage : %s\n", program_name);
  printf("Usage : %s <header filename>\n", program_name);
  exit(0);
}

enum CommandKeyword {
  COMMAND_VERSION,
  COMMAND_LAYOUT,
  COMMAND_ANIMATION,
  COMMAND_SET,
  COMMAND_BITMAP,

  COMMAND_COUNT,
  COMMAND_INVALID,
  COMMAND_NONE,
  COMMAND_COMMENT_BEGIN,
  COMMAND_COMMENT_END,
};

enum AttributeKeyword {
  ATTRIBUTE_LAYOUT,
  ATTRIBUTE_HAS_NORMAL_MAP,
  ATTRIBUTE_MIN_BLEND,
  ATTRIBUTE_MAX_BLEND,
  ATTRIBUTE_S_CLAMP,
  ATTRIBUTE_T_CLAMP,
  ATTRIBUTE_OFFSET,
  ATTRIBUTE_SPRITE_DEPTH,
  ATTRIBUTE_SPRITE_INDEX,

  ATTRIBUTE_COUNT,
  ATTRIBUTE_INVALID,
};

//
// StringTable functions ---
//

struct StringTable {
  hstring command_hstrings[COMMAND_COUNT];
  hstring attribute_hstrings[ATTRIBUTE_COUNT];
  hstring animation_type_hstrings[ANIM_COUNT];
  hstring layout_type_hstrings[LAYOUT_COUNT];
  hstring group_id_hstrings[TEXTURE_GROUP_COUNT];
  hstring blend_mode_hstrings[BLEND_COUNT];
  hstring clamp_mode_hstrings[CLAMP_COUNT];
  hstring direction_hstrings[DIRECTION_COUNT];
} string_table_;

StringTable * const string_table = &string_table_;

static void init_string_table() {

  {
#define ADD_COMMAND_HASH(name) \
    string_table->command_hstrings[COMMAND_##name] = hash_string(#name); i++;
    uint32_t i = 0;
    ADD_COMMAND_HASH(VERSION);
    ADD_COMMAND_HASH(LAYOUT);
    ADD_COMMAND_HASH(ANIMATION);
    ADD_COMMAND_HASH(BITMAP);
    ADD_COMMAND_HASH(SET);
    assert(i == COMMAND_COUNT);
#undef ADD_COMMAND_HASH
  }

  {
#define ADD_ATTRIBUTE_HASH(name) \
    string_table->attribute_hstrings[ATTRIBUTE_##name] = hash_string(#name); i++;
    uint32_t i = 0;
    ADD_ATTRIBUTE_HASH(LAYOUT);
    ADD_ATTRIBUTE_HASH(HAS_NORMAL_MAP);
    ADD_ATTRIBUTE_HASH(MIN_BLEND);
    ADD_ATTRIBUTE_HASH(MAX_BLEND);
    ADD_ATTRIBUTE_HASH(S_CLAMP);
    ADD_ATTRIBUTE_HASH(T_CLAMP);
    ADD_ATTRIBUTE_HASH(OFFSET);
    ADD_ATTRIBUTE_HASH(SPRITE_DEPTH);
    ADD_ATTRIBUTE_HASH(SPRITE_INDEX);
    assert(i == ATTRIBUTE_COUNT);
#undef ADD_ATTRIBUTE_HASH
  }

  {
#define ADD_ANIM_HASH(type) \
    string_table->animation_type_hstrings[ANIM_##type] = hash_string(#type); i++;
    uint32_t i = 0;
    ADD_ANIM_HASH(IDLE);
    ADD_ANIM_HASH(MOVE);
    ADD_ANIM_HASH(SLIDE);
    assert(i == ANIM_COUNT);
#undef ADD_ANIM_HASH
  }

  {
#define ADD_LAYOUT_HASH(type) \
    string_table->layout_type_hstrings[LAYOUT_##type] = hash_string(#type); i++;
    uint32_t i = 0;
    ADD_LAYOUT_HASH(CHARACTER);
    ADD_LAYOUT_HASH(TERRAIN);
    ADD_LAYOUT_HASH(SINGLETON);
    assert(i == LAYOUT_COUNT);
#undef ADD_LAYOUT_HASH
  }

  {
#define ADD_GROUP_HASH(type) \
    string_table->group_id_hstrings[TEXTURE_GROUP_##type] = hash_string(#type); i++;
    uint32_t i = 1; // NOTE : Texture groups start at 1
    ADD_GROUP_HASH(LINK);
    ADD_GROUP_HASH(WALL);
    assert(i == TEXTURE_GROUP_COUNT);
#undef ADD_GROUP_HASH
  }

  {
#define ADD_BLEND_HASH(type) \
    string_table->blend_mode_hstrings[type##_BLEND - BLEND_FIRST] \
        = hash_string(#type); i++;
    uint32_t i = 0;
    ADD_BLEND_HASH(LINEAR);
    ADD_BLEND_HASH(NEAREST);
    assert(i == BLEND_COUNT);
#undef ADD_BLEND_HASH
  }

  {
#define ADD_CLAMP_HASH(type) \
    string_table->clamp_mode_hstrings[type - CLAMP_FIRST] = hash_string(#type); i++;
    uint32_t i = 0;
    ADD_CLAMP_HASH(CLAMP_TO_EDGE);
    ADD_CLAMP_HASH(REPEAT_CLAMPING);
    assert(i == CLAMP_COUNT);
#undef ADD_CLAMP_HASH
  }

  {
#define ADD_DIRECTION_HASH(dir) \
    string_table->direction_hstrings[dir] = hash_string(#dir); i++;
    uint32_t i = 0;
    ADD_DIRECTION_HASH(UP);
    ADD_DIRECTION_HASH(DOWN);
    ADD_DIRECTION_HASH(LEFT);
    ADD_DIRECTION_HASH(RIGHT);
    assert(i == DIRECTION_COUNT);
#undef ADD_DIRECTION_HASH
  }

#if 0
  for (uint32_t i = 0; i < ANIM_COUNT; i++) {
    print_hstring(string_table->animation_type_hstrings[i]);
  }

  for (uint32_t i = 0; i < LAYOUT_COUNT; i++) {
    print_hstring(string_table->layout_type_hstrings[i]);
  }

  for (uint32_t i = 1; i < TEXTURE_GROUP_COUNT; i++) {
    print_hstring(string_table->group_id_hstrings[i]);
  }

  for (uint32_t i = 0; i < BLEND_COUNT; i++) {
    print_hstring(string_table->blend_mode_hstrings[i]);
  }

  for (uint32_t i = 0; i < CLAMP_COUNT; i++) {
    print_hstring(string_table->clamp_mode_hstrings[i]);
  }

  printf("\n");
#endif
}

#define MAKE_LOOKUP_FUNCTION(Type, array, PREFIX, start, offset) \
  static Type array##_lookup(StringTable *table, hstring str) { \
    for (u32 i = start; i < PREFIX##_COUNT; i++) { \
      if (str_equal(string_table-> array##_hstrings[i], str)) \
        return Type(i + offset); \
    } \
    return PREFIX##_INVALID; \
  }

MAKE_LOOKUP_FUNCTION(CommandKeyword, command, COMMAND, 0, 0);
MAKE_LOOKUP_FUNCTION(AttributeKeyword, attribute, ATTRIBUTE, 0, 0);
MAKE_LOOKUP_FUNCTION(TextureLayoutType, layout_type, LAYOUT, 0, 0);
MAKE_LOOKUP_FUNCTION(AnimationType, animation_type, ANIM, 0, 0);
MAKE_LOOKUP_FUNCTION(TextureGroupID, group_id, TEXTURE_GROUP, 1, 0);
MAKE_LOOKUP_FUNCTION(TextureFormatSpecifier, blend_mode, BLEND, 0, BLEND_FIRST);
MAKE_LOOKUP_FUNCTION(TextureFormatSpecifier, clamp_mode, CLAMP, 0, CLAMP_FIRST);
MAKE_LOOKUP_FUNCTION(Direction, direction, DIRECTION, 0, 0);

//
// Token functions ---
//

enum TokenType {
  TOKEN_ERROR,
  TOKEN_HSTRING,
  TOKEN_LSTRING,
  TOKEN_INT,
  TOKEN_FLOAT,
  TOKEN_COMMAND,
  TOKEN_ATTRIBUTE,
};

enum TokenError : u32 {
  ERROR_NOT_SET,

  EMPTY_STRING,
  MISSING_END_QUOTE,
  UNEXPECTED_ADDITIONAL_ARGUMENTS,

  // NUMBERS :
  NON_NUMERIC_CHARACTER,
  UNEXPECTED_FLOATING_POINT,
  INTEGER_TOO_LARGE, // TODO Rename overflows to match?
  FLOAT_TOO_LARGE,
  OVERFLOW_16_BIT_INTEGER,
  SINGLETON_MINUS,

  // KEYWORDS :
  INVALID_ATTRIBUTE_NAME,
  INVALID_COMMAND_NAME,
  INVALID_LAYOUT_TYPE,
  INVALID_ANIMATION_TYPE,
  INVALID_GROUP_ID,
  INVALID_BLEND_MODE,
  INVALID_CLAMP_MODE,
  INVALID_DIRECTION,
  ANIMATION_BEFORE_LAYOUT_SPECIFIED,
  BITMAP_BEFORE_LAYOUT_SPECIFIED,
  COMMENT_NEVER_ENDED,
};

static char *to_string(TokenError error) {
  switch (error) {
    case EMPTY_STRING : return "Expected additonal token";
    case MISSING_END_QUOTE : return "Missing end quote on quoted string";
    case UNEXPECTED_ADDITIONAL_ARGUMENTS : return "Received unexpected additional arguments";
    case NON_NUMERIC_CHARACTER : return "Received unexpected non-numeric character";
    case UNEXPECTED_FLOATING_POINT : return "Received unexpected decimal point";
    case INTEGER_TOO_LARGE : return "Integer argument was too large";
    case FLOAT_TOO_LARGE : return "Floating point argument was too large";
    case OVERFLOW_16_BIT_INTEGER : return "16 bit integer argument was too large";
    case SINGLETON_MINUS : return "Received a minus sign with no number following it";
    case INVALID_ATTRIBUTE_NAME : return "Received an unrecognized attribute name";
    case INVALID_COMMAND_NAME : return "Received an invalid command name";
    case INVALID_LAYOUT_TYPE : return "Received an invalid layout type";
    case INVALID_ANIMATION_TYPE : return "Received an invalid layout type";
    case INVALID_GROUP_ID : return "Received an invalid texture group id";
    case INVALID_BLEND_MODE : return "Received an invalid blend mode";
    case INVALID_CLAMP_MODE : return "Received an invalid clamp mode";
    case INVALID_DIRECTION : return "Received an direction name";
    case ANIMATION_BEFORE_LAYOUT_SPECIFIED : return "ANIMATION command cannot be called before LAYOUT";
    case BITMAP_BEFORE_LAYOUT_SPECIFIED : return "BITMAP command cannot be called before LAYOUT";
    case COMMENT_NEVER_ENDED : return "A comment block was opened but never closed";
    default : return "Unspecified error";
  }
}

static void print_error(TokenError error, char *filename, u32 line_num) {
  assert(error);
  auto msg = to_string(error);
  printf("Error parsing line %u of %s : %s.\n", line_num, filename, msg);
  exit(1); // TODO should maybe close more cleanly
}

#define MAKE_TOKEN_FUNCTIONS(name, type_name, POSTFIX) \
  inline Token set(type_name arg, lstring rem = {}) { \
    type = TOKEN_##POSTFIX; name = arg; remainder = rem; return *this; \
  } \
  inline explicit operator type_name() { \
    assert(!type || type == TOKEN_##POSTFIX); return name; \
  }

struct Token {
  // TODO consider making this private?
  union {
    hstring hstr;
    lstring lstr;
    int i;
    float f;
    CommandKeyword command;
    AttributeKeyword attribute;
    TokenError error;
  };
  lstring remainder;
  TokenType type;

  MAKE_TOKEN_FUNCTIONS(hstr, hstring, HSTRING);
  MAKE_TOKEN_FUNCTIONS(lstr, lstring, LSTRING);
  MAKE_TOKEN_FUNCTIONS(i, int, INT);
  MAKE_TOKEN_FUNCTIONS(f, float, FLOAT);
  MAKE_TOKEN_FUNCTIONS(command, CommandKeyword, COMMAND);
  MAKE_TOKEN_FUNCTIONS(attribute, AttributeKeyword, ATTRIBUTE);
  MAKE_TOKEN_FUNCTIONS(error, TokenError, ERROR);

  inline operator bool() { return (type != TOKEN_ERROR); }
};

static Token parse_string(lstring line) {
  uint32_t i;
  for (i = 0; i < line.len; i++) {
    if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n') break;
  }
  lstring str = {line.str, i};
  return Token().set(str, line + i);
}

static Token pop_token(lstring line) {
  line = remove_whitespace(line);
  if (!line) return Token().set(EMPTY_STRING);

  return parse_string(line);
}

static Token pop_quote(lstring line) {
  line = remove_whitespace(line);
  if (!line) return Token().set(EMPTY_STRING);

  if (line[0] == '\"') {
    line = line + 1;
    uint32_t i;
    for (i = 0; i < line.len; i++) {
      if (line[i] == '\"') {
        lstring str = {line.str, i};
        if (!str) return Token().set(EMPTY_STRING);
        lstring remainder = line + i + 1;
        assert(!remainder || remainder[0] != '\"');
        return Token().set(str, remainder);
      }
    }
    return Token().set(MISSING_END_QUOTE);
  }

  return parse_string(line);
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

// TODO I may eventually want this to work without requiring the length first
static Token parse_float(lstring line) {

  assert(line);
  if (line.len >= 149) return Token().set(FLOAT_TOO_LARGE);

  bool is_negative = false;
  if (line[0] == '-') {
    is_negative = true;
    line = remove_whitespace(line + 1);
  }

  if (!line) return Token().set(SINGLETON_MINUS);

  float result = 0;
  u32 i;
  for (i = 0; i < line.len; i++) {

    if (line[i] == '.') break;
    if (line[i] > '9' || line[i] < '0') return Token().set(NON_NUMERIC_CHARACTER);

    float digit = line[i] - '0';
    result *= 10; result += digit;
  }

  assert(i == line.len || line[i] == '.');

  float place = 10;
  for (i++; i < line.len; i++) {

    if (line[i] == '.') return Token().set(UNEXPECTED_FLOATING_POINT);
    if (line[i] > '9' || line[i] < '0') return Token().set(NON_NUMERIC_CHARACTER);

    float digit = line[i] - '0';
    result += digit / place;
    place *= 10;
  }


  if (is_negative) result = -result;
  printf("Float : %f\n", result); // TODO delete this
  return Token().set(result);
}

static Token pop_float(lstring line) {
  Token token = pop_token(line);
  if (!token) return token;

  Token result = parse_float(lstring(token));
  result.remainder = token.remainder;
  return result;
}

//
// Spec file parsing helpers ---
//

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
  AttributeKeyword attribute;
  union {
    bool has_normal_map;
    TextureLayoutType layout;
    TextureFormatSpecifier blend_mode;
    TextureFormatSpecifier clamp_mode;
    V3 offset;
    float sprite_depth;
    u16 sprite_index;
  };

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

#define HANDLE_TOKEN_ERROR(token) \
  if (!token) { \
    result.error = token.error; \
    return result; \
  }

#define HANDLE_EXTRA_ARGS() \
  if (remove_whitespace(args)) { \
    result.error = UNEXPECTED_ADDITIONAL_ARGUMENTS; \
    return result; \
  }

inline TextureGroupID invalid_of(TextureGroupID) { return TEXTURE_GROUP_INVALID; }
inline TextureLayoutType invalid_of(TextureLayoutType) { return LAYOUT_INVALID; }
inline AnimationType invalid_of(AnimationType) { return ANIM_INVALID; }
inline CommandKeyword invalid_of(CommandKeyword) { return COMMAND_INVALID; }
inline AttributeKeyword invalid_of(AttributeKeyword) { return ATTRIBUTE_INVALID; }
inline Direction invalid_of(Direction) { return DIRECTION_INVALID; }
inline TextureFormatSpecifier invalid_of(TextureFormatSpecifier spec) { 
  if (spec == BLEND_INVALID) return BLEND_INVALID;
  if (spec == CLAMP_INVALID) return CLAMP_INVALID;
  if (spec >= BLEND_FIRST && spec <= BLEND_LAST) return BLEND_INVALID;
  if (spec >= CLAMP_FIRST && spec <= CLAMP_LAST) return CLAMP_INVALID;
  assert(!"Unknown spec invalid");
  return BLEND_INVALID; 
}

inline TokenError error_of(TextureGroupID) { return INVALID_GROUP_ID; }
inline TokenError error_of(TextureLayoutType) { return INVALID_LAYOUT_TYPE; }
inline TokenError error_of(AnimationType) { return INVALID_ANIMATION_TYPE; }
inline TokenError error_of(CommandKeyword) { return INVALID_COMMAND_NAME; }
inline TokenError error_of(AttributeKeyword) { return INVALID_ATTRIBUTE_NAME; }
inline TokenError error_of(Direction) { return INVALID_DIRECTION; }
inline TokenError error_of(TextureFormatSpecifier spec) { 
  if (spec == BLEND_INVALID) return INVALID_BLEND_MODE;
  if (spec == CLAMP_INVALID) return INVALID_CLAMP_MODE;
  if (spec >= BLEND_FIRST && spec <= BLEND_LAST) return INVALID_BLEND_MODE;
  if (spec >= CLAMP_FIRST && spec <= CLAMP_LAST) return INVALID_CLAMP_MODE;
  assert(!"Unknown spec error");
  return INVALID_BLEND_MODE; 
}
#define IS_VALID(x) ((x) != invalid_of(x))
#define DO_LOOKUP(arg, type, token) \
  auto arg = type##_lookup(string_table, hstring(token)); \
  result.arg = arg; \
  if (!IS_VALID(arg)) { \
    result.error = error_of(arg); \
    return result; \
  }

#define HANDLE_ARG_INT(name) \
  Token name = pop_int(args); \
  args = name.remainder; \
  HANDLE_TOKEN_ERROR(name); \
  result.name = int(name);

#define HANDLE_ARG_UINT16(name) \
  Token name = pop_int(args); \
  args = name.remainder; \
  HANDLE_TOKEN_ERROR(name); \
  if (int(name) > UINT16_MAX) { \
    result.error = OVERFLOW_16_BIT_INTEGER; \
    return result; \
  } \
  result.name = int(name);

#define PARSE_ARG_FLOAT(name) \
  Token name = pop_float(args); \
  args = name.remainder; \
  HANDLE_TOKEN_ERROR(name);

#define HANDLE_ARG_FLOAT(name) \
  PARSE_ARG_FLOAT(name) \
  result.name = float(name);

#define HANDLE_ARG_LOOKUP(name, type) \
  Token name##_str = pop_hash(args); \
  args = name##_str.remainder; \
  HANDLE_TOKEN_ERROR(name##_str); \
  DO_LOOKUP(name, type, name##_str);

#define HANDLE_ARG_QUOTE(name) \
  Token name = pop_quote(args); \
  args = name.remainder; \
  HANDLE_TOKEN_ERROR(name); \
  result.name = lstring(name);

//
// Spec file parsing ---
//

static VersionArgs parse_version_args(lstring args) {
  VersionArgs result = {};
  HANDLE_ARG_INT(version_number);
  HANDLE_EXTRA_ARGS();
  return result;
}

static LayoutArgs parse_layout_args(lstring args) {
  LayoutArgs result = {};
  HANDLE_ARG_LOOKUP(type, layout_type);
  HANDLE_EXTRA_ARGS();
  return result;
}

static AnimationArgs parse_animation_args(lstring args) {
  AnimationArgs result = {};
  HANDLE_ARG_LOOKUP(type, animation_type);
  HANDLE_ARG_INT(frames);
  HANDLE_ARG_FLOAT(duration);
  HANDLE_ARG_LOOKUP(direction, direction);
  HANDLE_EXTRA_ARGS();
  return result;
}

static SetArgs parse_set_args(lstring args) {
  SetArgs result = {};
  HANDLE_ARG_LOOKUP(attribute, attribute);

  switch (attribute) {
    case ATTRIBUTE_LAYOUT : {
      HANDLE_ARG_LOOKUP(layout, layout_type);
    } break;

    case ATTRIBUTE_HAS_NORMAL_MAP : {
      HANDLE_ARG_INT(has_normal_map);
    } break;

    case ATTRIBUTE_MIN_BLEND : {
      HANDLE_ARG_LOOKUP(blend_mode, blend_mode);
    } break;

    // NOTE : Although there is currently no difference between MIN and MAX
    // blend modes, there will be once we add mip mapping.
    case ATTRIBUTE_MAX_BLEND : {
      HANDLE_ARG_LOOKUP(blend_mode, blend_mode);
    } break;

    case ATTRIBUTE_S_CLAMP :
    case ATTRIBUTE_T_CLAMP : {
      HANDLE_ARG_LOOKUP(clamp_mode, clamp_mode);
    } break;

    case ATTRIBUTE_OFFSET : {
      PARSE_ARG_FLOAT(offset_x);
      PARSE_ARG_FLOAT(offset_y);
      PARSE_ARG_FLOAT(offset_z);
      result.offset = vec3(float(offset_x), float(offset_y), float(offset_z));
      print_vector(result.offset);
    } break;

    case ATTRIBUTE_SPRITE_DEPTH : {
      HANDLE_ARG_FLOAT(sprite_depth);
    } break;

    case ATTRIBUTE_SPRITE_INDEX : {
      HANDLE_ARG_UINT16(sprite_index);
    } break;

    default : {
      assert(!"Unhandled attribute type error.");
    } break;
  }

  HANDLE_EXTRA_ARGS();

  return result;
}

static BitmapArgs parse_bitmap_args(lstring args) {
  BitmapArgs result = {};
  HANDLE_ARG_QUOTE(filename);
  HANDLE_ARG_LOOKUP(id, group_id);
  HANDLE_ARG_UINT16(sprite_count);
  HANDLE_ARG_UINT16(sprite_width);
  HANDLE_ARG_UINT16(sprite_height);
  HANDLE_EXTRA_ARGS();
  return result;
}

static Token parse_command(lstring line) {
  line = remove_whitespace(line);
  if (!line) return Token().set(COMMAND_NONE);
  if (line[0] == '#') {
    if (line.len >= 3) {
      if (line[1] == '#' && line[2] == '/') return Token().set(COMMAND_COMMENT_BEGIN);
    }
    return Token().set(COMMAND_NONE);
  }

  Token command = pop_hash(line);
  if (!command) return command;

  // TODO consider checking for COMMAND_INVALID here instead of later
  CommandKeyword command_num = command_lookup(string_table, hstring(command));
  return Token().set(command_num, command.remainder);
}

static Token parse_comment_end(lstring line) {
  line = remove_whitespace(line);
  if (!line) return Token().set(COMMAND_NONE);
  if (line.len < 3) return Token().set(COMMAND_NONE);
  if (line[0] == '/' && line[1] == '#' && line[2] == '#') return Token().set(COMMAND_COMMENT_END);
  return Token().set(COMMAND_NONE);
}

#define DIRECTORY_BUFFER_SIZE 512

static void write_pack_file(PushAllocator *header, darray<PixelBuffer> bitmaps) {
  // TODO I might want to make my own version of this, but this is fine for now
  FILE *pack_file = fopen("assets/packed_assets.pack", "wb");
  //if (!pack_file) printf("Failed to open pack file for writing"); // TODO handle error better
  assert(pack_file);
  auto written = fwrite(header->memory, 1, header->bytes_allocated, pack_file);
  assert(written == header->bytes_allocated); // TODO handle error
  for (u32 i = 0; i < count(bitmaps); i++) {
    auto bitmap = bitmaps + i;
    u64 bitmap_size = u64(bitmap->width) * bitmap->height * 4;
    written = fwrite(bitmap->buffer, 1, bitmap_size, pack_file);
    assert(written == bitmap_size); // TODO handle error
  }
  printf("Packed in %ld bytes\n", ftell(pack_file));
  fclose(pack_file);
}

int main(int argc, char *argv[]) {
  if (argc > 2) usage(argv[0]);
  // TODO I may actually want it to just read all the files ending in .spec
  char *build_filename = "assets/build.spec";
  if (argc == 2) build_filename = argv[1];

  printf("Packing assets from %s\n", build_filename);

#if 1
  init_string_table();
  char directory_buffer[DIRECTORY_BUFFER_SIZE] = "assets/";
  lstring directory_name = length_string(directory_buffer);

  auto allocator_ = new_push_allocator(2048 * 4);
  auto allocator = &allocator_;

  PackedAssetHeader packed_header = {};
  packed_header.magic = 'PACK';
  packed_header.version = 0;

  darray<PackedTextureLayout> packed_layouts;
  darray<PackedAnimation> packed_animations;
  darray<PackedTextureGroup> packed_groups;
  darray<PixelBuffer> loaded_bitmaps;

  // TODO read build file for info about what files to read and what to do with them
  File build_file_ = open_file(build_filename);
  File *build_file = &build_file_;

  int error = read_entire_file(build_file, allocator);
  assert(!error);
  close_file(build_file);

  auto build_stream_ = get_stream(build_file->buffer, build_file->size);
  auto build_stream = &build_stream_;

  u32 line_number = 0;

  //
  // Parse the first line, which must be the version number :
  //

  lstring version_ln = pop_line(build_stream);
  line_number++;

  Token command = parse_command(version_ln);
  if (!command) print_error(command.error, build_filename, line_number);
  if (CommandKeyword(command) != COMMAND_VERSION) {
    print_error("VERSION not specified", build_filename, line_number);
  }
  auto args = parse_version_args(command.remainder);
  if (args.error) print_error(args.error, build_filename, line_number);

  if (args.version_number != 0) {
    print_error("Only version number 0 is currently accepted", build_filename, line_number);
  }

  //
  // Set up attributes to their default values :
  //

  uint16_t current_animation_index = 0;
  TextureLayoutType current_layout_type = LAYOUT_INVALID;
  uint16_t current_group_flags = 0;
  uint8_t current_min_blend = LINEAR_BLEND;
  uint8_t current_max_blend = LINEAR_BLEND;
  uint8_t current_s_clamp = CLAMP_TO_EDGE;
  uint8_t current_t_clamp = CLAMP_TO_EDGE;
  V3 current_offset = vec3(0);
  float current_sprite_depth = 0;

  u64 current_data_offset = 0;

  while (has_remaining(build_stream)) {

    //
    // Parse each command, then its arguments, then push the result to a dynamic array
    //

    lstring next_line = pop_line(build_stream);
    line_number++;

    Token command = parse_command(next_line);
    if (!command) print_error(command.error, build_filename, line_number);

    CommandKeyword command_num = CommandKeyword(command);
    switch (command_num) {
      case COMMAND_NONE    : { // Comment or newline, do nothing
      } break;
      case COMMAND_COMMENT_BEGIN : {
        while (has_remaining(build_stream)) {
          lstring next_line = pop_line(build_stream);
          line_number++;

          Token comment_token = parse_comment_end(next_line);
          if (!comment_token) assert(!"Error during comment.");
          auto comment_command = CommandKeyword(comment_token);
          if (comment_command == COMMAND_NONE) continue;
          if (comment_command == COMMAND_COMMENT_END) break;
          assert(!"Invalid command from parse_comment_end.");
        }
      } break;

      case COMMAND_VERSION : {
        print_error("VERSION command repeated", build_filename, line_number);
      } break; 

      case COMMAND_LAYOUT  : {
        auto args = parse_layout_args(command.remainder);
        if (args.error) print_error(args.error, build_filename, line_number);
        auto layout = push(packed_layouts);
        layout->layout_type = args.type;
        layout->animation_count = 0;
      } break; 

      case COMMAND_ANIMATION : {
        auto args = parse_animation_args(command.remainder);
        if (args.error) print_error(args.error, build_filename, line_number);
        auto layout = peek(packed_layouts);
        if (!layout) print_error(ANIMATION_BEFORE_LAYOUT_SPECIFIED, build_filename, line_number);
        auto animation = push(packed_animations);
        *animation = {};
        layout->animation_count++;
        animation->animation_type = args.type;
        animation->frame_count = args.frames;
        animation->duration = args.duration;
        animation->direction = args.direction;
        animation->start_index = current_animation_index;
        current_animation_index += args.frames;
      } break;

      case COMMAND_SET : {
        auto args = parse_set_args(command.remainder);
        if (args.error) print_error(args.error, build_filename, line_number);

        switch (args.attribute) {
          case ATTRIBUTE_LAYOUT : 
            current_layout_type = args.layout; break;
          case ATTRIBUTE_HAS_NORMAL_MAP :
            if (args.has_normal_map)
              current_group_flags |= GROUP_HAS_NORMAL_MAP;
            else
              current_group_flags &= ~GROUP_HAS_NORMAL_MAP; break;
          case ATTRIBUTE_MIN_BLEND : 
            current_min_blend = args.blend_mode; break;
          case ATTRIBUTE_MAX_BLEND :
            current_max_blend = args.blend_mode; break;
          case ATTRIBUTE_S_CLAMP :
            current_s_clamp = args.clamp_mode; break;
          case ATTRIBUTE_T_CLAMP :
            current_t_clamp = args.clamp_mode; break;
          case ATTRIBUTE_OFFSET :
            current_offset = args.offset; break;
          case ATTRIBUTE_SPRITE_DEPTH :
            current_sprite_depth = args.sprite_depth; break;
          case ATTRIBUTE_SPRITE_INDEX :
            current_animation_index = args.sprite_index; break;
          default : assert(!"Unknown attribute.");
        }
      } break;

      case COMMAND_BITMAP : {
        auto args = parse_bitmap_args(command.remainder);
        if (args.error) print_error(args.error, build_filename, line_number);
        if (current_layout_type == LAYOUT_INVALID)
          print_error(BITMAP_BEFORE_LAYOUT_SPECIFIED, build_filename, line_number);
        
        auto group = push(packed_groups);
        group->bitmap_offset = current_data_offset;
        group->texture_group_id = args.id;
        group->layout_type = current_layout_type;
        group->sprite_count = args.sprite_count;
        group->sprite_width = args.sprite_width;
        group->sprite_height = args.sprite_height;

        group->flags = current_group_flags;
        group->min_blend = current_min_blend;
        group->max_blend = current_max_blend;
        group->s_clamp = current_s_clamp;
        group->t_clamp = current_t_clamp;
        group->offset_x = current_offset.x;
        group->offset_y = current_offset.y;
        group->offset_z = current_offset.z;
        group->sprite_depth = current_sprite_depth;

        // TODO TODO Actually read the bitmap
        lstring bitmap_filename = append(directory_name, args.filename, DIRECTORY_BUFFER_SIZE - 1);
        zero_terminate(bitmap_filename);
        print_lstring(bitmap_filename); // TODO DELETE ME
        printf("%s\n", bitmap_filename.str);
        PixelBuffer bitmap = load_image_file(bitmap_filename.str);
        if (!is_initialized(&bitmap)) 
          print_error("Failed to load bitmap", build_filename, line_number);
        group->width = bitmap.width;
        group->height = bitmap.height;
        current_data_offset += bitmap.width * bitmap.height * 4;
        push(loaded_bitmaps, bitmap);

      } break;

      case COMMAND_INVALID : {
        print_error(INVALID_COMMAND_NAME, build_filename, line_number);
      } break;

      default : {
        print_lstring(next_line);
        assert(!"Invalid command from parse_command.");
      } break;
    }
  }

#if 1
  {
    u32 pre_data_size = sizeof(PackedAssetHeader);
    pre_data_size += count(packed_layouts) * sizeof(PackedTextureLayout);
    pre_data_size += count(packed_animations) * sizeof(PackedAnimation);
    pre_data_size += count(packed_groups) * sizeof(PackedTextureGroup);
    packed_header.data_offset = pre_data_size;
    packed_header.layout_count = count(packed_layouts);
    packed_header.texture_group_count = count(packed_groups);

    packed_header.total_size = pre_data_size + current_data_offset;

    PushAllocator dest_allocator_ = new_push_allocator(pre_data_size);
    auto dest_allocator = &dest_allocator_;

    PackedAssetHeader *asset_header = alloc_struct(dest_allocator, PackedAssetHeader);
    assert(asset_header);
    *asset_header = packed_header;

    u32 current_animation_index = 0;
    for (u32 i = 0; i < packed_header.layout_count; i++) {
      auto layout = alloc_struct(dest_allocator, PackedTextureLayout);
      assert(layout);
      *layout = packed_layouts[i];
      u32 animation_count = layout->animation_count;
      auto animations = alloc_array(dest_allocator, PackedAnimation, animation_count);
      assert(animations);
      assert(animations == layout->animations); // Check alignment
      assert(current_animation_index + animation_count <= count(packed_animations));
      array_copy(packed_animations + current_animation_index, animations, animation_count);
      current_animation_index += animation_count;
    }

    auto groups = alloc_array(dest_allocator, PackedTextureGroup, packed_header.texture_group_count);
    assert(groups);
    array_copy(packed_groups.p, groups, packed_header.texture_group_count);
    assert(dest_allocator->bytes_allocated == pre_data_size);

    write_pack_file(dest_allocator, loaded_bitmaps);
  }


#endif

#else

  PixelBuffer buffer = load_image_file("assets/lttp_link.png");
  assert(is_initialized(&buffer));

  // TODO replace all this with values from the spec file
  
  uint32_t layout_count = 1; // LAYOUT_COUNT
  uint32_t animation_count = 1; // ANIM_COUNT
  uint32_t group_count = 1; // TEXTURE_GROUP_COUNT
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
#endif


  printf("Packing successful.\n");
  return 0;
}

