
// TODO rename this file to "collision.h" or something

#include "vector_math.h"

struct Circle {
  V2 center;
  float radius;
};

struct AlignedRect {
  V2 center;
  V2 offset;
};

struct AlignedRect3 {
  V3 center;
  V3 offset;
};

struct AlignedBox {
  V3 center;
  V3 offset;
};

static inline V2 min_xy(AlignedRect rect) {
  return rect.center - rect.offset;
}

static inline float min_x(AlignedRect rect) {
  return min_xy(rect).x;
}

static inline float min_y(AlignedRect rect) {
  return min_xy(rect).y;
}

static inline V2 max_xy(AlignedRect rect) {
  return rect.center + rect.offset;
}

static inline float max_x(AlignedRect rect) {
  return max_xy(rect).x;
}

static inline float max_y(AlignedRect rect) {
  return max_xy(rect).y;
}

static inline V3 min_xyz(AlignedBox box) {
  return box.center - box.offset;
}

static inline V2 min_xy(AlignedBox box) {
  return min_xyz(box).xy;
}

static inline float min_x(AlignedBox box) {
  return min_xy(box).x;
}

static inline float min_y(AlignedBox box) {
  return min_xy(box).y;
}

static inline float min_z(AlignedBox box) {
  return min_xyz(box).z;
}

static inline V3 max_xyz(AlignedBox box) {
  return box.center + box.offset;
}

static inline V2 max_xy(AlignedBox box) {
  return max_xyz(box).xy;
}

static inline float max_x(AlignedBox box) {
  return max_xy(box).x;
}

static inline float max_y(AlignedBox box) {
  return max_xy(box).y;
}

static inline float max_z(AlignedBox box) {
  return max_xyz(box).z;
}

static inline AlignedRect aligned_rect(V2 center, float width, float height) {
  AlignedRect result;
  result.center = center;
  result.offset = V2{width, height} / 2;
  return result;
}

static inline AlignedRect aligned_rect(float min_x, float min_y, float max_x, float max_y) {
  AlignedRect result;
  result.center = V2{min_x + max_x, min_y + max_y} / 2;
  result.offset = V2{max_x - min_x, max_y - min_y} / 2;
  return result;
}

static inline AlignedRect aligned_rect(V2 min_p, V2 max_p) {
  AlignedRect result = aligned_rect(min_p.x, min_p.y, max_p.x, max_p.y);
  return result;
}

static inline void translate(AlignedRect *rect, V2 dp) {
  rect->center += dp;
}

static inline V2 center(AlignedRect r) {
  return r.center;
}

static inline float width_of(AlignedRect r) {
  return r.offset.x * 2;
}

static inline float height_of(AlignedRect r) {
  return r.offset.y * 2;
}


// NOTE This is actually a parallelagram
struct Rectangle {
  V3 center;
  V3 offsets[2];
};

static inline V3 center(Rectangle r) {
  return r.center;
}

static inline V3 center(AlignedRect3 r) {
  return r.center;
}

static inline Rectangle rectangle(V3 center, V3 offset_1, V3 offset_2) {
  Rectangle result;
  result.center = center;
  result.offsets[0] = offset_1;
  result.offsets[1] = offset_2;
  return result;
}

static inline Rectangle rectangle(AlignedRect rect) {
  Rectangle result = {};
  result.center.xy = center(rect);
  result.offsets[0] = v3(rect.offset, 0);
  rect.offset.x *= -1;
  result.offsets[1] = v3(rect.offset, 0);
  return result;
}

static inline Rectangle rectangle(AlignedRect rect, float z) {
  auto result = rectangle(rect);
  result.center.z = z;
  return result;
}

static inline Rectangle rectangle(AlignedRect3 rect) {
  Rectangle result = {};
  result.center = rect.center;
  result.offsets[0] = rect.offset;
  if (rect.offset.x) rect.offset.x *= -1;
  else               rect.offset.y *= -1;
  result.offsets[1] = rect.offset;
  return result;
}

static inline void translate(Rectangle *r, V3 dp) {
  r->center += dp;
}

struct Quad {
  V3 verts[4];
};

static inline Quad to_quad(Rectangle r) {
  Quad q;
  q.verts[0] = r.center - r.offsets[0];
  q.verts[1] = r.center - r.offsets[1];
  q.verts[2] = r.center + r.offsets[0];
  q.verts[3] = r.center + r.offsets[1];
  return q;
}

