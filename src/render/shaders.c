#include "render/shaders.h"
#include <string.h>

#ifdef __EMSCRIPTEN__
#define SH_PRELUDE "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\n"
#else
#define SH_PRELUDE "#version 330\n"
#endif

const char *sh_compose_def(const char *body, const char *defs) {
    static char buf[10][14336];
    static int idx;
    char *b = buf[idx++ % 10];
    size_t pl = strlen(SH_PRELUDE), dl = strlen(defs), bl = strlen(body);
    if (pl + dl + bl + 1 > sizeof buf[0]) return SH_PRELUDE "void main(){}"; /* never in practice; compile error is loud */
    memcpy(b, SH_PRELUDE, pl);
    memcpy(b + pl, defs, dl);
    memcpy(b + pl + dl, body, bl + 1);
    return b;
}

const char *sh_compose(const char *body) { return sh_compose_def(body, ""); }

const char *SH_VS_MESH = "";

/* ---------------- toon (main lit pass) ---------------- */
const char *SH_VS_TOON =
"in vec3 vertexPosition;\n"
"in vec3 vertexNormal;\n"
"in vec2 vertexTexCoord;\n"
"uniform mat4 mvp;\n"
"uniform mat4 matModel;\n"
"uniform mat4 matNormal;\n"
"uniform mat4 uLightVP;\n"
"out vec2 vUv;\n"
"out vec3 vWorld;\n"
"out vec3 vNormal;\n"
"out vec4 vLight;\n"
"out vec3 vBind;\n"
"#ifdef SKINNED\n"
"in vec4 vertexBoneIds;\n"
"in vec4 vertexBoneWeights;\n"
"uniform mat4 boneMatrices[72];\n"
"mat4 skinMat(){\n"
"  return boneMatrices[int(vertexBoneIds.x)] * vertexBoneWeights.x + boneMatrices[int(vertexBoneIds.y)] * vertexBoneWeights.y +\n"
"         boneMatrices[int(vertexBoneIds.z)] * vertexBoneWeights.z + boneMatrices[int(vertexBoneIds.w)] * vertexBoneWeights.w;\n"
"}\n"
"#endif\n"
"void main(){\n"
"  vec3 lp = vertexPosition;\n"
"  vec3 ln = vertexNormal;\n"
"#ifdef SKINNED\n"
"  mat4 sk = skinMat();\n"
"  lp = (sk * vec4(vertexPosition, 1.0)).xyz;\n"
"  ln = mat3(sk) * vertexNormal;\n"
"#endif\n"
"  vec4 w = matModel * vec4(lp, 1.0);\n"
"  vUv = vertexTexCoord;\n"
"  vBind = vertexPosition;\n"
"  vWorld = w.xyz;\n"
"  vNormal = normalize(mat3(matNormal) * ln);\n"
"  vLight = uLightVP * w;\n"
"  gl_Position = mvp * vec4(lp, 1.0);\n"
"}\n";

