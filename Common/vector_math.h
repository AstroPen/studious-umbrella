
#ifndef _VECTOR_MATH_H_
#define _VECTOR_MATH_H_

#include "scalar_math.h"

// TODO add [] overload
union V2 {
  struct {
    float x;
    float y;
  };

  float elements[2];
};

static inline V2 vec2(float x, float y) {
  return V2{x,y};
}

static inline V2 vec2(float f) {
  return V2{f,f};
}

union V3 {
  struct {
    float x;
    float y;
    float z;
  };

  struct {
    float r;
    float g;
    float b;
  };

  struct {
    V2 xy;
    float _pad1;
  };

  struct {
    float _pad2;
    V2 yz;
  };

  float elements[3];
};

static inline V3 vec3(float f) {
  return {f,f,f};
}

static inline V3 vec3(float x, float y, float z) {
  return {x,y,z};
}

static inline V3 vec3(V2 xy, float z) {
  V3 result;
  result.xy = xy;
  result.z = z;
  return result;
}

static inline V3 vec3(float x, V2 yz) {
  V3 result;
  result.x = x;
  result.yz = yz;
  return result;
}

union V4 {
  struct {
    float x;
    float y;
    float z;
    float w;
  };

  struct {
    V3 rgb;
    float _pad1;
  };

  struct {
    V3 xyz;
    float _pad2;
  };

  struct {
    float _pad3;
    V3 yzw;
  };

  struct {
    float r;
    float g;
    float b;
    float a;
  };

  float elements[4];
};

static inline V4 vec4(float f) {
  return {f,f,f,f};
}

static inline V4 vec4(float x, float y, float z, float w) {
  return {x,y,z,w};
}

static inline V4 vec4(V2 xy, float z, float w) {
  return {xy.x, xy.y, z, w};
}

static inline V4 vec4(V3 xyz, float w) {
  V4 result;
  result.xyz = xyz;
  result.w = w;
  return result;
}


static inline
V2 operator+(V2 a, V2 b) {
  return {a.x + b.x, a.y + b.y};
}

