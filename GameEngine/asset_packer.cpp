
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

struct SpecAttributes {
  u16 animation_index = 0;
  TextureLayoutType layout_type = LAYOUT_INVALID;
  u16 group_flags = 0;
  u8 min_blend = LINEAR_BLEND;
  u8 max_blend = LINEAR_BLEND;
  u8 s_clamp = CLAMP_TO_EDGE;
  u8 t_clamp = CLAMP_TO_EDGE;
  V3 offset = vec3(0);
  float sprite_depth = 0;
  char character_range_start = ' ';
  char character_range_end = '~';
} attributes;

struct PackerState {
  PushAllocator spec_allocator[1];

  // Packed data :
  PackedAssetHeader header;
  darray<PackedTextureLayout> layouts;
  darray<PackedAnimation> animations;
  darray<PackedFaces> faces;
  darray<PackedTextureGroup> groups;
  darray<PackedFontType> font_types;
  darray<PackedFont> fonts;
  darray<lstring> font_filenames;
  darray<lstring> bitmap_filenames;
  darray<BakedChar> baked_chars;
  u32 total_char_count;

  u64 current_data_offset = 0;
  File spec_file[1];
  Stream spec_stream[1];
  u32 line_number;
} packer;

// TODO rename to print_parse_error or something
static void print_error(char *msg, lstring line = {}) {
  printf("Error on line %u of %s : %s.\n", packer.line_number, packer.spec_file->filename, msg);
  if (line) {
    STACK_STRING(buf, line);
    printf(KRED "%u    %s\n\n" KNRM, packer.line_number, buf);
  }
  exit(1); // TODO should maybe close more cleanly
}

static void print_load_bitmap_error(char *filename) {
  printf("Error loading bitmap %s\n", filename);
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
  COMMAND_ANIMATION_LAYOUT,
  COMMAND_CUBE_LAYOUT,
  COMMAND_ANIMATION,
  COMMAND_FACE,
  COMMAND_SET,
  COMMAND_BITMAP,
  COMMAND_FONT,
  COMMAND_SIZE,

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
  ATTRIBUTE_CHARACTER_RANGE,

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
  hstring face_index_hstrings[FACE_COUNT];
  hstring font_id_hstrings[FONT_COUNT];
} string_table_;

StringTable * const string_table = &string_table_;