const char *SH_FS_TOON =
"in vec3 vBind;\n"
"in vec2 vUv;\n"
"in vec3 vWorld;\n"
"in vec3 vNormal;\n"
"in vec4 vLight;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec4 colDiffuse;\n"
"uniform vec3 viewPos;\n"
"uniform vec3 uLightDir;\n"   /* direction TO the light */
"uniform vec3 uLightCol;\n"
"uniform vec3 uAmbTop;\n"
"uniform vec3 uAmbBot;\n"
"uniform sampler2D uRamp;\n"
"uniform sampler2D uShadow;\n"
"uniform float uShadowTexel;\n"
"uniform int uPcf;\n"
"uniform float uShadowBias;\n"
"uniform float uShadowStr;\n"
"uniform vec3 uRimCol;\n"
"uniform float uRimPow;\n"
"uniform float uRimStr;\n"
"uniform vec2 uSpec;\n"        /* strength, size (n.h threshold) */
"uniform vec3 uFogCol;\n"
"uniform float uFogDen;\n"
"uniform vec4 uMat;\n"         /* rimMul, specMul, emissive, flags */
"uniform vec3 uPaint1;\n"      /* outfit paint: top, bottom, shoes (bind-space regions) */
"uniform vec3 uPaint2;\n"
"uniform vec3 uPaint3;\n"
"uniform vec4 uPaintCfg;\n"    /* x: sleeve reach (|x| limit, 0 = sleeveless), y: pants hem y, z: shoe height, w: top hem y */
"float unpackDepth(vec4 c){ return dot(c, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0)); }\n"
"float shadowAt(vec4 lp, float ndl){\n"
"  vec3 p = lp.xyz / lp.w * 0.5 + 0.5;\n"
"  if (p.z > 1.0 || p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0) return 1.0;\n"
"  float cur = p.z - uShadowBias * (1.0 + 2.0 * (1.0 - max(ndl, 0.0)));\n"
"  float sum = 0.0, cnt = 0.0;\n"
"  for (int y = -2; y <= 2; y++) for (int x = -2; x <= 2; x++) {\n"
"    if (abs(x) > uPcf || abs(y) > uPcf) continue;\n"
"    float d = unpackDepth(texture(uShadow, p.xy + vec2(float(x), float(y)) * uShadowTexel));\n"
"    sum += (cur <= d) ? 1.0 : 0.0; cnt += 1.0;\n"
"  }\n"
"  return sum / cnt;\n"
"}\n"
"float hash21(vec2 p){ p = fract(p * vec2(123.34, 456.21)); p += dot(p, p + 45.32); return fract(p.x * p.y); }\n"
"void main(){\n"
"  int fl = int(uMat.w + 0.5);\n"
"  vec3 N = normalize(vNormal);\n"
"  if (!gl_FrontFacing) N = -N;\n"
"  vec3 V = normalize(viewPos - vWorld);\n"
"  vec3 L = normalize(uLightDir);\n"
"  vec3 base = pow(colDiffuse.rgb * texture(texture0, vUv).rgb, vec3(2.2));\n"
"  if ((fl & 1) != 0) {\n"                     /* stylized stone-tile ground */
"    vec2 g = vWorld.xz / 3.0;\n"
"    vec2 f = abs(fract(g - 0.5) - 0.5) / fwidth(g);\n"
"    float line = 1.0 - min(min(f.x, f.y), 1.0);\n"
"    float tone = 0.88 + 0.24 * hash21(floor(g));\n"
"    base *= tone * (1.0 - 0.45 * line);\n"
"  }\n"
"  if ((fl & 8) != 0) {\n"                     /* outfit paint by bind-space body region */
"    base = mix(pow(colDiffuse.rgb, vec3(2.2)), base, 0.22);\n"   /* flat anime skin: keep only a little texture detail */
"    float ax = abs(vBind.x), y = vBind.y;\n"
"    float torso = step(ax, 0.30) * smoothstep(uPaintCfg.w - 0.015, uPaintCfg.w + 0.015, y) * (1.0 - smoothstep(1.50, 1.54, y));\n"
"    float sleeve = step(0.30, ax) * (1.0 - smoothstep(uPaintCfg.x - 0.01, uPaintCfg.x + 0.01, ax)) * step(1.30, y) * step(0.01, uPaintCfg.x);\n"
"    float pants = step(ax, 0.32) * (1.0 - smoothstep(uPaintCfg.w - 0.015, uPaintCfg.w + 0.015, y)) * smoothstep(uPaintCfg.y - 0.012, uPaintCfg.y + 0.012, y);\n"
"    float shoes = (1.0 - smoothstep(uPaintCfg.z - 0.01, uPaintCfg.z + 0.01, y)) * step(ax, 0.32);\n"
"    vec3 pc = pow(uPaint1, vec3(2.2));\n"
"    base = mix(base, pc, clamp(torso + sleeve, 0.0, 1.0));\n"
"    base = mix(base, pow(uPaint2, vec3(2.2)), pants);\n"
"    base = mix(base, pow(uPaint3, vec3(2.2)), shoes);\n"
"  }\n"
"  float ndl = dot(N, L);\n"
"  float sh = ((fl & 4) != 0) ? 1.0 : shadowAt(vLight, ndl);\n"
"  float v = clamp(ndl * 0.5 + 0.5, 0.0, 1.0) * mix(1.0, 0.35 + 0.65 * sh, uShadowStr);\n"
"  vec3 ramp = texture(uRamp, vec2(v, 0.5)).rgb;\n"
"  vec3 amb = mix(uAmbBot, uAmbTop, N.y * 0.5 + 0.5);\n"
"  vec3 col = base * (ramp * uLightCol + amb);\n"
"  vec3 H = normalize(L + V);\n"
"  float ndh = max(dot(N, H), 0.0);\n"
"  float spec = smoothstep(uSpec.y, uSpec.y + 0.012, ndh) * uSpec.x * uMat.y * sh;\n"
"  if ((fl & 2) != 0) {\n"                     /* hair: anime highlight band */
"    float band = 1.0 - smoothstep(0.0, 0.07, abs(N.y - 0.42));\n"
"    float facing = smoothstep(-0.1, 0.5, dot(normalize(N.xz + 1e-4), normalize(V.xz + 1e-4)));\n"
"    spec += band * facing * 0.55 * sh;\n"
"  }\n"
"  col += spec * uLightCol;\n"
"  float fres = pow(1.0 - max(dot(N, V), 0.0), uRimPow);\n"
"  float rim = smoothstep(0.35, 0.55, fres) * uRimStr * uMat.x;\n"
"  col += rim * uRimCol * (0.4 + 0.6 * smoothstep(-0.4, 0.4, -dot(N, L) + 0.2));\n"
"  col = mix(col, base * 1.25 + vec3(0.04), uMat.z);\n"
"  float dist = length(viewPos - vWorld);\n"
"  float fog = (1.0 - exp(-dist * uFogDen)) * exp(-max(vWorld.y, 0.0) * 0.12);\n"
"  col = mix(col, pow(uFogCol, vec3(2.2)), clamp(fog, 0.0, 0.95));\n"
"  finalColor = vec4(pow(clamp(col, 0.0, 1.0), vec3(1.0/2.2)), clamp(max(uMat.z, rim * 0.35 + spec * 0.15), 0.0, 1.0));\n"
"}\n";