static inline Quad to_quad(AlignedRect rect) {
  // TODO inline this
  return to_quad(rectangle(rect));
}

struct VerticalLine {
  V2 bottom_pos;
  float height;
};

struct HorizontalLine {
  V2 left_pos;
  float width;
};

struct Motion2 {
  V2 v;
  V2 a;
  float dt;
};

struct Collision2 {
  V2 pos;
  V2 vel;
  V2 normal;
  float t;
};

struct Collision3 {
  V3 pos;
  V3 vel;
  V3 normal;
  float t;
};

struct Motion3 {
  V3 v;
  V3 a;
  float dt;
};

#define COLLISION_MISS (Collision2{{},{},{}, -INFINITY})
#define COLLISION3_MISS (Collision3{{},{},{}, -INFINITY})

static inline bool is_earlier_collision(Collision2 first, Collision2 second) {
  return is_earlier(first.t, second.t);
}

static inline Collision2 earlier_collision(Collision2 a, Collision2 b) {
  if (b.t < 0) return a;
  if (a.t < 0) return b;
  if (a.t < b.t) return a;
  return b;
}

static inline VerticalLine left_side(AlignedRect r) {
  float height = height_of(r);
  return VerticalLine{min_xy(r), height};
}

static inline VerticalLine right_side(AlignedRect r) {
  float height = height_of(r);
  return VerticalLine{V2{max_x(r), min_y(r)}, height};
}

static inline HorizontalLine top_side(AlignedRect r) {
  float width = width_of(r);
  return HorizontalLine{V2{min_x(r), max_y(r)}, width};
}

static inline HorizontalLine bottom_side(AlignedRect r) {
  float width = width_of(r);
  return HorizontalLine{min_xy(r), width};
}

static inline V3 center(AlignedBox r) {
  return r.center;
}

static inline void translate(AlignedBox *r, V3 dp) {
  r->center += dp;
}

static inline AlignedRect3 top_face(AlignedBox r) {
  AlignedRect3 result = {};
  result.center = r.center;
  result.center.z += r.offset.z;
  result.offset.xy = r.offset.xy;
  return result;
}

// TODO replace flatten with this
static inline AlignedRect top_rect(AlignedBox r) {
  AlignedRect result;
  result.center = r.center.xy;
  result.offset = r.offset.xy;
  return result;
}

static inline AlignedRect bot_rect(AlignedBox r) {
  AlignedRect result;
  result.center = r.center.xy;
  result.offset.x = -r.offset.x;
  result.offset.y = r.offset.x;
  return result;
}

static inline AlignedRect front_rect(AlignedBox r) {
  AlignedRect result;
  result.center.x = r.center.x;
  result.center.y = r.center.z;
  result.offset.x = r.offset.x;
  result.offset.y = r.offset.z;
  return result;
}

static inline AlignedRect back_rect(AlignedBox r) {
  AlignedRect result;
  result.center.x = r.center.x;
  result.center.y = r.center.z;
  result.offset.x = -r.offset.x;
  result.offset.y = r.offset.z;
  return result;
}

static inline AlignedRect right_rect(AlignedBox r) {
  AlignedRect result;
  result.center = r.center.yz;
  result.offset = r.offset.yz;
  return result;
}

static inline AlignedRect left_rect(AlignedBox r) {
  AlignedRect result;
  result.center = r.center.yz;
  result.offset.x = -r.offset.y;
  result.offset.y = r.offset.z;
  return result;
}

static inline AlignedRect3 bottom_face(AlignedBox r) {
  AlignedRect3 result = {};
  result.center = r.center;
  result.center.z -= r.offset.z;
  // NOTE I flip this to make offset point to the correct
  // corner when viewed FROM BELOW.
  result.offset.x = -r.offset.x;
  result.offset.y = r.offset.y;
  return result;
}

static inline AlignedRect3 front_face(AlignedBox r) {
  AlignedRect3 result = {};
  result.center = r.center;
  result.center.y -= r.offset.y;
  result.offset.x = r.offset.x;
  result.offset.z = r.offset.z;
  return result;
}

