
#ifndef _SCALAR_MATH_H_
#define _SCALAR_MATH_H_

inline u32 max(uint32_t a, uint32_t b) {
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

inline float squared(float f) {
  return f * f;
}

inline float abs(float a) {
  return (a >= 0) ? a : -a;
}

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
  float result = (1.0f - dt) * f1 + dt * f2;
  return result;
}

// TODO I apparently copied this from somewhere... Maybe double-check it?
static inline uint32_t next_power_2(uint32_t v) {
  uint32_t r;

  if (v > 1) {
    float f = (float)v;
    uint32_t t = 1U << ((*(unsigned int *)&f >> 23) - 0x7f);
    r = t << (t < v);
  }
  else {
    r = 1;
  }

  return r;
}

#endif

