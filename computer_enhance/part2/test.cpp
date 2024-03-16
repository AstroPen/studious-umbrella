#include <common.h>
#include <errno.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include <lstring.h>
#include <dynamic_array.h>

#include "timers.h"
#include "haversine.h"

struct Parser {
  char *cursor;
  char *end;
  char *start_of_line;
  int line_number;
  bool err;
};

bool has_remaining(Parser *p) {
  return p->cursor < p->end;
}

void print_context(Parser *p) {
  char *end_p = p->end;
  if (end_p > p->cursor + 20) end_p = p->cursor + 20;
  lstring before = length_string(p->start_of_line, p->cursor - p->start_of_line);
  lstring after = length_string(p->cursor, end_p - p->cursor);
  printf(KRED "\t%.*s^%.*s\n" KNRM, before.len, before.str, after.len, after.str);
}

void set_parse_error_unexpected_char(Parser *p, char expected, char got) {
  p->err = true;
  printf("[Error on line %d] Failed to parse json: expected %c, got %c\n", p->line_number, expected, got);
  print_context(p);
}

void set_parse_error_eof(Parser *p) {
  p->err = true;
  printf("[Error on line %d] Failed to parse json: unexpected end of file.\n", p->line_number);
  print_context(p);
}

void set_parse_error_unexpected_key(Parser *p, lstring expected, lstring got) {
  p->err = true;
  printf(
    "[Error on line %d] Failed to parse json: expected key %.*s, got %.*s\n",
    p->line_number, expected.len, expected.str, got.len, got.str
  );
  print_context(p);
}

void set_parse_error_unexpected_key(Parser *p, u16 key) {
  p->err = true;
  printf("[Error on line %d] Failed to parse json: unexpected key %x.\n", p->line_number, key);
  print_context(p);
}

void set_parse_error_float_out_of_range(Parser *p, f64 f) {
  p->err = true;
  printf("[Error on line %d] Failed to parse json: float out of range: %f.\n", p->line_number, f);
  print_context(p);
}

void remove_whitespace(Parser *p) {
  if (p->err) return;
  while (has_remaining(p)) {
    if (p->cursor[0] == ' ') p->cursor++;
    else if (p->cursor[0] == '\n') {
      p->cursor++;
      p->line_number++;
      p->start_of_line = p->cursor;
    } else {
      return;
    }
  }
}

void pop_expected_char(Parser *p, char c) {
  if (p->err) return;
  remove_whitespace(p);
  if (!has_remaining(p)) {
    set_parse_error_eof(p);
    return;
  }
  if (p->cursor[0] == c) {
    p->cursor++;
    return;
  }
  set_parse_error_unexpected_char(p, c, p->cursor[0]);
}

void pop_expected_key(Parser *p, lstring key) {
  pop_expected_char(p, '"');
  if (p->err) return;
  if (p->end - p->cursor < key.len) {
    set_parse_error_eof(p);
    return;
  }
  auto parsed_key = length_string(p->cursor, key.len);
  if (!str_equal(key, parsed_key)) {
    set_parse_error_unexpected_key(p, key, parsed_key);
    return;
  }
  p->cursor += key.len;
  pop_expected_char(p, '"');
}

f64 pop_float(Parser *p) {
  char *end;
  // FIXME This can potentially buffer overflow
  f64 result = strtod(p->cursor, &end);
  if (errno == ERANGE) {
    set_parse_error_float_out_of_range(p, result);
  } else {
    p->cursor = end;
  }
  return result;
}

CoordPair parse_coord_keys(Parser *p) {
  TIMED_FUNCTION();
  CoordPair pair = CoordPair{NAN,NAN,NAN,NAN};
  if (p->err) return pair;
  pop_expected_char(p, '{');
  for (int i = 0; i < 4; i++) {
    pop_expected_char(p, '"');
    if (p->err) return pair;
    if (p->end - p->cursor < 2) {
      set_parse_error_eof(p);
      return pair;
    }
    f64 *target = NULL;
    u16 key = *((u16 *)p->cursor);
    switch (key) {
      // NOTE Multi-char constants are stored big endian
      case '0x': target = &pair.x0; break;
      case '0y': target = &pair.y0; break;
      case '1x': target = &pair.x1; break;
      case '1y': target = &pair.y1; break;
    }
    if (!target) {
      set_parse_error_unexpected_key(p, key);
      return pair;
    }
    p->cursor += 2;
    
    pop_expected_char(p, '"');
    pop_expected_char(p, ':');

    f64 f = pop_float(p);
    if (i != 3) pop_expected_char(p, ',');
    if (p->err) return pair;
    *target = f;
  }
  pop_expected_char(p, '}');
  return pair;
}

File load_file(char *filename) {
  TIMED_FUNCTION();

  File json_file = open_file(filename);
  PushAllocator allocator = new_push_allocator(json_file.size);
  int error = read_entire_file(&json_file, &allocator);
  ASSERT(!error);
  close_file(&json_file);
  return json_file;
}

darray<CoordPair> parse_json_file(File json_file) {
  FUNCTION_BANDWIDTH(json_file.loaded_size);

  Parser parser = Parser{
    .cursor = (char *)json_file.buffer,
    .end = (char *)json_file.buffer + json_file.loaded_size,
    .start_of_line = (char *)json_file.buffer,
  };

  pop_expected_char(&parser, '{');
  pop_expected_key(&parser, CONST_LENGTH_STRING("pairs"));
  pop_expected_char(&parser, ':');
  pop_expected_char(&parser, '[');
  if (parser.err) return NULL;

  darray<CoordPair> pairs;
  bool comma = true;
  while (comma) {
    auto pair = parse_coord_keys(&parser);
    if (parser.err) return NULL;

    push(pairs, pair);

    remove_whitespace(&parser);
    if (!has_remaining(&parser) || parser.cursor[0] != ',') comma = false;
    else parser.cursor++;
  }
  pop_expected_char(&parser, ']');
  pop_expected_char(&parser, '}');
  if (parser.err) return NULL;

  return pairs;
}

void usage(char *program_name) {
  printf("usage: %s [uniform/cluster] [random seed] [num coordinate pairs]\n", program_name);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) usage(argv[0]);

  auto filename = argv[1];
  printf("Parsing %s.\n", filename);

  u64 start_time = read_cpu_timer();
  File json_file = load_file(filename);
  darray<CoordPair> pairs = parse_json_file(json_file);

  BEGIN_TIMED_BLOCK(compute);
  f64 total_dist = compute_total_haversine_dist(pairs);
  f64 avg_dist = total_dist / count(pairs);
  END_TIMED_BLOCK(compute);
  u64 end_time = read_cpu_timer();

  print_timers(start_time, end_time);

  printf("\nPair count: %d\n", count(pairs));
  printf("Sum: %f\n", total_dist);
  printf("Avg: %f\n", avg_dist);

  return 0;
}

CHECK_GLOBAL_TIMER_COUNT();

