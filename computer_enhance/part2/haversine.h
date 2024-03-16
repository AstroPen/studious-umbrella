
#include <common.h>
#include <dynamic_array.h>

struct Coordinate {
  f64 x, y;
};

union CoordPair {
  Coordinate point[2];
  struct {
    f64 x0, y0, x1, y1;
  };
  struct {
    Coordinate p0, p1;
  };
};

f64 haversine_distance(CoordPair pair) {
  f64 dlat = degrees_to_radians(pair.y1 - pair.y0);
  f64 dlon = degrees_to_radians(pair.x1 - pair.x0);
  f64 lat1 = degrees_to_radians(pair.y0);
  f64 lat2 = degrees_to_radians(pair.y1);

  f64 a = squared(sin(dlat/2.0)) + cos(lat1)*cos(lat2)*squared(sin(dlon/2.0));
  f64 c = 2.0*asin(sqrt(a));

  f64 earth_radius = 6372.8;
  return earth_radius * c;
}

f64 compute_total_haversine_dist(darray<CoordPair> pairs) {
  auto len = count(pairs);
  f64 total = 0;
  for (int i = 0; i < len; i++) {
    total += haversine_distance(pairs[i]);
  }
  return total;
}