static inline AlignedRect3 back_face(AlignedBox r) {
  AlignedRect3 result = {};
  result.center = r.center;
  result.center.y += r.offset.y;
  // NOTE I flip this to make offset point to the correct
  // corner when viewed FROM BEHIND.
  result.offset.x = -r.offset.x;
  result.offset.z = r.offset.z;
  return result;
}

static inline AlignedRect3 right_face(AlignedBox r) {
  AlignedRect3 result = {};
  result.center = r.center;
  result.center.x += r.offset.x;
  result.offset.y = r.offset.y;
  result.offset.z = r.offset.z;
  return result;
}

static inline AlignedRect3 left_face(AlignedBox r) {
  AlignedRect3 result = {};
  result.center = r.center;
  result.center.x -= r.offset.x;
  // NOTE I flip this to make offset point to the correct
  // corner when viewed FROM THE LEFT.
  result.offset.y = -r.offset.y;
  result.offset.z = r.offset.z;
  return result;
}

static inline Quad top_quad(AlignedBox b) {
  Quad result;

  auto min_p = b.center - b.offset;
  auto max_p = b.center + b.offset;

  result.verts[0] = V3{min_p.x, min_p.y, max_p.z};
  result.verts[1] = V3{max_p.x, min_p.y, max_p.z};
  result.verts[2] = V3{max_p.x, max_p.y, max_p.z};
  result.verts[3] = V3{min_p.x, max_p.y, max_p.z};

  return result;
}

static inline Quad bot_quad(AlignedBox b) {
  Quad result;

  auto min_p = b.center - b.offset;
  auto max_p = b.center + b.offset;

  result.verts[0] = V3{max_p.x, min_p.y, min_p.z};
  result.verts[1] = V3{min_p.x, min_p.y, min_p.z};
  result.verts[2] = V3{min_p.x, max_p.y, min_p.z};
  result.verts[3] = V3{max_p.x, max_p.y, min_p.z};

  return result;
}

static inline Quad front_quad(AlignedBox b) {
  Quad result;

  auto min_p = b.center - b.offset;
  auto max_p = b.center + b.offset;

  result.verts[0] = V3{min_p.x, min_p.y, min_p.z};
  result.verts[1] = V3{max_p.x, min_p.y, min_p.z};
  result.verts[2] = V3{max_p.x, min_p.y, max_p.z};
  result.verts[3] = V3{min_p.x, min_p.y, max_p.z};

  return result;
}

static inline Quad back_quad(AlignedBox b) {
  Quad result;

  auto min_p = b.center - b.offset;
  auto max_p = b.center + b.offset;

  result.verts[0] = V3{max_p.x, max_p.y, min_p.z};
  result.verts[1] = V3{min_p.x, max_p.y, min_p.z};
  result.verts[2] = V3{min_p.x, max_p.y, max_p.z};
  result.verts[3] = V3{max_p.x, max_p.y, max_p.z};

  return result;
}

static inline Quad left_quad(AlignedBox b) {
  Quad result;

  auto min_p = b.center - b.offset;
  auto max_p = b.center + b.offset;

  result.verts[0] = V3{min_p.x, max_p.y, min_p.z};
  result.verts[1] = V3{min_p.x, min_p.y, min_p.z};
  result.verts[2] = V3{min_p.x, min_p.y, max_p.z};
  result.verts[3] = V3{min_p.x, max_p.y, max_p.z};

  return result;
}

static inline Quad right_quad(AlignedBox b) {
  Quad result;

  auto min_p = b.center - b.offset;
  auto max_p = b.center + b.offset;

  result.verts[0] = V3{max_p.x, min_p.y, min_p.z};
  result.verts[1] = V3{max_p.x, max_p.y, min_p.z};
  result.verts[2] = V3{max_p.x, max_p.y, max_p.z};
  result.verts[3] = V3{max_p.x, min_p.y, max_p.z};

  return result;
}

static inline AlignedRect flatten(AlignedBox r) {
  AlignedRect result = {};
  result.center = r.center.xy;
  result.offset = r.offset.xy;
  return result;
}

static inline AlignedBox aligned_box(AlignedRect r, float z, float height) {
  AlignedBox result;
  result.center = v3(center(r), z + height/2);
  result.offset = v3(r.offset, height / 2);
  return result;
}

