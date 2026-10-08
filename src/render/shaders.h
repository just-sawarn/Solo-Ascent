#ifndef SA_SHADERS_H
#define SA_SHADERS_H
/* GLSL sources (ES 3.00 on web, 3.30 core natively). No permutations: a fixed small set of programs. */

/* Returns "prelude + body" in a static ring of buffers (valid until 8 more calls). */
const char *sh_compose(const char *body);

extern const char *SH_VS_MESH;      /* toon + shadow-depth + outline use their own VS; these are bodies */
extern const char *SH_VS_TOON;
extern const char *SH_FS_TOON;
extern const char *SH_VS_OUTLINE;
extern const char *SH_FS_OUTLINE;
extern const char *SH_FS_SHADOW;
extern const char *SH_VS_QUAD;
extern const char *SH_FS_SKY;
extern const char *SH_FS_BRIGHT;
extern const char *SH_FS_BLUR;
extern const char *SH_FS_COMPOSITE;

#endif
