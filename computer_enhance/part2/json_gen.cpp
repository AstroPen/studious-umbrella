
#include <common.h>
#include <errno.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include <lstring.h>
#include <dynamic_array.h>

#include "haversine.h"


// TODO Improve this estimate
#define MAX_PAIR_STR_SIZE 110

int try_write_pair(darray<char> buffer, CoordPair pair) {
  return snprintf(buffer, MAX_PAIR_STR_SIZE, "  {\"x0\":%16.12f, \"y0\":%16.12f, \"x1\":%16.12f, \"y1\":%16.12f}", pair.x0, pair.y0, pair.x1, pair.y1);
}

void output_json_file(darray<CoordPair> pairs) {
  u32 len = count(pairs);

  darray<char> buffer;
  initialize(buffer, MAX_PAIR_STR_SIZE * len);
  int max_line_length = 0;
  lstring header_str = CONST_LENGTH_STRING("{\"pairs\":[\n");
  lstring footer_str = CONST_LENGTH_STRING("]}\n");

  copy_string(header_str, push_count(buffer, header_str.len), header_str.len);

  for (int i = 0; i < len; i++) {
    u32 allocated_space = MAX_PAIR_STR_SIZE;
    auto dest = push_count(buffer, allocated_space);
    auto pair = pairs[i];
    int needed_space = try_write_pair(dest, pair);
    max_line_length = max(max_line_length, needed_space);

    if (needed_space >= allocated_space) {
      printf("Line too long: %d\n", needed_space);
      u32 extra_space = needed_space + 1 - allocated_space;
      push_count(buffer, extra_space);
      allocated_space += extra_space;

      int final_len = try_write_pair(dest, pair);
      ASSERT_EQUAL(needed_space, final_len);
    }
    ASSERT(allocated_space > needed_space);
    pop_count(buffer, allocated_space - needed_space);
    if (i < len - 1) push(buffer, ','); // Need to skip the last line
    push(buffer, '\n');
  }
  copy_string(footer_str, push_count(buffer, footer_str.len), footer_str.len);
  printf("Max line length: %d\n", max_line_length);

  FILE *out_file = fopen("out.json", "wb");
  ASSERT(out_file);
  fwrite(buffer, 1, count(buffer), out_file);
  fclose(out_file);
}

f64 rand_float(f64 rmin, f64 rmax) {
  // FIXME This is a hack to add precision
  bit64 r = bit64{
    .low = rand(),
    .high = rand(),
  };
  bit64 r_max = bit64{
    .low = RAND_MAX,
    .high = RAND_MAX,
  };
  f64 t = (f64)r.u/(f64)r_max.u;
  return lerp(rmin, rmax, t);
}

Coordinate gen_coord(CoordPair bounds) {
  return Coordinate{rand_float(bounds.x0, bounds.x1), rand_float(bounds.y0, bounds.y1)};
}

CoordPair gen_rand_cluster_bounds() {
  CoordPair result;
  f64 xa = rand_float(-180, 180);
  f64 xb = rand_float(-180, 180);
  f64 ya = rand_float(-90, 90);
  f64 yb = rand_float(-90, 90);
  result.x0 = min(xa, xb);
  result.x1 = max(xa, xb);
  result.y0 = min(ya, yb);
  result.y1 = max(ya, yb);
  return result;
}

darray<CoordPair> gen_pairs_clustered(u64 num_pairs) {
  darray<CoordPair> result;
  auto cluster_a = gen_rand_cluster_bounds();
  auto cluster_b = gen_rand_cluster_bounds();
  for (int i = 0; i < num_pairs; i++) {
    push(result, CoordPair{
      .p0 = gen_coord(cluster_a),
      .p1 = gen_coord(cluster_b),
    });
  }
  return result;
}

darray<CoordPair> gen_pairs_uniform(u64 num_pairs) {
  darray<CoordPair> result;
  auto bounds = CoordPair{
    .x0 = -180,
    .y0 = -90,
    .x1 = 180,
    .y1 = 90,
  };
  for (int i = 0; i < num_pairs; i++) {
    push(result, CoordPair{
      .p0 = gen_coord(bounds),
      .p1 = gen_coord(bounds),
    });
  }
  return result;
}

void usage(char *program_name) {
  printf("usage: %s [uniform/cluster] [random seed] [num coordinate pairs]\n", program_name);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 4) usage(argv[0]);

  auto method = argv[1];
  auto seed_str = argv[2];
  auto count_str = argv[3];

  bool cluster_mode = false;
  if (!strcmp(method, "uniform")) {
    cluster_mode = false;
  } else if (!strcmp(method, "cluster")) {
    cluster_mode = true;
  } else {
    usage(argv[0]);
  }

  u64 seed = strtoull(seed_str, NULL, 0);
  if (errno == ERANGE) {
    printf("Error: seed value out of range for u64");
    return ERANGE;
  }
  srand(seed);
  u64 num_coordinate_pairs = strtoull(count_str, NULL, 0);
  if (errno == ERANGE) {
    printf("Error: count value out of range for u64");
    return ERANGE;
  }

  printf("Method: %s\n", method);
  printf("Random seed: %llu\n", seed);
  printf("Pair count: %llu\n", num_coordinate_pairs);

  darray<CoordPair> pairs;
  if (cluster_mode) {
    pairs = gen_pairs_clustered(num_coordinate_pairs);
  } else {
    pairs = gen_pairs_uniform(num_coordinate_pairs);
  }

  f64 total_dist = compute_total_haversine_dist(pairs);
  f64 avg_dist = total_dist / count(pairs);

  printf("Expected sum: %f\n", total_dist);
  printf("Expected avg: %f\n", avg_dist);

  output_json_file(pairs);
  
  return 0;
}