/* ---------------- inverted-hull outline ---------------- */
const char *SH_VS_OUTLINE =
"in vec3 vertexPosition;\n"
"in vec3 vertexNormal;\n"
"uniform mat4 matModel;\n"
"uniform mat4 matView;\n"
"uniform mat4 matProjection;\n"
"uniform mat4 matNormal;\n"
"uniform vec3 viewPos;\n"
"uniform float uOutlinePx;\n"   /* pixel-ish thickness, converted to world units by distance */
"uniform float uSmooth;\n"      /* 0 = use mesh normals, 1 = radial from object centre (fixes hard-edged cubes) */
"#ifdef SKINNED\n"
"in vec4 vertexBoneIds;\n"
"in vec4 vertexBoneWeights;\n"
"uniform mat4 boneMatrices[72];\n"
"mat4 skinMat(){\n"
"  return boneMatrices[int(vertexBoneIds.x)] * vertexBoneWeights.x + boneMatrices[int(vertexBoneIds.y)] * vertexBoneWeights.y +\n"
"         boneMatrices[int(vertexBoneIds.z)] * vertexBoneWeights.z + boneMatrices[int(vertexBoneIds.w)] * vertexBoneWeights.w;\n"
"}\n"
"#endif\n"
"void main(){\n"
"  vec3 lp = vertexPosition;\n"
"  vec3 nrm = vertexNormal;\n"
"#ifdef SKINNED\n"
"  mat4 sk = skinMat();\n"
"  lp = (sk * vec4(vertexPosition, 1.0)).xyz;\n"
"  nrm = mat3(sk) * vertexNormal;\n"
"#endif\n"
"  vec3 dir = normalize(mix(nrm, normalize(lp + vec3(0.0, 1e-4, 0.0)), uSmooth));\n"
"  vec4 w = matModel * vec4(lp, 1.0);\n"
"  vec3 nw = normalize(mat3(matNormal) * dir);\n"
"  float dist = length(viewPos - w.xyz);\n"
"  w.xyz += nw * clamp(dist * uOutlinePx, 0.002, 0.06);\n"
"  gl_Position = matProjection * matView * w;\n"
"}\n";