static inline
V3 operator+(V3 a, V3 b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline
V4 operator+(V4 a, V4 b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

static inline
V2 &operator+=(V2 &a, V2 b) {
  a = a + b;
  return a;
}

static inline
V3 &operator+=(V3 &a, V3 b) {
  a = a + b;
  return a;
}

static inline
V4 &operator+=(V4 &a, V4 b) {
  a = a + b;
  return a;
}

static inline
V2 operator-(V2 a, V2 b) {
  return {a.x - b.x, a.y - b.y};
}

static inline
V3 operator-(V3 a, V3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline
V4 operator-(V4 a, V4 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

static inline
V2 operator-(V2 a) {
  return {-a.x, -a.y};
}

static inline
V3 operator-(V3 a) {
  return {-a.x, -a.y, -a.z};
}

static inline
V4 operator-(V4 a) {
  return {-a.x, -a.y, -a.z, -a.w};
}

static inline
V2 &operator-=(V2 &a, V2 b) {
  a = a - b;
  return a;
}

static inline
V3 &operator-=(V3 &a, V3 b) {
  a = a - b;
  return a;
}

static inline
V4 &operator-=(V4 &a, V4 b) {
  a = a - b;
  return a;
}

static inline
V2 operator*(V2 v, float scalar) {
  return {scalar * v.x, scalar * v.y};
}

static inline
V3 operator*(V3 v, float scalar) {
  return {scalar * v.x, scalar * v.y, scalar * v.z};
}

static inline
V4 operator*(V4 v, float scalar) {
  return {scalar * v.x, scalar * v.y, scalar * v.z, scalar * v.w};
}

static inline
V2 operator*(float scalar, V2 v) {
  return v * scalar;
}

static inline
V3 operator*(float scalar, V3 v) {
  return v * scalar;
}

static inline
V4 operator*(float scalar, V4 v) {
  return v * scalar;
}

static inline
V2 &operator*=(V2 &v, float scalar) {
  v = v * scalar;
  return v;
}

static inline
V3 &operator*=(V3 &v, float scalar) {
  v = v * scalar;
  return v;
}

static inline
V4 &operator*=(V4 &v, float scalar) {
  v = v * scalar;
  return v;
}

static inline
V2 operator*(V2 v1, V2 v2) {
  return {v1.x * v2.x, v1.y * v2.y};
}

static inline
V3 operator*(V3 v1, V3 v2) {
  return {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

static inline
V4 operator*(V4 v1, V4 v2) {
  return {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w};
}

static inline
V2 &operator*=(V2 &v1, V2 v2) {
  v1 = v1 * v2;
  return v1;
}

static inline
V3 &operator*=(V3 &v1, V3 v2) {
  v1 = v1 * v2;
  return v1;
}

static inline
V4 &operator*=(V4 &v1, V4 v2) {
  v1 = v1 * v2;
  return v1;
}

static inline
V2 operator/(V2 v, float scalar) {
  assert(isfinite(scalar));
  return {v.x / scalar, v.y / scalar};
}

static inline
V3 operator/(V3 v, float scalar) {
  assert(isfinite(scalar));
  return {v.x / scalar, v.y / scalar, v.z / scalar};
}

static inline
V4 operator/(V4 v, float scalar) {
  assert(isfinite(scalar));
  return {v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar};
}

static inline
V2 &operator/=(V2 &v, float scalar) {
  v = v / scalar;
  return v;
}

static inline
V3 &operator/=(V3 &v, float scalar) {
  v = v / scalar;
  return v;
}

static inline
V4 &operator/=(V4 &v, float scalar) {
  v = v / scalar;
  return v;
}

static inline
V2 operator/(V2 v1, V2 v2) {
  return {v1.x / v2.x, v1.y / v2.y};
}

static inline
V3 operator/(V3 v1, V3 v2) {
  return {v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};
}

static inline
V4 operator/(V4 v1, V4 v2) {
  return {v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w};
}

static inline
V2 &operator/=(V2 &v1, V2 v2) {
  v1 = v1 / v2;
  return v1;
}

static inline
V3 &operator/=(V3 &v1, V3 v2) {
  v1 = v1 / v2;
  return v1;
}

static inline
V4 &operator/=(V4 &v1, V4 v2) {
  v1 = v1 / v2;
  return v1;
}

static inline
bool operator==(V2 a, V2 b) {
  return a.x == b.x && a.y == b.y;
}

static inline
bool operator==(V3 a, V3 b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

static inline
bool operator==(V4 a, V4 b) {
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

static inline
bool operator!=(V2 a, V2 b) {
  return !(a == b);
}

static inline
bool operator!=(V3 a, V3 b) {
  return !(a == b);
}

static inline
bool operator!=(V4 a, V4 b) {
  return !(a == b);
}

static inline
V3 max(V3 a, V3 b) {
  V3 result = {
    max(a.x, b.x),
    max(a.y, b.y),
    max(a.z, b.z)
  };
  return result;
}

static inline
V3 min(V3 a, V3 b) {
  V3 result = {
    min(a.x, b.x),
    min(a.y, b.y),
    min(a.z, b.z)
  };
  return result;
}

static inline float dot(V2 a, V2 b) {
  return a.x * b.x + a.y * b.y;
}

static inline float dot(V3 a, V3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float dot(V4 a, V4 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

#define length_sq(v) (dot((v),(v)))
#define towards(a, b) (dot((a),(b)) > 0)

static inline V3 cross(V3 a, V3 b) {
  V3 result;
  result.x = a.y * b.z - a.z * b.y;
  result.y = a.z * b.x - a.x * b.z;
  result.z = a.x * b.y - a.y * b.x;
  return result;
}

static inline float length(V2 v) {
  float r = dot(v,v);
  if (r < 0.00000001) return 0;
  r = sqrt(r);
  return r;
}

static inline float length(V3 v) {
  float r = dot(v,v);
  if (r < 0.00000001) return 0;
  r = sqrt(r);
  return r;
}

static inline float length(V4 v) {
  float r = dot(v,v);
  if (r < 0.00000001) return 0;
  r = sqrt(r);
  return r;
}

static inline V2 normalize(V2 v) {
  float len = length(v);
  if (len < 0.0000001) return {};
  return v / len;
}

static inline V3 normalize(V3 v) {
  float len = length(v);
  if (len < 0.0000001) return {};
  return v / len;
}

static inline V4 normalize(V4 v) {
  float len = length(v);
  if (len < 0.0000001) return {};
  return v / len;
}

static inline V2 clamp(V2 v, V2 min, V2 max) {
  V2 r;
  r.x = clamp(v.x, min.x, max.x);
  r.y = clamp(v.y, min.y, max.y);

  return r;
}

static inline V2 clamp(V2 v, float min, float max) {
  V2 r;
  r.x = clamp(v.x, min, max);
  r.y = clamp(v.y, min, max);

  return r;
}

static inline V3 clamp(V3 v, float min, float max) {
  V3 r;
  r.x = clamp(v.x, min, max);
  r.y = clamp(v.y, min, max);
  r.z = clamp(v.z, min, max);

  return r;
}

// TODO I don't think this works for negatives...
static inline V2 floor(V2 v) {
  float x = (int) v.x;
  float y = (int) v.y;
  return V2{x,y};
}

static inline V2 round(V2 v) {
  v += V2{0.5,0.5};
  return floor(v);
}

static inline V2 lerp(V2 v1, V2 v2, float dt) {
  auto result = (1.0f - dt) * v1 + dt * v2;
  return result;
}

static inline V3 lerp(V3 v1, V3 v2, float dt) {
  auto result = (1.0f - dt) * v1 + dt * v2;
  return result;
}

static inline V4 lerp(V4 v1, V4 v2, float dt) {
  auto result = (1.0f - dt) * v1 + dt * v2;
  return result;
}

static inline V2 reflect(V2 v, V2 surface_normal) {
  auto result = v - 2 * dot(v, surface_normal) * surface_normal;
  return result;
}

static inline V3 reflect(V3 v, V3 surface_normal) {
  auto result = v - 2 * dot(v, surface_normal) * surface_normal;
  return result;
}

static inline V4 reflect(V4 v, V4 surface_normal) {
  auto result = v - 2 * dot(v, surface_normal) * surface_normal;
  return result;
}

static inline V2 backward(V2 v) {
  return V2{v.y, v.x};
}

static inline bool non_zero(V2 v, float epsilon = 0.0001) {
  return non_zero(v.x, epsilon) || non_zero(v.y, epsilon);
}

static inline bool non_zero(V3 v, float epsilon = 0.0001) {
  return non_zero(v.x, epsilon) || non_zero(v.y, epsilon) || non_zero(v.z, epsilon);
}

static inline bool non_zero(V4 v, float epsilon = 0.0001) {
  return non_zero(v.x, epsilon) || non_zero(v.y, epsilon) || non_zero(v.z, epsilon) || non_zero(v.w, epsilon);
}

static inline bool equals(V2 a, V2 b, float epsilon = 0.0001) {
  return !non_zero(a - b, epsilon);
}

static inline bool equals(V3 a, V3 b, float epsilon = 0.0001) {
  return !non_zero(a - b, epsilon);
}

static inline bool equals(V4 a, V4 b, float epsilon = 0.0001) {
  return !non_zero(a - b, epsilon);
}

// TODO switch these to to_string type functions
static void print_vector(V2 v) {
  printf("{%f, %f}\n", v.x, v.y);
}

static void print_vector(V3 v) {
  printf("{%f, %f, %f}\n", v.x, v.y, v.z);
}

static void print_vector(V4 v) {
  printf("{%f, %f, %f, %f}\n", v.x, v.y, v.z, v.w);
}

#endif

