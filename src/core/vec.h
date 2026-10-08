#ifndef SA_VEC_H
#define SA_VEC_H

typedef struct { float x, y, z; } Vec3;

Vec3 vec3(float x, float y, float z);
Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_scale(Vec3 a, float s);
Vec3 vec3_lerp(Vec3 a, Vec3 b, float t);
float vec3_len(Vec3 a);
float lerp_angle(float a, float b, float t); /* radians, shortest path */
float clampf(float v, float lo, float hi);

#endif