const char *SH_FS_OUTLINE =
"out vec4 finalColor;\n"
"uniform vec4 colDiffuse;\n"
"uniform vec3 uOutlineCol;\n"
"void main(){\n"
"  vec3 c = mix(uOutlineCol, pow(colDiffuse.rgb, vec3(2.2)) * 0.18, 0.35);\n"
"  finalColor = vec4(pow(c, vec3(1.0/2.2)), 0.0);\n"
"}\n";

/* ---------------- shadow depth (packed into RGBA8: portable, no depth-texture ext) ---------------- */
const char *SH_FS_SHADOW =
"out vec4 finalColor;\n"
"void main(){\n"
"  float d = gl_FragCoord.z;\n"
"  vec4 e = fract(d * vec4(1.0, 255.0, 65025.0, 16581375.0));\n"
"  e -= e.yzww * vec4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);\n"
"  finalColor = e;\n"
"}\n";

/* ---------------- fullscreen quad VS shared by sky / post ---------------- */
const char *SH_VS_QUAD =
"in vec3 vertexPosition;\n"
"in vec2 vertexTexCoord;\n"
"in vec4 vertexColor;\n"
"uniform mat4 mvp;\n"
"out vec2 vUv;\n"
"out vec4 vColor;\n"
"void main(){\n"
"  vUv = vertexTexCoord;\n"
"  vColor = vertexColor;\n"
"  gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
"}\n";

const char *SH_FS_SKY =
"in vec2 vUv;\n"
"out vec4 finalColor;\n"
"uniform vec3 uTop;\n"
"uniform vec3 uBottom;\n"
"uniform vec3 uGlow;\n"          /* horizon glow colour */
"uniform vec4 uDisc;\n"          /* x, y (uv), radius, halo size */
"uniform vec3 uDiscCol;\n"
"uniform float uStars;\n"
"uniform float uYaw;\n"
"uniform float uHorizon;\n"      /* screen-space y of the horizon (0 bottom, 1 top) */
"uniform vec2 uRes;\n"
"float hash(vec2 p){ p = fract(p * vec2(443.897, 441.423)); p += dot(p, p + 19.19); return fract(p.x * p.y); }\n"
"void main(){\n"
"  vec2 uv = vec2(vUv.x, 1.0 - vUv.y);\n"            /* y up: 1 = top of screen */
"  float asp = uRes.x / uRes.y;\n"
"  float t = clamp((uv.y - uHorizon) / max(1.0 - uHorizon, 0.2), 0.0, 1.0);\n"
"  vec3 c = mix(uBottom, uTop, pow(t, 0.8));\n"
"  c += uGlow * exp(-pow((uv.y - uHorizon) * 5.0, 2.0));\n"
"  vec2 sp = vec2(uv.x * asp - uYaw * 0.35, uv.y) * 140.0;\n"
"  vec2 cell = floor(sp);\n"
"  vec2 fp = fract(sp) - 0.5 + (vec2(hash(cell + 3.1), hash(cell + 9.7)) - 0.5) * 0.5;\n"
"  float star = step(0.985, hash(cell)) * smoothstep(0.22, 0.0, length(fp)) * smoothstep(0.1, 0.6, t) * uStars;\n"
"  c += vec3(star) * (0.5 + 0.8 * hash(cell + 7.0));\n"
"  vec2 dp = vec2(uv.x * asp - uYaw * 0.18, uv.y) - vec2(uDisc.x * asp, uDisc.y);\n"
"  float dd = length(dp);\n"
"  c += uDiscCol * smoothstep(uDisc.z + 0.004, uDisc.z, dd);\n"
"  c += uDiscCol * 0.35 * exp(-dd * (1.0 / max(uDisc.w, 1e-3)));\n"
"  finalColor = vec4(pow(clamp(c, 0.0, 1.0), vec3(1.0/2.2)), 0.0);\n"
"}\n";

/* ---------------- bloom ---------------- */
const char *SH_FS_BRIGHT =
"in vec2 vUv;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform float uThr;\n"
"void main(){\n"
"  vec4 s = texture(texture0, vUv);\n"
"  float luma = dot(s.rgb, vec3(0.299, 0.587, 0.114));\n"
"  float m = max(s.a, smoothstep(uThr, 1.0, luma) * 0.5);\n"
"  finalColor = vec4(s.rgb * m, 1.0);\n"
"}\n";