// TODO fix this code, it doesn't work
static inline Collision2 ray_intersect(V2 position, Motion2 move, V2 point) {
  // point = position + velocity * t + 1/2 * acceleration * t*t
  // t = (-v +- sqrt(v*v + 2*a*diff)) / a
  // (t + v/a)^2 = (v^2 + 2a*diff) / a^2
  
  auto acceleration = move.a;
  auto velocity = move.v;
  auto diff = point - position;

  if (length(acceleration) == 0) {
    float t1 = diff.x / velocity.x;
    float t2 = diff.y / velocity.y;
    if (!equals(t1, t2)) return COLLISION_MISS;
    float t = t1 + t2 / 2.0;
    return Collision2{point, -normalize(velocity), {}, t};
  }

  V2 middle;
  middle.x = velocity.x * velocity.x + 2 * acceleration.x * diff.x;
  if (middle.x < 0) return COLLISION_MISS;
  middle.y = velocity.y * velocity.y + 2 * acceleration.y * diff.y;
  if (middle.y < 0) return COLLISION_MISS;

  float t1;
  float t2;

  if (acceleration.x) {
    float root = sqrt(middle.x);
    t1 = (root - velocity.x) / acceleration.x;
    t2 = (root + velocity.x) / acceleration.x;
  } else {
    float root = sqrt(middle.y);
    t1 = (root - velocity.y) / acceleration.y;
    t2 = (root + velocity.y) / acceleration.y;
  }

  float t = earlier(t1, t2); 

  // TODO consider early out here if t < 0

  float left_side = t + velocity.y / acceleration.y;
  left_side *= left_side;

  float right_side = velocity.y * velocity.y + 2 * acceleration.y * diff.y;
  right_side /= acceleration.y * acceleration.y;

  if (!equals(left_side, right_side)) return COLLISION_MISS;

  V2 final_velocity = velocity + acceleration * t;
  return Collision2{point, -normalize(final_velocity), final_velocity, t};
}

static inline Collision2 ray_intersect(V2 position, Motion2 move, VerticalLine line, V2 normal) {

  auto acceleration = move.a;
  auto velocity = move.v;
  auto top_pos = V2{line.bottom_pos.x, line.bottom_pos.y + line.height};

  // TODO uncomment this when I've handled the immediate collision problem better
#if 0
  if (position.x == line.bottom_pos.x) {
    if (position.y >= line.bottom_pos.y && position.y <= top_pos.y) {
      Collision2 result;
      result.pos = position;
      result.normal = normal;
      result.t = 0;
      result.vel = velocity;
      return result;
    }
  }
#endif

  if (!length_sq(acceleration)) {
    if (dot(velocity, normal) >= 0) return COLLISION_MISS;
    if (velocity.x) {
      float t = (line.bottom_pos.x - position.x) / velocity.x;
      if (t > move.dt) return COLLISION_MISS;

      auto p = velocity * t + position;
      if (p.y >= line.bottom_pos.y && p.y <= top_pos.y) {
        Collision2 result;
        result.pos = p;
        result.normal = normal;
        result.t = t;
        result.vel = velocity;
        return result;
      }
      return COLLISION_MISS;

    } else {
      // TODO consider testing end points
      return COLLISION_MISS;
    }
  }

  if (velocity.x || acceleration.x) {
    if (dot(velocity, normal) >= 0 && dot(acceleration, normal) >= 0) return COLLISION_MISS;
    float t;
    float dx = line.bottom_pos.x - position.x;
    if (acceleration.x) {
      float midx = velocity.x * velocity.x + 2 * acceleration.x * dx;
      if (midx < 0) return COLLISION_MISS;
      float root = sqrt(midx);
      assert(isfinite(root));

      float t1 = (-velocity.x + root) / acceleration.x;
      float t2 = (-velocity.x - root) / acceleration.x;
   
      if (t1 < 0 && t2 < 0) return COLLISION_MISS;
      t = earlier(t1, t2);

      if (t > move.dt) return COLLISION_MISS;
    } else {
      t = dx / velocity.x;
    }

    assert(isfinite(t));

    auto p = 0.5 * acceleration * t * t + velocity * t + position;
    if (p.y >= line.bottom_pos.y && p.y <= top_pos.y) {
      Collision2 result;
      result.pos = p;
      V2 final_velocity = velocity + t * acceleration;
      result.normal = normal;
      result.t = t;
      result.vel = final_velocity;
      return result;
    }
    return COLLISION_MISS;

  } else {
    // TODO consider testing end points
    return COLLISION_MISS;
  }
}

