#ifndef _LSTRING_H_
#define _LSTRING_H_

// For colored strings :
#define KNRM  "\x1B[0m"
#define KBLK  "\x1B[30m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define KBGBLK  "\x1B[40m"
#define KBGRED  "\x1B[41m"
#define KBGGRN  "\x1B[42m"
#define KBGYEL  "\x1B[43m"
#define KBGBLU  "\x1B[44m"
#define KBGMAG  "\x1B[45m"
#define KBGCYN  "\x1B[46m"
#define KBGWHT  "\x1B[47m"

struct lstring {
  char *str;
  u32 len;

  inline char &operator[](u32 i) {
    assert(i < len);
    return str[i];
  }

  inline lstring operator+(u32 i) {
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
  u32 len = 0;
  while (cstr[len]) {
    len++;
  }
  return {cstr, len};
}

inline lstring append(lstring a, lstring b, u32 max_len) {
  assert(max_len >= a.len);
  lstring result = a;
  result.len = min(max_len, a.len + b.len);
  for (u32 i = 0; i < b.len && (i + a.len) < result.len; i++) {
    result[i + a.len] = b[i];
  }
  return result;
}

inline lstring copy_string(lstring source, char *buffer, u32 buf_len) {
  assert(buffer != source.str);
  u32 length = min(source.len, buf_len);
  lstring result = {buffer, length};
  for (u32 i = 0; i < length; i++) {
    result[i] = source[i];
  }
  return result;
}

inline lstring append(lstring a, lstring b, char *buffer, u32 buf_len) {
  assert(buffer != b.str);
  if (buffer == a.str) return append(a, b, buf_len);
  lstring result = copy_string(a, buffer, buf_len);
  return append(result, b, buf_len);
}

inline void zero_terminate(lstring s, u32 buf_len = 0) {
  if (buf_len) assert(s.len < buf_len);
  else s.str[s.len] = '\0';
}

struct hstring {
  union {
    struct {
      char *str;
      u32 len;
    };
    lstring lstr;
  };
  u32 hash;

  inline operator lstring() { return lstr; }
  inline operator void*() {
    return (void *)(len > 0 && str);
  }
};

struct Stream {
  lstring data;
  u32 cursor;
};

static Stream get_stream(u8 *data, u32 len) {
  return {(char *)data, len, 0};
}

// djb2 by Dan Bernstein, found on stackoverflow
static hstring hash_string(u8 *str, u32 len, int end_char) {
  if (!str) return {};
  if (!len) return {};

  u32 hash = 5381; int c;

  u32 i;
  for (i = 0; i < len; i++) {
    c = str[i];
    if (c == end_char) return {(char *)str, i, hash};
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return {(char *)str, len, hash};
}

inline hstring hash_string(const char *str, u32 len, int end_char = -1) {
  return hash_string((u8 *) str, len, end_char);
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

#define STACK_STRING(name, lstr) \
    assert(lstr && lstr.len < 2048); \
    char name[lstr.len + 1]; \
    mem_copy(lstr.str, name, lstr.len); \
    name[lstr.len] = '\0'; \

// TODO do this instead 
// https://stackoverflow.com/questions/2137779/how-do-i-print-a-non-null-terminated-string-using-printf
static void print_hstring(hstring str) {
  STACK_STRING(tmp, str);
  printf("hstring : '%s', len : %u, hash : %u\n", tmp, str.len, str.hash);
}

static void print_lstring(lstring str) {
  STACK_STRING(tmp, str);
  printf("lstring : '%s', len : %u\n", tmp, str.len);
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

static hstring pop_hline(Stream *stream) {
  assert(stream); assert(stream->data);
  lstring remainder = stream->data + stream->cursor;
  hstring result = hash_string(remainder, '\n');
  return result;
}

inline lstring remove_whitespace(lstring line) {
  uint32_t i;
  for (i = 0; i < line.len; i++) {
    if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n') continue;
    break;
  }
  return line + i;
}

#endif