const char *SH_FS_BLUR =
"in vec2 vUv;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 uDir;\n"
"void main(){\n"
"  vec3 c = texture(texture0, vUv).rgb * 0.2270270270;\n"
"  c += texture(texture0, vUv + uDir * 1.3846153846).rgb * 0.3162162162;\n"
"  c += texture(texture0, vUv - uDir * 1.3846153846).rgb * 0.3162162162;\n"
"  c += texture(texture0, vUv + uDir * 3.2307692308).rgb * 0.0702702703;\n"
"  c += texture(texture0, vUv - uDir * 3.2307692308).rgb * 0.0702702703;\n"
"  finalColor = vec4(c, 1.0);\n"
"}\n";

/* ---------------- composite: FXAA-lite, bloom, ACES, grade, vignette ---------------- */
const char *SH_FS_COMPOSITE =
"in vec2 vUv;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform sampler2D uBloom;\n"
"uniform vec2 uTexel;\n"
"uniform float uFxaa;\n"
"uniform float uBloomStr;\n"
"uniform vec3 uBloomTint;\n"
"uniform float uExposure;\n"
"uniform float uSat;\n"
"uniform float uContrast;\n"
"uniform vec3 uShadowTint;\n"
"uniform vec3 uHighTint;\n"
"uniform float uVignette;\n"
"float luma(vec3 c){ return dot(c, vec3(0.299, 0.587, 0.114)); }\n"
"vec3 aces(vec3 x){ return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }\n"
"vec3 fxaa(vec2 uv){\n"
"  vec3 rgbM = texture(texture0, uv).rgb;\n"
"  if (uFxaa < 0.5) return rgbM;\n"
"  vec3 rgbNW = texture(texture0, uv + vec2(-1.0, -1.0) * uTexel).rgb;\n"
"  vec3 rgbNE = texture(texture0, uv + vec2( 1.0, -1.0) * uTexel).rgb;\n"
"  vec3 rgbSW = texture(texture0, uv + vec2(-1.0,  1.0) * uTexel).rgb;\n"
"  vec3 rgbSE = texture(texture0, uv + vec2( 1.0,  1.0) * uTexel).rgb;\n"
"  float lM = luma(rgbM), lNW = luma(rgbNW), lNE = luma(rgbNE), lSW = luma(rgbSW), lSE = luma(rgbSE);\n"
"  float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));\n"
"  float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));\n"
"  vec2 dir = vec2(-((lNW + lNE) - (lSW + lSE)), ((lNW + lSW) - (lNE + lSE)));\n"
"  float dirReduce = max((lNW + lNE + lSW + lSE) * 0.03125, 1.0/128.0);\n"
"  float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);\n"
"  dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * uTexel;\n"
"  vec3 rgbA = 0.5 * (texture(texture0, uv + dir * (1.0/3.0 - 0.5)).rgb + texture(texture0, uv + dir * (2.0/3.0 - 0.5)).rgb);\n"
"  vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(texture0, uv + dir * -0.5).rgb + texture(texture0, uv + dir * 0.5).rgb);\n"
"  float lB = luma(rgbB);\n"
"  return (lB < lMin || lB > lMax) ? rgbA : rgbB;\n"
"}\n"
"void main(){\n"
"  vec3 c = pow(fxaa(vUv), vec3(2.2));\n"
"  c += pow(texture(uBloom, vUv).rgb, vec3(2.2)) * uBloomStr * uBloomTint;\n"
"  c *= uExposure;\n"
"  c = aces(c);\n"
"  c = pow(c, vec3(1.0/2.2));\n"
"  float l = luma(c);\n"
"  c = mix(vec3(l), c, uSat);\n"
"  c = (c - 0.5) * uContrast + 0.5;\n"
"  c *= mix(uShadowTint, uHighTint, smoothstep(0.1, 0.9, l));\n"
"  vec2 q = vUv - 0.5;\n"
"  c *= 1.0 - uVignette * smoothstep(0.25, 0.85, dot(q, q) * 2.2);\n"
"  finalColor = vec4(clamp(c, 0.0, 1.0), 1.0);\n"
"}\n";