static inline Collision2 ray_intersect(V2 position, Motion2 move, HorizontalLine line, V2 normal) {
  auto result = ray_intersect(backward(position), Motion2{backward(move.v), backward(move.a), move.dt},
                              VerticalLine{backward(line.left_pos), line.width}, backward(normal));
  result.normal = backward(result.normal);
  result.pos = backward(result.pos);
  result.vel = backward(result.vel);
  return result;
}

// Helper function that ignores acceleration
static inline Collision2 linear_ray_intersect(V2 position, Motion2 move, Circle circ) {
  V2 c = position - circ.center;
  V2 v = move.v;
  float r = circ.radius;

  if (!length_sq(v)) return COLLISION_MISS;
  float root_interior = -4 * length_sq(c) * length_sq(v) + dot(c,v) + 4 * length_sq(v) * r*r;
  if (root_interior < 0) return COLLISION_MISS;
  float root = sqrt(root_interior);

  float t1 = -(root + dot(c,v)) / (2 * length_sq(v));
  float t2 = -(-root + dot(c,v)) / (2 * length_sq(v));
  float t = earlier(t1, t2);
  if (t < 0) return COLLISION_MISS;
  if (t > move.dt) return COLLISION_MISS;

  Collision2 result;
  result.pos = position + v * t;
  result.vel = v;
  result.t = t;
  result.normal = normalize(result.pos - circ.center);

  return result;
}

// TODO consider adding collision mode
static inline Collision2 ray_intersect(V2 position, Motion2 move, Circle circ, int num_iterations = 1) {
  // NOTE This approximates the collision by iterating a series of linear collision tests.
  float t_max = move.dt;
  //int num_iterations = 1;
  float t_step = t_max / num_iterations;

  for (int i = 0; i < num_iterations; i++) {
    move.v += move.a * t_step;
    auto collision = linear_ray_intersect(position, move, circ);

    if (collision.t >= 0) return collision;

    position += move.v * t_step;
  }
  return COLLISION_MISS;
}

static inline bool contains(AlignedRect rect, V2 position) {
  auto min_p = min_xy(rect);
  auto max_p = max_xy(rect);
  if (position.x <= min_p.x) return false;
  if (position.y <= min_p.y) return false;
  if (position.x >= max_p.x) return false;
  if (position.y >= max_p.y) return false;
  return true;
}

enum CollisionMode {
  SOLID_INTERIOR,
  SOLID_EXTERIOR
};

static inline Collision2 ray_intersect(V2 position, Motion2 move, AlignedRect rect, CollisionMode mode) {

  float interior_factor = 1.0f;
  if (mode == SOLID_EXTERIOR) {
    interior_factor = -1.0f;
  }

  Collision2 result;
  auto c0 = ray_intersect(position, move, left_side(rect), V2{-1,0} * interior_factor);

  result = c0;

  auto c1 = ray_intersect(position, move, right_side(rect), V2{1,0} * interior_factor);

  result = earlier_collision(result, c1);

  auto c2 = ray_intersect(position, move, bottom_side(rect), V2{0,-1} * interior_factor);

  result = earlier_collision(result, c2);

  auto c3 = ray_intersect(position, move, top_side(rect), V2{0,1} * interior_factor);
  
  result = earlier_collision(result, c3);

  return result;
}

// TODO handle different CollisionModes better
static inline Collision2 rect_intersect(AlignedRect r1, Motion2 move, AlignedRect r2, CollisionMode mode) {
  V2 center_p = center(r1);
  if (mode == SOLID_INTERIOR) {
    r2.offset += r1.offset;
  }
  if (mode == SOLID_EXTERIOR) {
    r2.offset -= r1.offset;
  }
  return ray_intersect(center_p, move, r2, mode);
}