static void init_string_table() {

  {
#define ADD_COMMAND_HASH(name) \
    string_table->command_hstrings[COMMAND_##name] = hash_string(#name); i++;
    u32 i = 0;
    ADD_COMMAND_HASH(VERSION);
    ADD_COMMAND_HASH(ANIMATION_LAYOUT);
    ADD_COMMAND_HASH(ANIMATION);
    ADD_COMMAND_HASH(CUBE_LAYOUT);
    ADD_COMMAND_HASH(FACE);
    ADD_COMMAND_HASH(BITMAP);
    ADD_COMMAND_HASH(SET);
    ADD_COMMAND_HASH(FONT);
    ADD_COMMAND_HASH(SIZE);
    ASSERT_EQUAL(i, COMMAND_COUNT);
#undef ADD_COMMAND_HASH
  }

  {
#define ADD_ATTRIBUTE_HASH(name) \
    string_table->attribute_hstrings[ATTRIBUTE_##name] = hash_string(#name); i++;
    u32 i = 0;
    ADD_ATTRIBUTE_HASH(LAYOUT);
    ADD_ATTRIBUTE_HASH(HAS_NORMAL_MAP);
    ADD_ATTRIBUTE_HASH(MIN_BLEND);
    ADD_ATTRIBUTE_HASH(MAX_BLEND);
    ADD_ATTRIBUTE_HASH(S_CLAMP);
    ADD_ATTRIBUTE_HASH(T_CLAMP);
    ADD_ATTRIBUTE_HASH(OFFSET);
    ADD_ATTRIBUTE_HASH(SPRITE_DEPTH);
    ADD_ATTRIBUTE_HASH(SPRITE_INDEX);
    ADD_ATTRIBUTE_HASH(CHARACTER_RANGE);
    ASSERT_EQUAL(i, ATTRIBUTE_COUNT);
#undef ADD_ATTRIBUTE_HASH
  }

  {
#define ADD_ANIM_HASH(type) \
    string_table->animation_type_hstrings[ANIM_##type] = hash_string(#type); i++;
    u32 i = 0;
    ADD_ANIM_HASH(IDLE);
    ADD_ANIM_HASH(MOVE);
    ADD_ANIM_HASH(SLIDE);
    ASSERT_EQUAL(i, ANIM_COUNT);
#undef ADD_ANIM_HASH
  }

  {
#define ADD_LAYOUT_HASH(type) \
    string_table->layout_type_hstrings[LAYOUT_##type] = hash_string(#type); i++;
    u32 i = 0;
    ADD_LAYOUT_HASH(CHARACTER);
    ADD_LAYOUT_HASH(TERRAIN);
    ADD_LAYOUT_HASH(SINGLETON);
    ASSERT_EQUAL(i, LAYOUT_COUNT);
#undef ADD_LAYOUT_HASH
  }

  {
#define ADD_GROUP_HASH(type) \
    string_table->group_id_hstrings[TEXTURE_GROUP_##type] = hash_string(#type); i++;
    u32 i = 1; // NOTE : Texture groups start at 1
    ADD_GROUP_HASH(LINK);
    ADD_GROUP_HASH(WALL);
    ASSERT_EQUAL(i, TEXTURE_GROUP_COUNT);
#undef ADD_GROUP_HASH
  }

  {
#define ADD_BLEND_HASH(type) \
    string_table->blend_mode_hstrings[type##_BLEND - BLEND_FIRST] \
        = hash_string(#type); i++;
    u32 i = 0;
    ADD_BLEND_HASH(LINEAR);
    ADD_BLEND_HASH(NEAREST);
    ASSERT_EQUAL(i, BLEND_COUNT);
#undef ADD_BLEND_HASH
  }

  {
#define ADD_CLAMP_HASH(type) \
    string_table->clamp_mode_hstrings[type - CLAMP_FIRST] = hash_string(#type); i++;
    u32 i = 0;
    ADD_CLAMP_HASH(CLAMP_TO_EDGE);
    ADD_CLAMP_HASH(REPEAT_CLAMPING);
    ASSERT_EQUAL(i, CLAMP_COUNT);
#undef ADD_CLAMP_HASH
  }

  {
#define ADD_DIRECTION_HASH(dir) \
    string_table->direction_hstrings[dir] = hash_string(#dir); i++;
    u32 i = 0;
    ADD_DIRECTION_HASH(UP);
    ADD_DIRECTION_HASH(DOWN);
    ADD_DIRECTION_HASH(LEFT);
    ADD_DIRECTION_HASH(RIGHT);
    ASSERT_EQUAL(i, DIRECTION_COUNT);
#undef ADD_DIRECTION_HASH
  }

  {
#define ADD_FACE_HASH(face) \
    string_table->face_index_hstrings[FACE_##face] = hash_string(#face); i++;
    u32 i = 0;
    ADD_FACE_HASH(TOP);
    ADD_FACE_HASH(BOTTOM);
    ADD_FACE_HASH(RIGHT);
    ADD_FACE_HASH(LEFT);
    ADD_FACE_HASH(FRONT);
    ADD_FACE_HASH(BACK);
    ASSERT_EQUAL(i, FACE_COUNT);
#undef ADD_FACE_HASH
  }

  {
#define ADD_FONT_HASH(type) \
    string_table->font_id_hstrings[FONT_##type] = hash_string(#type); i++;
    u32 i = 1;
    ADD_FONT_HASH(COURIER_NEW_BOLD);
    ADD_FONT_HASH(ARIAL);
    ADD_FONT_HASH(DEBUG);
    ASSERT_EQUAL(i, FONT_COUNT);
#undef ADD_ANIM_HASH
  }

#if 0
  for (u32 i = 0; i < ANIM_COUNT; i++) {
    print_hstring(string_table->animation_type_hstrings[i]);
  }

  for (u32 i = 0; i < LAYOUT_COUNT; i++) {
    print_hstring(string_table->layout_type_hstrings[i]);
  }

  for (u32 i = 1; i < TEXTURE_GROUP_COUNT; i++) {
    print_hstring(string_table->group_id_hstrings[i]);
  }

  for (u32 i = 0; i < BLEND_COUNT; i++) {
    print_hstring(string_table->blend_mode_hstrings[i]);
  }

  for (u32 i = 0; i < CLAMP_COUNT; i++) {
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
MAKE_LOOKUP_FUNCTION(FaceIndex, face_index, FACE, 0, 0);
MAKE_LOOKUP_FUNCTION(FontID, font_id, FONT, 1, 0);

#undef MAKE_LOOKUP_FUNCTION

//
// Token functions ---
//

enum TokenType {
  TOKEN_ERROR,
  TOKEN_HSTRING,
  TOKEN_LSTRING,
  TOKEN_INT,
  TOKEN_FLOAT,
  TOKEN_CHAR,
  TOKEN_COMMAND,
  TOKEN_ATTRIBUTE,
};

enum TokenError : u32 {
  ERROR_NOT_SET,

  EMPTY_STRING,
  MISSING_END_QUOTE,
  MISSING_SINGLE_QUOTE,
  MISSING_END_SINGLE_QUOTE,
  EMPTY_SINGLE_QUOTE,
  UNEXPECTED_EXTRA_CHARACTER,
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
  INVALID_FACE_NAME,
  INVALID_FONT_ID,

  ANIMATION_BEFORE_LAYOUT_SPECIFIED,
  FACE_BEFORE_LAYOUT_SPECIFIED,
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
    case INVALID_DIRECTION : return "Received an invalid direction name";
    case INVALID_FACE_NAME : return "Received an invalid face name";
    case ANIMATION_BEFORE_LAYOUT_SPECIFIED : return "ANIMATION command cannot be called before ANIMATION_LAYOUT";
    case FACE_BEFORE_LAYOUT_SPECIFIED : return "FACE command cannot be called before CUBE_LAYOUT";
    case BITMAP_BEFORE_LAYOUT_SPECIFIED : return "BITMAP command cannot be called before LAYOUT";
    case COMMENT_NEVER_ENDED : return "A comment block was opened but never closed";
    default : return "Unspecified error";
  }
}

// TODO rename to print_parse_error or something
static void print_error(TokenError error, lstring line) {
  ASSERT(error);
  auto msg = to_string(error);
  printf("Error parsing line %u of %s : %s.\n", packer.line_number, packer.spec_file->filename, msg);
  STACK_STRING(buf, line);
  printf(KRED "%u    %s\n\n" KNRM, packer.line_number, buf);
  exit(1); // TODO should maybe close more cleanly
}

#define MAKE_TOKEN_FUNCTIONS(name, type_name, POSTFIX) \
  inline Token set(type_name arg, lstring rem = {}) { \
    type = TOKEN_##POSTFIX; name = arg; remainder = rem; return *this; \
  } \
  inline explicit operator type_name() { \
    ASSERT(!type || type == TOKEN_##POSTFIX); return name; \
  }

struct Token {
  // TODO consider making this private?
  union {
    hstring hstr;
    lstring lstr;
    int i;
    float f;
    char c;
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
  MAKE_TOKEN_FUNCTIONS(c, char, CHAR);
  MAKE_TOKEN_FUNCTIONS(command, CommandKeyword, COMMAND);
  MAKE_TOKEN_FUNCTIONS(attribute, AttributeKeyword, ATTRIBUTE);
  MAKE_TOKEN_FUNCTIONS(error, TokenError, ERROR);

  inline operator bool() { return (type != TOKEN_ERROR); }
};

#undef MAKE_TOKEN_FUNCTIONS

static Token parse_string(lstring line) {
  u32 i;
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
    u32 i;
    for (i = 0; i < line.len; i++) {
      if (line[i] == '\"') {
        lstring str = {line.str, i};
        if (!str) return Token().set(EMPTY_STRING);
        lstring remainder = line + i + 1;
        return Token().set(str, remainder);
      }
    }
    return Token().set(MISSING_END_QUOTE);
  }

  return parse_string(line);
}

static Token pop_char(lstring line) {
  line = remove_whitespace(line);
  if (!line) return Token().set(EMPTY_STRING);

  if (line[0] != '\'') return Token().set(MISSING_SINGLE_QUOTE);
  if (line.len < 2) return Token().set(MISSING_END_SINGLE_QUOTE);
  if (line[1] == '\'') return Token().set(EMPTY_SINGLE_QUOTE);
  if (line.len < 3) return Token().set(MISSING_END_SINGLE_QUOTE);
  if (line[2] != '\'') return Token().set(UNEXPECTED_EXTRA_CHARACTER);

  if (line[0] != '\'') return Token().set(MISSING_SINGLE_QUOTE);
  line = line + 1;
  u32 i;
  for (i = 0; i < line.len; i++) {
    if (line[i] == '\'') {
      lstring str = {line.str, i};
      if (!str) return Token().set(EMPTY_SINGLE_QUOTE);
      if (str.len > 1) return Token().set(UNEXPECTED_EXTRA_CHARACTER);
      lstring remainder = line + i + 1;
      return Token().set(str[0], remainder);
    }
  }
  return Token().set(MISSING_END_SINGLE_QUOTE);


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

  ASSERT(line);
  if (line.len >= 10) return Token().set(INTEGER_TOO_LARGE);

  bool is_negative = false;
  if (line[0] == '-') {
    is_negative = true;
    line = remove_whitespace(line + 1);
  }

  if (!line) return Token().set(SINGLETON_MINUS);

  int result = 0;
  for (u32 i = 0; i < line.len; i++) {

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

  ASSERT(line);
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

  ASSERT(i == line.len || line[i] == '.');

  float place = 10;
  for (i++; i < line.len; i++) {

    if (line[i] == '.') return Token().set(UNEXPECTED_FLOATING_POINT);
    if (line[i] > '9' || line[i] < '0') return Token().set(NON_NUMERIC_CHARACTER);

    float digit = line[i] - '0';
    result += digit / place;
    place *= 10;
  }


  if (is_negative) result = -result;
  //printf("Float : %f\n", result); // TODO delete this
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
  u32 version_number;

  TokenError error;
};

struct LayoutArgs {
  hstring name;
  TextureLayoutType type;

  TokenError error;
};

struct AnimationArgs {
  AnimationType type;
  u16 frames;
  float duration;
  Direction direction;

  TokenError error;
};

struct FaceArgs {
  FaceIndex face;
  u16 index;

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
    struct {
      char start_char;
      char end_char;
    };
  };

  TokenError error;
};

// TODO I want to reorder these so that id comes before filename.
// TODO Remove sprite_count probably.
struct BitmapArgs {
  lstring filename;
  TextureGroupID id;
  u16 sprite_count;
  u16 sprite_width;
  u16 sprite_height;

  TokenError error;
};

struct FontArgs {
  FontID id;
  lstring filename;

  TokenError error;
};

struct SizeArgs {
  u16 size;

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

#define MAKE_INVALID_ERROR_FUNCTIONS(Type, PREFIX, ERROR) \
  inline Type invalid_of(Type) { return PREFIX##_INVALID; } \
  inline TokenError error_of(Type) { return INVALID_##ERROR; }

MAKE_INVALID_ERROR_FUNCTIONS(TextureGroupID, TEXTURE_GROUP, GROUP_ID);
MAKE_INVALID_ERROR_FUNCTIONS(TextureLayoutType, LAYOUT, LAYOUT_TYPE);
MAKE_INVALID_ERROR_FUNCTIONS(AnimationType, ANIM, ANIMATION_TYPE);
MAKE_INVALID_ERROR_FUNCTIONS(CommandKeyword, COMMAND, COMMAND_NAME);
MAKE_INVALID_ERROR_FUNCTIONS(AttributeKeyword, ATTRIBUTE, ATTRIBUTE_NAME);
MAKE_INVALID_ERROR_FUNCTIONS(Direction, DIRECTION, DIRECTION);
MAKE_INVALID_ERROR_FUNCTIONS(FaceIndex, FACE, FACE_NAME);
MAKE_INVALID_ERROR_FUNCTIONS(FontID, FONT, FONT_ID);

inline TextureFormatSpecifier invalid_of(TextureFormatSpecifier spec) { 
  if (spec == BLEND_INVALID) return BLEND_INVALID;
  if (spec == CLAMP_INVALID) return CLAMP_INVALID;
  if (spec >= BLEND_FIRST && spec <= BLEND_LAST) return BLEND_INVALID;
  if (spec >= CLAMP_FIRST && spec <= CLAMP_LAST) return CLAMP_INVALID;
  FAILURE("Unknown spec invalid");
  return BLEND_INVALID; 
}

inline TokenError error_of(TextureFormatSpecifier spec) { 
  if (spec == BLEND_INVALID) return INVALID_BLEND_MODE;
  if (spec == CLAMP_INVALID) return INVALID_CLAMP_MODE;
  if (spec >= BLEND_FIRST && spec <= BLEND_LAST) return INVALID_BLEND_MODE;
  if (spec >= CLAMP_FIRST && spec <= CLAMP_LAST) return INVALID_CLAMP_MODE;
  FAILURE("Unknown spec error");
  return INVALID_BLEND_MODE; 
}

#undef MAKE_INVALID_ERROR_FUNCTIONS

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
  if (int(name) > UINT16_MAX || int(name) < 0) { \
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

#define HANDLE_ARG_CHAR(name) \
  Token name = pop_char(args); \
  args = name.remainder; \
  HANDLE_TOKEN_ERROR(name); \
  result.name = char(name);

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
  HANDLE_ARG_UINT16(frames);
  HANDLE_ARG_FLOAT(duration);
  HANDLE_ARG_LOOKUP(direction, direction);
  HANDLE_EXTRA_ARGS();
  return result;
}

static FaceArgs parse_face_args(lstring args) {
  FaceArgs result = {};
  HANDLE_ARG_LOOKUP(face, face_index);
  HANDLE_ARG_UINT16(index);
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
      //print_vector(result.offset);
    } break;

    case ATTRIBUTE_SPRITE_DEPTH : {
      HANDLE_ARG_FLOAT(sprite_depth);
    } break;

    case ATTRIBUTE_SPRITE_INDEX : {
      HANDLE_ARG_UINT16(sprite_index);
    } break;

    case ATTRIBUTE_CHARACTER_RANGE : {
      HANDLE_ARG_CHAR(start_char);
      HANDLE_ARG_CHAR(end_char);
    } break;

    default : {
      INVALID_SWITCH_CASE(attribute);
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

static FontArgs parse_font_args(lstring args) {
  FontArgs result = {};
  HANDLE_ARG_LOOKUP(id, font_id);
  HANDLE_ARG_QUOTE(filename);
  HANDLE_EXTRA_ARGS();
  return result;
}

static SizeArgs parse_size_args(lstring args) {
  SizeArgs result = {};
  HANDLE_ARG_UINT16(size);
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

// TODO add nested comments
static Token parse_comment_end(lstring line) {
  line = remove_whitespace(line);
  if (!line) return Token().set(COMMAND_NONE);
  if (line.len < 3) return Token().set(COMMAND_NONE);
  if (line[0] == '/' && line[1] == '#' && line[2] == '#') return Token().set(COMMAND_COMMENT_END);
  return Token().set(COMMAND_NONE);
}


#define DIRECTORY_BUFFER_SIZE 512


static void write_fonts(FILE *out) {

  char directory_buffer[DIRECTORY_BUFFER_SIZE] = "/Library/Fonts/";
  lstring directory_name = length_string(directory_buffer);
  darray<u8> buffer;
  PushAllocator file_allocator[1];
  *file_allocator = new_push_allocator(2048 * 512);

  ASSERT_EQUAL(count(packer.font_types), count(packer.font_filenames));


  for (u32 i = 0; i < count(packer.font_types); i++) {
    lstring filename = packer.font_filenames[i];
    lstring full_filename = append(directory_name, filename, DIRECTORY_BUFFER_SIZE - 1);
    zero_terminate(full_filename);
    printf("Reading %s\n", full_filename.str);

    u8 *file_buffer = read_entire_file(full_filename.str, file_allocator);
    ASSERT(file_buffer); // TODO handle error

    auto font_type = packer.font_types + i;

    for (u32 j = 0; j < font_type->size_count; j++) {

      ASSERT(j < count(packer.fonts));
      auto font = packer.fonts + j;

      int first_glyph = font->start;
      int num_glyphs = font->end - font->start;
      u32 font_height = font->font_size;
      u32 bitmap_dim = next_power_2(sqrt(num_glyphs) * font_height);

      int pixel_bytes = 1;
      BakedChar *baked_chars = push_count(packer.baked_chars, num_glyphs);
      ASSERT(count(packer.baked_chars) < UINT16_MAX);
      push_count(buffer, bitmap_dim * bitmap_dim * pixel_bytes);

      // if return is positive, the first unused row of the bitmap
      // if return is negative, returns the negative of the number of characters that fit
      // if return is 0, no characters fit and no rows were used
      int result = bake_font_bitmap(file_buffer, 0, font_height, buffer, bitmap_dim, bitmap_dim, first_glyph, num_glyphs, baked_chars);
      ASSERT(result > 0); // TODO handle error

      font->bitmap_offset = packer.current_data_offset;
      font->width = bitmap_dim;
      font->height = bitmap_dim;
      font->char_index = baked_chars - packer.baked_chars;

      int first_unused_row = result;
      if (first_unused_row < bitmap_dim / 2) {
        u32 new_height = next_power_2(first_unused_row);
        ASSERT(new_height <= bitmap_dim);
        if (new_height < bitmap_dim) {
          font_height = new_height;
        }
      }

      u64 bitmap_size = u64(font->width) * font->height * pixel_bytes;
      packer.current_data_offset += bitmap_size;

      u64 written = fwrite(buffer, 1, bitmap_size, out);
      ASSERT_EQUAL(written, bitmap_size); // TODO handle error

      clear(buffer);
    }
    clear(file_allocator);
  }
  dfree(buffer);
  free(file_allocator->memory);
}

static void write_bitmaps(FILE *out) {

  ASSERT_EQUAL(count(packer.bitmap_filenames), count(packer.groups));

  char directory_buffer[DIRECTORY_BUFFER_SIZE] = "assets/";
  lstring directory_name = length_string(directory_buffer);

  for (u32 i = 0; i < count(packer.bitmap_filenames); i++) {
   
    lstring filename = packer.bitmap_filenames[i];
    auto group = packer.groups + i;
    
    lstring full_filename = append(directory_name, filename, DIRECTORY_BUFFER_SIZE - 1);
    zero_terminate(full_filename);
    printf("Writing %s\n", full_filename.str);
    PixelBuffer bitmap = load_image_file(full_filename.str);
    if (!is_initialized(&bitmap)) print_load_bitmap_error(full_filename.str);
    group->bitmap_offset = packer.current_data_offset;
    group->width = bitmap.width;
    group->height = bitmap.height;

    u64 bitmap_size = u64(bitmap.width) * bitmap.height * 4;
    packer.current_data_offset += bitmap_size;

    u64 written = fwrite(bitmap.buffer, 1, bitmap_size, out);
    ASSERT_EQUAL(written, bitmap_size); // TODO handle error
    free(bitmap.buffer);
  }
}

static void write_header(FILE *out, u32 header_size) {
  PushAllocator header_mem[1];
  *header_mem = new_push_allocator(header_size);
  ASSERT(is_initialized(header_mem));

  packer.header.magic = 'PACK';
  packer.header.version = 0;

  packer.header.data_offset = header_size;
  packer.header.layout_count = count(packer.layouts);
  packer.header.font_type_count = count(packer.font_types);
  packer.header.texture_group_count = count(packer.groups);
  packer.header.total_size = header_size + packer.current_data_offset;

  PackedAssetHeader *asset_header = ALLOC_STRUCT(header_mem, PackedAssetHeader);
  ASSERT(asset_header);
  *asset_header = packer.header;

  u32 packed_animation_index = 0;
  u32 packed_face_index = 0;

  for (u32 i = 0; i < packer.header.layout_count; i++) {
    auto layout = ALLOC_STRUCT(header_mem, PackedTextureLayout);
    ASSERT(layout);
    *layout = packer.layouts[i];
    switch (layout->layout_type) {
      case LAYOUT_CHARACTER : {
        u32 animation_count = layout->animation_count;
        auto animations = ALLOC_ARRAY(header_mem, PackedAnimation, animation_count);
        ASSERT(animations);
        ASSERT_EQUAL(animations, layout->animations); // Check alignment
        ASSERT(packed_animation_index + animation_count <= count(packer.animations));
        array_copy(packer.animations + packed_animation_index, animations, animation_count);
        packed_animation_index += animation_count;
      } break;
      case LAYOUT_TERRAIN : {
        u32 face_count = layout->face_count;
        auto face = ALLOC_STRUCT(header_mem, PackedFaces);
        ASSERT(face);
        ASSERT_EQUAL(face, layout->faces); // Check alignment
        ASSERT(packed_face_index + 1 <= count(packer.faces));
        *face = packer.faces[packed_face_index];
        packed_face_index++;
      } break;
      default : INVALID_SWITCH_CASE(layout->layout_type);
    }
  }

  auto groups = ALLOC_ARRAY(header_mem, PackedTextureGroup, packer.header.texture_group_count);
  ASSERT(groups);
  array_copy(packer.groups.p, groups, packer.header.texture_group_count);

  u32 packed_font_index = 0;

  for (u32 i = 0; i < packer.header.font_type_count; i++) {
    auto font_type = ALLOC_STRUCT(header_mem, PackedFontType);
    ASSERT(font_type);
    *font_type = packer.font_types[i];
    auto sizes = ALLOC_ARRAY(header_mem, PackedFont, font_type->size_count);
    ASSERT(sizes);
    ASSERT_EQUAL(sizes, font_type->sizes); // Check alignment
    ASSERT(packed_font_index + font_type->size_count <= count(packer.fonts));
    array_copy(packer.fonts + packed_font_index, sizes, font_type->size_count);
    packed_font_index += font_type->size_count;
  }

  auto baked_chars = ALLOC_ARRAY(header_mem, BakedChar, count(packer.baked_chars));
  ASSERT(baked_chars);
  array_copy(packer.baked_chars.p, baked_chars, count(packer.baked_chars));
  ASSERT_EQUAL(header_mem->bytes_allocated, header_size);

  u64 written = fwrite(header_mem->memory, 1, header_mem->bytes_allocated, out);
  ASSERT_EQUAL(written, header_mem->bytes_allocated); // TODO handle error
}

static void write_pack_file() {

  printf("Packing assets.\n");

  u32 header_size = sizeof(PackedAssetHeader);
  header_size += count(packer.layouts) * sizeof(PackedTextureLayout);
  header_size += count(packer.animations) * sizeof(PackedAnimation);
  header_size += count(packer.groups) * sizeof(PackedTextureGroup);
  header_size += count(packer.faces) * sizeof(PackedFaces);
  header_size += packer.total_char_count * sizeof(BakedChar);
  header_size += count(packer.fonts) * sizeof(PackedFont);
  header_size += count(packer.font_types) * sizeof(PackedFontType);


  // TODO I might want to make my own version of this, but clib is fine for now
  FILE *pack_file = fopen("assets/packed_assets.pack", "wb");
  // TODO handle error better
  ASSERT(pack_file);

  fseek(pack_file, header_size, SEEK_SET);
  write_bitmaps(pack_file);
  write_fonts(pack_file);
  fseek(pack_file, 0, SEEK_SET);
  write_header(pack_file, header_size);

  printf("Packed in %ld bytes\n", ftell(pack_file));
  fclose(pack_file);
}

static void parse_spec_file(char *filename) {
  printf("Parsing %s.\n", filename);

  *packer.spec_file = open_file(filename);

  int error = read_entire_file(packer.spec_file, packer.spec_allocator);
  ASSERT(!error);
  close_file(packer.spec_file);

  *packer.spec_stream = get_stream(packer.spec_file->buffer, packer.spec_file->size);
  packer.line_number = 0;

  //
  // Parse the first line, which must be the version number :
  //

  lstring version_ln = pop_line(packer.spec_stream);
  packer.line_number++;

  Token command = parse_command(version_ln);
  if (!command) print_error(command.error, version_ln);
  if (CommandKeyword(command) != COMMAND_VERSION) {
    print_error("VERSION not specified");
  }
  auto args = parse_version_args(command.remainder);
  if (args.error) print_error(args.error, version_ln);

  if (args.version_number != 0) {
    print_error("Only version number 0 is currently accepted", version_ln);
  }

  //
  // Set up attributes to their default values :
  //

  attributes.animation_index = 0;
  attributes.layout_type = LAYOUT_INVALID;
  attributes.group_flags = 0;
  attributes.min_blend = LINEAR_BLEND;
  attributes.max_blend = LINEAR_BLEND;
  attributes.s_clamp = CLAMP_TO_EDGE;
  attributes.t_clamp = CLAMP_TO_EDGE;
  attributes.offset = vec3(0);
  attributes.sprite_depth = 0;
  attributes.character_range_start = ' ';
  attributes.character_range_end = '~';

  //packer.current_data_offset = 0;

  while (has_remaining(packer.spec_stream)) {

    //
    // Parse each command, then its arguments, then push the result to a dynamic array
    //

    lstring next_line = pop_line(packer.spec_stream);
    packer.line_number++;

    Token command = parse_command(next_line);
    if (!command) print_error(command.error, next_line);

    CommandKeyword command_num = CommandKeyword(command);
    switch (command_num) {
      case COMMAND_NONE    : { // Comment or newline, do nothing
      } break;
      case COMMAND_COMMENT_BEGIN : {
        while (has_remaining(packer.spec_stream)) {
          lstring next_line = pop_line(packer.spec_stream);
          packer.line_number++;

          Token comment_token = parse_comment_end(next_line);
          if (!comment_token) FAILURE("Error during comment.", comment_token.error);
          auto comment_command = CommandKeyword(comment_token);
          if (comment_command == COMMAND_NONE) continue;
          if (comment_command == COMMAND_COMMENT_END) break;
          
          FAILURE("Invalid command from parse_comment_end.", comment_command);
        }
      } break;

      case COMMAND_VERSION : {
        print_error("VERSION command repeated", next_line);
      } break; 

#define DO_LAYOUT() \
        auto args = parse_layout_args(command.remainder); \
        if (args.error) print_error(args.error, next_line); \
        auto layout = push(packer.layouts); \
        layout->layout_type = args.type;

      case COMMAND_ANIMATION_LAYOUT  : {
        DO_LAYOUT();
        layout->animation_count = 0;
        attributes.animation_index = 0;
      } break; 

      case COMMAND_CUBE_LAYOUT : {
        DO_LAYOUT();
        layout->face_count = 0;
      } break;

      case COMMAND_ANIMATION : {
        auto args = parse_animation_args(command.remainder);
        if (args.error) print_error(args.error, next_line);
        auto layout = peek(packer.layouts);
        if (!layout) print_error(ANIMATION_BEFORE_LAYOUT_SPECIFIED, next_line);
        // TODO handle this error for real
        ASSERT(layout->layout_type == LAYOUT_CHARACTER);
        auto animation = push(packer.animations);
        *animation = {};
        layout->animation_count++;
        animation->animation_type = args.type;
        animation->frame_count = args.frames;
        animation->duration = args.duration;
        animation->direction = args.direction;
        animation->start_index = attributes.animation_index;
        attributes.animation_index += args.frames;
      } break;

      case COMMAND_FACE : {
        auto args = parse_face_args(command.remainder);
        if (args.error) print_error(args.error, next_line);
        auto layout = peek(packer.layouts);
        if (!layout) print_error(FACE_BEFORE_LAYOUT_SPECIFIED, next_line);
        // TODO handle this error for real
        ASSERT(layout->layout_type == LAYOUT_TERRAIN);
        PackedFaces *face;
        if (layout->face_count) {
          face = peek(packer.faces);
        } else {
          face = push(packer.faces);
          *face = {};
        }
        layout->face_count++;
        face->sprite_index[args.face] = args.index;
      } break;

      case COMMAND_SET : {
        auto args = parse_set_args(command.remainder);
        if (args.error) print_error(args.error, next_line);

        switch (args.attribute) {
          case ATTRIBUTE_LAYOUT : 
            attributes.layout_type = args.layout; break;
          case ATTRIBUTE_HAS_NORMAL_MAP :
            if (args.has_normal_map)
              attributes.group_flags |= GROUP_HAS_NORMAL_MAP;
            else
              attributes.group_flags &= ~GROUP_HAS_NORMAL_MAP; break;
          case ATTRIBUTE_MIN_BLEND : 
            attributes.min_blend = args.blend_mode; break;
          case ATTRIBUTE_MAX_BLEND :
            attributes.max_blend = args.blend_mode; break;
          case ATTRIBUTE_S_CLAMP :
            attributes.s_clamp = args.clamp_mode; break;
          case ATTRIBUTE_T_CLAMP :
            attributes.t_clamp = args.clamp_mode; break;
          case ATTRIBUTE_OFFSET :
            attributes.offset = args.offset; break;
          case ATTRIBUTE_SPRITE_DEPTH :
            attributes.sprite_depth = args.sprite_depth; break;
          case ATTRIBUTE_SPRITE_INDEX :
            attributes.animation_index = args.sprite_index; break;
          case ATTRIBUTE_CHARACTER_RANGE :
            attributes.character_range_start = args.start_char;
            attributes.character_range_end = args.end_char; break;
          default : INVALID_SWITCH_CASE(args.attribute);
        }
      } break;

      case COMMAND_BITMAP : {
        auto args = parse_bitmap_args(command.remainder);
        if (args.error) print_error(args.error, next_line);
        if (attributes.layout_type == LAYOUT_INVALID)
          print_error(BITMAP_BEFORE_LAYOUT_SPECIFIED, next_line);
        
        auto group = push(packer.groups);
        //group->bitmap_offset = packer.current_data_offset;
        group->texture_group_id = args.id;
        group->layout_type = attributes.layout_type;
        group->sprite_count = args.sprite_count;
        group->sprite_width = args.sprite_width;
        group->sprite_height = args.sprite_height;

        group->flags = attributes.group_flags;
        group->min_blend = attributes.min_blend;
        group->max_blend = attributes.max_blend;
        group->s_clamp = attributes.s_clamp;
        group->t_clamp = attributes.t_clamp;
        group->offset_x = attributes.offset.x;
        group->offset_y = attributes.offset.y;
        group->offset_z = attributes.offset.z;
        group->sprite_depth = attributes.sprite_depth;

        push(packer.bitmap_filenames, args.filename);

        printf("Loading bitmap : %.*s\n", args.filename.len, args.filename.str);
      } break;

      case COMMAND_FONT : {
        auto args = parse_font_args(command.remainder);
        if (args.error) print_error(args.error, next_line);

        auto font_type = push(packer.font_types);
        font_type->font_id = args.id;
        font_type->size_count = 0;

        push(packer.font_filenames, args.filename);

        printf("Loading font : %.*s\n", args.filename.len, args.filename.str);
      } break;

      case COMMAND_SIZE : {
        auto args = parse_size_args(command.remainder);
        if (args.error) print_error(args.error, next_line);

        auto font_type = peek(packer.font_types);
        if (!font_type) print_error("SIZE command cannot be called before FONT command", next_line);
        font_type->size_count++;

        auto font = push(packer.fonts);
        font->font_size = args.size;
        font->start = attributes.character_range_start;
        font->end = attributes.character_range_end;
        packer.total_char_count += font->end - font->start;

        printf("Size : %u\n", args.size);
      } break;

      case COMMAND_INVALID : {
        print_error(INVALID_COMMAND_NAME, next_line);
      } break;

      default : {
        print_lstring(next_line);
        INVALID_SWITCH_CASE(command_num);
      } break;
    }
  }

  printf("Parsing complete.\n");
}

int main(int argc, char *argv[]) {
  if (argc > 2) usage(argv[0]);
  // TODO I may actually want it to just read all the files ending in .spec
  char *build_filename = "assets/build.spec";
  if (argc == 2) build_filename = argv[1];

  init_string_table();
  *packer.spec_allocator = new_push_allocator(2048 * 4);
  parse_spec_file(build_filename);


  write_pack_file();

  printf("Packing successful.\n");
  return 0;
}

