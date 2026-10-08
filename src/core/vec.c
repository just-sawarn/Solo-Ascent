#include "core/vec.h"
#include <math.h>

Vec3 vec3(float x, float y, float z) { return (Vec3){x, y, z}; }
Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 vec3_scale(Vec3 a, float s) { return (Vec3){a.x * s, a.y * s, a.z * s}; }
Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    return (Vec3){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}
float vec3_len(Vec3 a) { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float lerp_angle(float a, float b, float t) {
    const float tau = 6.28318530718f;
    float d = fmodf(b - a, tau);
    if (d > tau * 0.5f) d -= tau;
    if (d < -tau * 0.5f) d += tau;
    return a + d * t;
}