static inline Collision3 ray_intersect(V3 p, Motion3 move, AlignedBox b) {
  // TODO

  auto box_min = min_xyz(b);
  auto box_max = max_xyz(b);

  auto t_box_min = (box_min - p) / move.v;
  auto t_box_max = (box_max - p) / move.v;

  auto t_min3 = min(t_box_min, t_box_max);
  auto t_max3 = max(t_box_min, t_box_max);

  float t_min = max(t_min3.x, max(t_min3.y, t_min3.z));
  float t_max = min(t_max3.x, min(t_max3.y, t_max3.z));

  bool hit_box = t_max >= t_min && t_min <= move.dt;
  // NOTE : this just checks to see if we started in the box:
  hit_box = hit_box && t_min > 0;

  if (!hit_box) return COLLISION3_MISS;


  Collision3 result;
  result.t = t_min;
  // TODO consider taking into account acceleration here
  result.pos = move.v + p;
  // TODO we can break this into parts to make it more accurate
  result.vel = move.v + move.a * t_min;

  if      (t_min == t_box_min.x) result.normal = V3{-1,0,0};
  else if (t_min == t_box_max.x) result.normal = V3{1,0,0};
  else if (t_min == t_box_min.y) result.normal = V3{0,-1,0};
  else if (t_min == t_box_max.y) result.normal = V3{0,1,0};
  else if (t_min == t_box_min.z) result.normal = V3{0,0,-1};
  else if (t_min == t_box_max.z) result.normal = V3{0,0,1};

  return result;
}

static inline Collision3 box_intersect(AlignedBox b1, Motion3 move, AlignedBox b2) {
  return COLLISION3_MISS;
}

// TODO finish implementing gjk

enum ShapeType {
  SHAPE_ALIGNED_RECT,
  SHAPE_CIRCLE
};

union Shape {
  Circle circ;
  AlignedRect a_rect;
  Rectangle rect;
  VerticalLine v_line;
  HorizontalLine h_line;

  Shape() { }
  Shape(Circle c) { circ = c; }
  Shape(AlignedRect r) { a_rect = r; }
  Shape(Rectangle r) { rect = r; }
  Shape(VerticalLine l) { v_line = l; }
  Shape(HorizontalLine l) { h_line = l; }

};

static V2 support(Shape s, ShapeType s_type, V2 direction) {
  switch (s_type) {
    case SHAPE_ALIGNED_RECT : {
      return V2{};
    } break;
    case SHAPE_CIRCLE : {
      // NOTE we may need to change this when we go to 3D; this is a sphere.
      Circle *circ = &s.circ;
      float len = 1.0f;
      if (!equals(length_sq(direction), 1.0f)) len = length(direction);
      direction *= circ->radius / len;
      return circ->center + direction;
    } break;
  }
  assert(false);
  return V2{};
}

static V2 support(Shape p, ShapeType p_type, 
                  Shape q, ShapeType q_type, V2 direction) {
  V2 point = support(p, p_type, direction) - support(q, q_type, -direction);
  return point;
}

struct Simplex {
  V2 points[4];
  int count;
};

static inline void push(Simplex *s, V2 point) {
  assert(s->count < 4);
  s->points[s->count] = point;
  s->count++;
}

#define towards(a, b) (dot((a),(b)) > 0)

static V2 nearest_simplex_line(Simplex *s) {
  assert(s->count == 2);
  auto b = s->points[0];
  auto a = s->points[1]; // NOTE : The newest points should stay to the right.

  auto ab = b - a;
  auto ao = -a;

  if (towards(ab, ao)) {
    auto dir = V2{};//cross(cross(ab, ao), ab);
    // TODO I don't know if a and b actually need to be flipped here :
    s->points[0] = a;
    s->points[1] = b;
    return dir;
  } else {
    s->count = 1;
    s->points[0] = a;
    return ao;
  }
}

static V2 nearest_simplex(Simplex *s) {
  assert(s->count);
  assert(s->count < 4); // TODO when we have 3D change this
  switch (s->count) {
    case 2 : {
      return nearest_simplex_line(s);
    } break;
    case 3 : {
    } break;
    case 4 : {
      // TODO handle 3D
    } break;
  }
  assert(false);
  return V2{};
}

static bool gjk_intersection(Shape p, ShapeType p_type, 
                             Shape q, ShapeType q_type, V2 axis) {
  V2 point = support(p, p_type, q, q_type, axis);
  Simplex s;
  push(&s, point);

  V2 direction = -point;

  while (true) {
    point = support(p, p_type, q, q_type, direction);
    if (dot(point, direction) < 0) {
      return false;
    }

    push(&s, point);

    // TODO
    return false;

  }
  return false;
}

