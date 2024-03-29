
#ifndef _SCALAR_MATH_H_
#define _SCALAR_MATH_H_

inline u32 max(u32 a, u32 b) {
  return (a < b) ? b : a;
}

inline u64 max(u64 a, u64 b) {
  return (a < b) ? b : a;
}

inline int max(int a, int b) {
  return (a < b) ? b : a;
}

inline float max(float a, float b) {
  return (a < b) ? b : a;
}

inline double max(double a, double b) {
  return (a < b) ? b : a;
}

inline u32 min(u32 a, u32 b) {
  return (a > b) ? b : a;
}

inline u64 min(u64 a, u64 b) {
  return (a > b) ? b : a;
}

inline int min(int a, int b) {
  return (a > b) ? b : a;
}

inline float min(float a, float b) {
  return (a > b) ? b : a;
}

inline double min(double a, double b) {
  return (a > b) ? b : a;
}

inline float squared(float f) {
  return f * f;
}

inline double squared(double f) {
  return f * f;
}

inline float degrees_to_radians(float f) {
  return f * (M_PI/180.0f);
}

inline double degrees_to_radians(double f) {
  return f * (M_PI/180.0);
}

inline float radians_to_degrees(float f) {
  return f * (180.0f/M_PI);
}

inline double radians_to_degrees(double f) {
  return f * (180.0/M_PI);
}

/*
inline float abs(float a) {
  return (a >= 0) ? a : -a;
}
*/

/* NOTE : Already defined in stdlib
static inline int abs(int a) {
  return (a >= 0) ? a : -a;
}
*/

static inline bool is_earlier(float a, float b) {
  if (a < 0) return false;
  if (b < 0) return true;
  if (a >= b) return false;
  return true;
}

static inline float earlier(float a, float b) {
  if (b < 0) return a;
  if (a < 0) return b;
  return min(a,b);
}


static inline bool non_zero(float f, float epsilon = 0.0001) {
  if (f > epsilon) return true;
  if (f < -epsilon) return true;
  return false;
}

static inline bool equals(float f1, float f2, float epsilon = 0.0001) {
  return !non_zero(f1 - f2, epsilon);
}

static inline float clamp(float f, float mini, float maxi) {
  assert(mini <= maxi);

  float r;
  r = min(f, maxi);
  r = max(r, mini);

  return r;
}

static inline float lerp(float f1, float f2, float dt) {
  return (1.0f - dt) * f1 + dt * f2;
}

static inline double lerp(double f1, double f2, double dt) {
  return (1.0f - dt) * f1 + dt * f2;
}

// TODO I apparently copied this from somewhere... Maybe double-check it?
static inline u32 next_power_2(u32 v) {
  u32 r;

  if (v > 1) {
    float f = (float)v;
    u32 t = 1U << ((*(u32 *)&f >> 23) - 0x7f);
    r = t << (t < v);
  }
  else {
    r = 1;
  }

  return r;
}

#endif

