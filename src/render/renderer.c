#include "core/daynight.h"
#include "render/renderer.h"
#include <math.h>
#include <string.h>
#include <raymath.h>
#include <rlgl.h>
#include "core/log.h"
#include "render/shaders.h"

#define MAX_DRAWS 3072

typedef struct { const Mesh *mesh; Matrix xf; RenderMat mat; const Matrix *bones; int bone_count; } DrawCmd;

#define MAX_BONE_SETS 160
#define BONES_PER_SET 72

typedef struct {
    const char *name;
    Vector3 light_dir;         /* direction towards the light */
    Vector3 light_col;
    Vector3 amb_top, amb_bot;
    Vector3 shade0, shade1;    /* ramp tints: deep shadow, mid */
    float   t1, t2, soft;      /* ramp thresholds */
    float   shadow_str, shadow_bias;
    Vector3 rim_col; float rim_pow, rim_str;
    float   spec_str, spec_size;
    Vector3 outline_col; float outline_px;
    Vector3 sky_top, sky_bot, sky_glow; float disc_x, disc_y, disc_r, disc_halo; Vector3 disc_col; float stars;
    Vector3 fog_col; float fog_den;
    float   bloom_thr, bloom_str; Vector3 bloom_tint;
    float   exposure, sat, contrast, vignette;
    Vector3 grade_shadow, grade_high;
} Look;

#define V3(a, b, c) ((Vector3){a, b, c})

static const Look LOOKS[LOOK_COUNT] = {
    { /* 0: Solo-Leveling: cold moonlight, ink-dark shadows, violet rim, crisp thin lines, strong bloom */
      .name = "Shadow Monarch",
      .light_dir = V3(-0.38f, 0.80f, 0.46f), .light_col = V3(0.68f, 0.80f, 1.20f),
      .amb_top = V3(0.07f, 0.09f, 0.21f), .amb_bot = V3(0.02f, 0.015f, 0.05f),
      .shade0 = V3(0.13f, 0.12f, 0.26f), .shade1 = V3(0.46f, 0.48f, 0.78f),
      .t1 = 0.40f, .t2 = 0.60f, .soft = 0.018f, .shadow_str = 1.0f, .shadow_bias = 0.0025f,
      .rim_col = V3(0.50f, 0.40f, 1.40f), .rim_pow = 2.6f, .rim_str = 1.5f,
      .spec_str = 0.55f, .spec_size = 0.965f,
      .outline_col = V3(0.015f, 0.01f, 0.05f), .outline_px = 0.0021f,
      .sky_top = V3(0.012f, 0.016f, 0.07f), .sky_bot = V3(0.10f, 0.05f, 0.22f), .sky_glow = V3(0.16f, 0.07f, 0.30f),
      .disc_x = 0.72f, .disc_y = 0.78f, .disc_r = 0.055f, .disc_halo = 0.10f, .disc_col = V3(0.75f, 0.82f, 1.0f), .stars = 1.0f,
      .fog_col = V3(0.05f, 0.045f, 0.13f), .fog_den = 0.016f,
      .bloom_thr = 0.72f, .bloom_str = 1.05f, .bloom_tint = V3(0.75f, 0.70f, 1.25f),
      .exposure = 1.10f, .sat = 0.92f, .contrast = 1.14f, .vignette = 0.60f,
      .grade_shadow = V3(0.82f, 0.86f, 1.16f), .grade_high = V3(1.0f, 0.97f, 1.05f) },
    { /* 1: Shonen Adventure: bright sun, pastel shadows, warm outline, saturated */
      .name = "Bright Adventure",
      .light_dir = V3(0.50f, 0.80f, 0.40f), .light_col = V3(1.25f, 1.10f, 0.90f),
      .amb_top = V3(0.50f, 0.60f, 0.85f), .amb_bot = V3(0.32f, 0.28f, 0.22f),
      .shade0 = V3(0.55f, 0.50f, 0.78f), .shade1 = V3(0.84f, 0.80f, 0.95f),
      .t1 = 0.34f, .t2 = 0.54f, .soft = 0.03f, .shadow_str = 0.8f, .shadow_bias = 0.0025f,
      .rim_col = V3(1.0f, 0.88f, 0.55f), .rim_pow = 3.0f, .rim_str = 0.5f,
      .spec_str = 0.35f, .spec_size = 0.95f,
      .outline_col = V3(0.10f, 0.05f, 0.07f), .outline_px = 0.0027f,
      .sky_top = V3(0.18f, 0.40f, 0.85f), .sky_bot = V3(0.65f, 0.82f, 1.0f), .sky_glow = V3(0.20f, 0.15f, 0.06f),
      .disc_x = 0.70f, .disc_y = 0.84f, .disc_r = 0.035f, .disc_halo = 0.15f, .disc_col = V3(1.0f, 0.95f, 0.75f), .stars = 0.0f,
      .fog_col = V3(0.62f, 0.78f, 0.95f), .fog_den = 0.011f,
      .bloom_thr = 0.88f, .bloom_str = 0.30f, .bloom_tint = V3(1.0f, 0.95f, 0.85f),
      .exposure = 1.0f, .sat = 1.15f, .contrast = 1.0f, .vignette = 0.22f,
      .grade_shadow = V3(0.97f, 0.97f, 1.04f), .grade_high = V3(1.03f, 1.0f, 0.96f) },
    { /* 2: Dawnbrand (house look): Solo-Leveling contrast + rim/bloom, but a golden dawn key (the Ascendant motif) */
      .name = "Dawnbrand",
      .light_dir = V3(0.52f, 0.46f, 0.72f), .light_col = V3(1.45f, 1.08f, 0.72f),
      .amb_top = V3(0.13f, 0.17f, 0.36f), .amb_bot = V3(0.06f, 0.04f, 0.10f),
      .shade0 = V3(0.17f, 0.17f, 0.40f), .shade1 = V3(0.58f, 0.56f, 0.84f),
      .t1 = 0.38f, .t2 = 0.58f, .soft = 0.02f, .shadow_str = 1.0f, .shadow_bias = 0.0025f,
      .rim_col = V3(0.35f, 0.75f, 1.35f), .rim_pow = 2.7f, .rim_str = 1.25f,
      .spec_str = 0.5f, .spec_size = 0.96f,
      .outline_col = V3(0.03f, 0.015f, 0.06f), .outline_px = 0.0023f,
      .sky_top = V3(0.025f, 0.05f, 0.20f), .sky_bot = V3(0.62f, 0.25f, 0.14f), .sky_glow = V3(0.18f, 0.05f, 0.14f),
      .disc_x = 0.30f, .disc_y = 0.25f, .disc_r = 0.050f, .disc_halo = 0.20f, .disc_col = V3(1.0f, 0.78f, 0.45f), .stars = 0.35f,
      .fog_col = V3(0.30f, 0.20f, 0.34f), .fog_den = 0.015f,
      .bloom_thr = 0.74f, .bloom_str = 0.85f, .bloom_tint = V3(1.15f, 0.88f, 0.70f),
      .exposure = 1.08f, .sat = 1.05f, .contrast = 1.12f, .vignette = 0.45f,
      .grade_shadow = V3(0.88f, 0.90f, 1.12f), .grade_high = V3(1.04f, 0.99f, 0.94f) },
};

static const TierSettings TIERS[TIER_COUNT] = {
    /* name     shadow pcf  scale outl   fxaa  bloom div  shadows */
    { "ULTRA",  4096, 2, 1.00f, true,  true, 3, 2, true },
    { "HIGH",   2048, 1, 1.00f, true,  true, 2, 2, true },
    { "MEDIUM", 1024, 1, 1.00f, true,  true, 2, 4, true },
    { "LOW",    1024, 0, 0.85f, true,  true, 1, 4, true },
    { "MOBILE",  512, 0, 0.70f, true,  false, 1, 4, true },
};

typedef struct {
    bool ready;
    int  screen_w, screen_h, rt_w, rt_h;
    QualityTier tier;
    int  look;
    Look cur;            /* the look with the day/night environment applied */
    bool env_on; Env env;
    Shader toon, outline, shadow, sky, bright, blur, composite;
    Shader toon_sk, outline_sk, shadow_sk;
    Material m_toon, m_toon_sk, m_outline, m_outline_sk, m_shadow, m_shadow_sk;
    Matrix bone_arena[MAX_BONE_SETS * BONES_PER_SET];
    int bone_sets;
    RenderTexture2D scene, shadow_rt, bloom_a, bloom_b;
    Texture2D ramp, white;
    unsigned char ramp_px[256 * 4];
    Camera3D cam;
    float time;
    Matrix light_vp;
    DrawCmd cmds[MAX_DRAWS];
    int  ncmds;
    int  dropped;
    /* governor */
    int  gov_frames; float gov_ema; float gov_slow_time; bool gov_settled; int gov_step;
} R;

static R r;

static Vector3 v3n(Vector3 v) { return Vector3Normalize(v); }

static void build_ramp(void) {
    const Look *L = &LOOKS[r.look];
    for (int i = 0; i < 256; ++i) {
        float x = (float)i / 255.0f;
        float a = Clamp((x - (L->t1 - L->soft)) / (2.0f * L->soft), 0.0f, 1.0f); a = a * a * (3 - 2 * a);
        float b = Clamp((x - (L->t2 - L->soft)) / (2.0f * L->soft), 0.0f, 1.0f); b = b * b * (3 - 2 * b);
        Vector3 c = Vector3Lerp(Vector3Lerp(L->shade0, L->shade1, a), V3(1, 1, 1), b);
        r.ramp_px[i * 4 + 0] = (unsigned char)(Clamp(c.x, 0, 1) * 255.0f);
        r.ramp_px[i * 4 + 1] = (unsigned char)(Clamp(c.y, 0, 1) * 255.0f);
        r.ramp_px[i * 4 + 2] = (unsigned char)(Clamp(c.z, 0, 1) * 255.0f);
        r.ramp_px[i * 4 + 3] = 255;
    }
    UpdateTexture(r.ramp, r.ramp_px);
}

static void free_targets(void) {
    if (r.scene.id) UnloadRenderTexture(r.scene);
    if (r.bloom_a.id) UnloadRenderTexture(r.bloom_a);
    if (r.bloom_b.id) UnloadRenderTexture(r.bloom_b);
    r.scene = r.bloom_a = r.bloom_b = (RenderTexture2D){0};
}

static void alloc_targets(void) {
    const TierSettings *T = &TIERS[r.tier];
    r.rt_w = (int)((float)r.screen_w * T->render_scale); if (r.rt_w < 16) r.rt_w = 16;
    r.rt_h = (int)((float)r.screen_h * T->render_scale); if (r.rt_h < 16) r.rt_h = 16;
    free_targets();
    r.scene = LoadRenderTexture(r.rt_w, r.rt_h);
    SetTextureFilter(r.scene.texture, TEXTURE_FILTER_BILINEAR);
    if (T->bloom_passes > 0) {
        int bw = r.rt_w / T->bloom_div, bh = r.rt_h / T->bloom_div;
        if (bw < 8) bw = 8;
        if (bh < 8) bh = 8;
        r.bloom_a = LoadRenderTexture(bw, bh);
        r.bloom_b = LoadRenderTexture(bw, bh);
        SetTextureFilter(r.bloom_a.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(r.bloom_b.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(r.bloom_a.texture, TEXTURE_WRAP_CLAMP);
        SetTextureWrap(r.bloom_b.texture, TEXTURE_WRAP_CLAMP);
    }
    if (T->shadows && (r.shadow_rt.id == 0 || r.shadow_rt.texture.width != T->shadow_res)) {
        if (r.shadow_rt.id) UnloadRenderTexture(r.shadow_rt);
        r.shadow_rt = LoadRenderTexture(T->shadow_res, T->shadow_res);
        SetTextureFilter(r.shadow_rt.texture, TEXTURE_FILTER_POINT); /* packed depth must not be filtered */
        SetTextureWrap(r.shadow_rt.texture, TEXTURE_WRAP_CLAMP);
    }
}

static int loc(Shader s, const char *n) { return GetShaderLocation(s, n); }

static Shader load(const char *vs, const char *fs, const char *name) {
    Shader s = LoadShaderFromMemory(sh_compose(vs), sh_compose(fs));
    if (s.id == 0 || s.id == rlGetShaderIdDefault()) LOGE("shader '%s' failed to compile/link", name);
    return s;
}

static Material make_material(Shader sh);
static void load_shaders(void) {
    r.toon = load(SH_VS_TOON, SH_FS_TOON, "toon");
    r.outline = load(SH_VS_OUTLINE, SH_FS_OUTLINE, "outline");
    r.shadow = load(SH_VS_TOON, SH_FS_SHADOW, "shadow");
    const char *SK = "#define SKINNED\n";
    r.toon_sk = LoadShaderFromMemory(sh_compose_def(SH_VS_TOON, SK), sh_compose(SH_FS_TOON));
    r.outline_sk = LoadShaderFromMemory(sh_compose_def(SH_VS_OUTLINE, SK), sh_compose(SH_FS_OUTLINE));
    r.shadow_sk = LoadShaderFromMemory(sh_compose_def(SH_VS_TOON, SK), sh_compose(SH_FS_SHADOW));
    if (r.toon_sk.id == rlGetShaderIdDefault() || r.outline_sk.id == rlGetShaderIdDefault() || r.shadow_sk.id == rlGetShaderIdDefault())
        LOGE("skinned shader variants failed to compile/link");
    r.m_toon = make_material(r.toon); r.m_toon_sk = make_material(r.toon_sk);
    r.m_outline = make_material(r.outline); r.m_outline_sk = make_material(r.outline_sk);
    r.m_shadow = make_material(r.shadow); r.m_shadow_sk = make_material(r.shadow_sk);
    r.sky = load(SH_VS_QUAD, SH_FS_SKY, "sky");
    r.bright = load(SH_VS_QUAD, SH_FS_BRIGHT, "bright");
    r.blur = load(SH_VS_QUAD, SH_FS_BLUR, "blur");
    r.composite = load(SH_VS_QUAD, SH_FS_COMPOSITE, "composite");
}

static void setf(Shader s, const char *n, float v) { SetShaderValue(s, loc(s, n), &v, SHADER_UNIFORM_FLOAT); }
static void seti(Shader s, const char *n, int v) { SetShaderValue(s, loc(s, n), &v, SHADER_UNIFORM_INT); }
static void set2(Shader s, const char *n, float a, float b) { float v[2] = {a, b}; SetShaderValue(s, loc(s, n), v, SHADER_UNIFORM_VEC2); }
static void set3(Shader s, const char *n, Vector3 v) { SetShaderValue(s, loc(s, n), &v, SHADER_UNIFORM_VEC3); }

static void draw_fullscreen(Texture2D tex, int dst_w, int dst_h, bool flip) {
    Rectangle src = {0, 0, (float)tex.width, flip ? -(float)tex.height : (float)tex.height};
    DrawTexturePro(tex, src, (Rectangle){0, 0, (float)dst_w, (float)dst_h}, (Vector2){0, 0}, 0.0f, WHITE);
}

void renderer_init(int w, int h, QualityTier tier, int look) {
    memset(&r, 0, sizeof r);
    r.screen_w = w; r.screen_h = h;
    r.tier = tier; r.look = look < 0 || look >= LOOK_COUNT ? 0 : look; r.cur = LOOKS[r.look];
    load_shaders();
    Image white = GenImageColor(1, 1, WHITE);
    r.white = LoadTextureFromImage(white);
    UnloadImage(white);
    Image ramp = GenImageColor(256, 1, WHITE);
    r.ramp = LoadTextureFromImage(ramp);
    UnloadImage(ramp);
    SetTextureFilter(r.ramp, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(r.ramp, TEXTURE_WRAP_CLAMP);
    build_ramp();
    alloc_targets();
    r.ready = true;
    r.gov_ema = 16.7f;
    LOGI("renderer: tier %s, look '%s', target %dx%d", TIERS[r.tier].name, LOOKS[r.look].name, r.rt_w, r.rt_h);
}

void renderer_shutdown(void) {
    if (!r.ready) return;
    free_targets();
    if (r.shadow_rt.id) UnloadRenderTexture(r.shadow_rt);
    UnloadTexture(r.ramp); UnloadTexture(r.white);
    UnloadShader(r.toon); UnloadShader(r.outline); UnloadShader(r.shadow); UnloadShader(r.sky);
    UnloadShader(r.bright); UnloadShader(r.blur); UnloadShader(r.composite);
    UnloadShader(r.toon_sk); UnloadShader(r.outline_sk); UnloadShader(r.shadow_sk);
    Material *ms[] = {&r.m_toon, &r.m_toon_sk, &r.m_outline, &r.m_outline_sk, &r.m_shadow, &r.m_shadow_sk};
    for (int i = 0; i < 6; ++i) { MemFree(ms[i]->maps); ms[i]->maps = NULL; } /* textures are owned elsewhere */
    r.ready = false;
}

void renderer_set_tier(QualityTier t) {
    if (t < 0 || t >= TIER_COUNT || t == r.tier) return;
    r.tier = t;
    alloc_targets();
    LOGI("renderer: tier -> %s (%dx%d)", TIERS[t].name, r.rt_w, r.rt_h);
}
QualityTier renderer_tier(void) { return r.tier; }
const TierSettings *renderer_tier_settings(QualityTier t) { return &TIERS[t]; }
void renderer_set_look(int l) { if (l < 0 || l >= LOOK_COUNT) return; r.look = l; r.cur = LOOKS[l]; build_ramp(); }
void renderer_set_env(const Env *e) { if (e) { r.env = *e; r.env_on = true; } else r.env_on = false; }

static Vector3 vrgb(Rgb c) { return (Vector3){c.r, c.g, c.b}; }
/* The art look keeps its shading ramp, rim, outline, bloom and grade; the environment drives light, sky and fog. */
static void apply_env(Look *L, const Env *e) {
    static const Vector3 LIGHT_TINT[LOOK_COUNT] = {{0.92f, 0.96f, 1.08f}, {1.0f, 1.0f, 1.0f}, {1.08f, 0.96f, 0.92f}};
    Vector3 t = LIGHT_TINT[r.look];
    float lum = 0.299f * L->light_col.x + 0.587f * L->light_col.y + 0.114f * L->light_col.z;
    float gain = lum * 0.75f; gain = gain < 0.6f ? 0.6f : (gain > 0.95f ? 0.95f : gain);
    L->light_dir = (Vector3){e->light_dir[0], e->light_dir[1], e->light_dir[2]};
    L->light_col = (Vector3){e->light_col.r * t.x * gain, e->light_col.g * t.y * gain, e->light_col.b * t.z * gain};
    L->amb_top = vrgb(e->amb_top); L->amb_bot = vrgb(e->amb_bot);
    L->sky_top = vrgb(e->sky_top); L->sky_bot = vrgb(e->sky_bot); L->sky_glow = vrgb(e->sky_glow);
    L->fog_col = vrgb(e->fog_col); L->fog_den = e->fog_den;
    L->disc_x = e->disc_x; L->disc_y = e->disc_y; L->stars = e->stars;
    L->disc_col = e->is_day ? (Vector3){1.0f, 0.93f, 0.78f} : (Vector3){0.75f, 0.82f, 1.0f};
    L->disc_r = e->is_day ? 0.05f : 0.04f;
    L->exposure *= e->exposure; L->sat *= e->sat;
}
int renderer_look(void) { return r.look; }
const char *renderer_look_name(int l) { return LOOKS[l].name; }
void renderer_target_size(int *w, int *h) { *w = r.rt_w; *h = r.rt_h; }
int renderer_queue_count(void) { return r.ncmds; }

void renderer_begin_frame(Camera3D cam, float time_s) {
    r.cam = cam; r.time = time_s; r.ncmds = 0; r.bone_sets = 0;
    r.cur = LOOKS[r.look];
    if (r.env_on) apply_env(&r.cur, &r.env);
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (sw != r.screen_w || sh != r.screen_h) {
        r.screen_w = sw; r.screen_h = sh;
        alloc_targets();
    }
}

void renderer_submit(const Mesh *mesh, Matrix xf, RenderMat mat) {
    if (r.ncmds >= MAX_DRAWS) { r.dropped++; return; }
    r.cmds[r.ncmds++] = (DrawCmd){.mesh = mesh, .xf = xf, .mat = mat};
}

Matrix *renderer_alloc_bones(int count) {
    if (count > BONES_PER_SET || r.bone_sets >= MAX_BONE_SETS) return NULL;
    return &r.bone_arena[(r.bone_sets++) * BONES_PER_SET];
}

void renderer_submit_skinned(const Mesh *mesh, Matrix xf, RenderMat mat, const Matrix *bones, int bone_count) {
    if (r.ncmds >= MAX_DRAWS) { r.dropped++; return; }
    r.cmds[r.ncmds++] = (DrawCmd){.mesh = mesh, .xf = xf, .mat = mat, .bones = bones, .bone_count = bone_count};
}

static Material make_material(Shader sh) {
    Material m = LoadMaterialDefault();
    m.shader = sh;
    return m;
}

static void compute_light_vp(Camera3D *lc, float extent) {
    const Look *L = &r.cur;
    Vector3 ld = v3n(L->light_dir);
    Vector3 focus = r.cam.target;
    /* snap the focus point to shadow texels in light space to prevent shimmering */
    Vector3 right = v3n(Vector3CrossProduct(V3(0, 1, 0), ld));
    Vector3 up = Vector3CrossProduct(ld, right);
    float texel = (2.0f * extent) / (float)TIERS[r.tier].shadow_res;
    float fx = floorf(Vector3DotProduct(focus, right) / texel) * texel;
    float fy = floorf(Vector3DotProduct(focus, up) / texel) * texel;
    float fz = Vector3DotProduct(focus, ld);
    Vector3 snapped = Vector3Add(Vector3Add(Vector3Scale(right, fx), Vector3Scale(up, fy)), Vector3Scale(ld, fz));
    lc->target = snapped;
    lc->position = Vector3Add(snapped, Vector3Scale(ld, 40.0f));
    lc->up = V3(0, 1, 0);
    lc->projection = CAMERA_ORTHOGRAPHIC;
    lc->fovy = extent * 2.0f; /* raylib: ortho fovy = view height in world units */
}

/* Mesh to draw for a command: skinned commands carry their own bone matrices (shared meshes, per-instance poses). */
static Mesh cmd_mesh(const DrawCmd *c) {
    Mesh m = *c->mesh;
    if (c->bones) { m.boneMatrices = (Matrix *)c->bones; m.boneCount = c->bone_count; }
    return m;
}

static void run_shadow_pass(void) {
    Camera3D lc = {0};
    compute_light_vp(&lc, 14.0f);
    BeginTextureMode(r.shadow_rt);
    ClearBackground(WHITE); /* packed depth 1.0 => unshadowed */
    rlSetClipPlanes(1.0, 80.0);
    BeginMode3D(lc);
    r.light_vp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlDisableColorBlend();
    rlSetCullFace(RL_CULL_FACE_FRONT);
    for (int i = 0; i < r.ncmds; ++i) {
        const DrawCmd *c = &r.cmds[i];
        if (c->mat.flags & RM_NO_SHADOW_CAST) continue;
        DrawMesh(cmd_mesh(c), c->bones ? r.m_shadow_sk : r.m_shadow, c->xf);
    }
    rlDrawRenderBatchActive();
    rlSetCullFace(RL_CULL_FACE_BACK);
    rlEnableColorBlend();
    EndMode3D();
    rlSetClipPlanes(0.05, 600.0);
    EndTextureMode();
}

static void set_toon_uniforms(Shader s) {
    const Look *L = &r.cur;
    const TierSettings *T = &TIERS[r.tier];
    set3(s, "uLightDir", v3n(L->light_dir));
    set3(s, "uLightCol", L->light_col);
    set3(s, "uAmbTop", L->amb_top);
    set3(s, "uAmbBot", L->amb_bot);
    setf(s, "uShadowTexel", 1.0f / (float)T->shadow_res);
    seti(s, "uPcf", T->pcf_radius);
    setf(s, "uShadowBias", L->shadow_bias);
    setf(s, "uShadowStr", T->shadows ? L->shadow_str : 0.0f);
    set3(s, "uRimCol", L->rim_col);
    setf(s, "uRimPow", L->rim_pow);
    setf(s, "uRimStr", L->rim_str);
    set2(s, "uSpec", L->spec_str, L->spec_size);
    set3(s, "uFogCol", L->fog_col);
    setf(s, "uFogDen", L->fog_den);
    SetShaderValueMatrix(s, loc(s, "uLightVP"), r.light_vp);
    float cp[3] = {r.cam.position.x, r.cam.position.y, r.cam.position.z};
    SetShaderValue(s, loc(s, "viewPos"), cp, SHADER_UNIFORM_VEC3);
    seti(s, "uShadow", 10);
    seti(s, "uRamp", 11);
}

static void draw_sky(void) {
    const Look *L = &r.cur;
    Shader s = r.sky;
    set3(s, "uTop", L->sky_top); set3(s, "uBottom", L->sky_bot); set3(s, "uGlow", L->sky_glow);
    float d[4] = {L->disc_x, L->disc_y, L->disc_r, L->disc_halo};
    SetShaderValue(s, loc(s, "uDisc"), d, SHADER_UNIFORM_VEC4);
    set3(s, "uDiscCol", L->disc_col);
    setf(s, "uStars", L->stars);
    Vector3 f = Vector3Subtract(r.cam.target, r.cam.position);
    setf(s, "uYaw", atan2f(f.x, f.z));
    float el = asinf(f.y / (Vector3Length(f) + 1e-6f));
    setf(s, "uHorizon", 0.5f - tanf(el) / (2.0f * tanf(r.cam.fovy * DEG2RAD * 0.5f)));
    set2(s, "uRes", (float)r.rt_w, (float)r.rt_h);
    BeginShaderMode(s);
    DrawTexturePro(r.white, (Rectangle){0, 0, 1, 1}, (Rectangle){0, 0, (float)r.rt_w, (float)r.rt_h}, (Vector2){0, 0}, 0, WHITE);
    EndShaderMode();
    rlDrawRenderBatchActive();
}

static void set_cmd_uniforms(Shader s, const DrawCmd *c) {
    int fl = 0;
    if (c->mat.flags & RM_GROUND) fl |= 1;
    if (c->mat.flags & RM_HAIR) fl |= 2;
    if (c->mat.flags & RM_NO_RECEIVE) fl |= 4;
    if (c->mat.flags & RM_PAINT) fl |= 8;
    float um[4] = {c->mat.rim, c->mat.spec, c->mat.emissive, (float)fl};
    SetShaderValue(s, loc(s, "uMat"), um, SHADER_UNIFORM_VEC4);
    setf(s, "uFlash", c->mat.flash);
    setf(s, "uSkinDetail", c->mat.skin_detail);
    if (c->mat.flags & RM_PAINT) {
        Vector3 p1 = {c->mat.paint[0].r / 255.0f, c->mat.paint[0].g / 255.0f, c->mat.paint[0].b / 255.0f};
        Vector3 p2 = {c->mat.paint[1].r / 255.0f, c->mat.paint[1].g / 255.0f, c->mat.paint[1].b / 255.0f};
        Vector3 p3 = {c->mat.paint[2].r / 255.0f, c->mat.paint[2].g / 255.0f, c->mat.paint[2].b / 255.0f};
        set3(s, "uPaint1", p1); set3(s, "uPaint2", p2); set3(s, "uPaint3", p3);
        SetShaderValue(s, loc(s, "uPaintCfg"), c->mat.paint_cfg, SHADER_UNIFORM_VEC4);
    }
}

static void run_scene_pass(void) {
    const Look *L = &r.cur;
    const TierSettings *T = &TIERS[r.tier];
    BeginTextureMode(r.scene);
    ClearBackground(BLACK);
    rlDisableColorBlend();
    draw_sky();
    rlSetClipPlanes(0.1, 300.0);
    BeginMode3D(r.cam);
    set_toon_uniforms(r.toon);
    set_toon_uniforms(r.toon_sk);
    rlActiveTextureSlot(10); rlEnableTexture(r.shadow_rt.texture.id);
    rlActiveTextureSlot(11); rlEnableTexture(r.ramp.id);
    rlActiveTextureSlot(0);
    for (int i = 0; i < r.ncmds; ++i) {
        const DrawCmd *c = &r.cmds[i];
        Material *m = c->bones ? &r.m_toon_sk : &r.m_toon;
        set_cmd_uniforms(m->shader, c);
        m->maps[MATERIAL_MAP_DIFFUSE].color = c->mat.base;
        m->maps[MATERIAL_MAP_DIFFUSE].texture = c->mat.tex.id ? c->mat.tex : r.white;
        DrawMesh(cmd_mesh(c), *m, c->xf);
    }
    if (T->outlines) {
        float px = L->outline_px * (float)r.rt_h / 720.0f;
        float cp[3] = {r.cam.position.x, r.cam.position.y, r.cam.position.z};
        for (int k = 0; k < 2; ++k) {
            Shader os = k ? r.outline_sk : r.outline;
            set3(os, "uOutlineCol", L->outline_col);
            SetShaderValue(os, loc(os, "viewPos"), cp, SHADER_UNIFORM_VEC3);
            setf(os, "uOutlinePx", px);
        }
        rlSetCullFace(RL_CULL_FACE_FRONT);
        for (int i = 0; i < r.ncmds; ++i) {
            const DrawCmd *c = &r.cmds[i];
            if (c->mat.flags & (RM_NO_OUTLINE | RM_GROUND)) continue;
            Material *om = c->bones ? &r.m_outline_sk : &r.m_outline;
            setf(om->shader, "uSmooth", c->mat.smooth);
            om->maps[MATERIAL_MAP_DIFFUSE].color = c->mat.base;
            DrawMesh(cmd_mesh(c), *om, c->xf);
        }
        rlSetCullFace(RL_CULL_FACE_BACK);
    }
    rlDrawRenderBatchActive();
    EndMode3D();
    rlEnableColorBlend();
    EndTextureMode();
}

static void run_bloom(void) {
    const Look *L = &r.cur;
    const TierSettings *T = &TIERS[r.tier];
    if (T->bloom_passes <= 0 || !r.bloom_a.id) return;
    int bw = r.bloom_a.texture.width, bh = r.bloom_a.texture.height;
    BeginTextureMode(r.bloom_a);
    ClearBackground(BLACK);
    BeginShaderMode(r.bright);
    setf(r.bright, "uThr", L->bloom_thr);
    draw_fullscreen(r.scene.texture, bw, bh, true);
    EndShaderMode();
    EndTextureMode();
    for (int i = 0; i < T->bloom_passes; ++i) {
        BeginTextureMode(r.bloom_b);
        BeginShaderMode(r.blur);
        set2(r.blur, "uDir", 1.0f / (float)bw, 0.0f);
        draw_fullscreen(r.bloom_a.texture, bw, bh, true);
        EndShaderMode();
        EndTextureMode();
        BeginTextureMode(r.bloom_a);
        BeginShaderMode(r.blur);
        set2(r.blur, "uDir", 0.0f, 1.0f / (float)bh);
        draw_fullscreen(r.bloom_b.texture, bw, bh, true);
        EndShaderMode();
        EndTextureMode();
    }
}

static void run_composite(void) {
    const Look *L = &r.cur;
    const TierSettings *T = &TIERS[r.tier];
    Shader s = r.composite;
    BeginShaderMode(s);
    set2(s, "uTexel", 1.0f / (float)r.rt_w, 1.0f / (float)r.rt_h);
    setf(s, "uFxaa", T->fxaa ? 1.0f : 0.0f);
    setf(s, "uBloomStr", (T->bloom_passes > 0 && r.bloom_a.id) ? L->bloom_str : 0.0f);
    set3(s, "uBloomTint", L->bloom_tint);
    setf(s, "uExposure", L->exposure);
    setf(s, "uSat", L->sat);
    setf(s, "uContrast", L->contrast);
    set3(s, "uShadowTint", L->grade_shadow);
    set3(s, "uHighTint", L->grade_high);
    setf(s, "uVignette", L->vignette);
    Texture2D bloom = r.bloom_a.id ? r.bloom_a.texture : r.white;
    SetShaderValueTexture(s, loc(s, "uBloom"), bloom);
    draw_fullscreen(r.scene.texture, r.screen_w, r.screen_h, true);
    EndShaderMode();
}

void renderer_end_frame(void) {
    if (!r.ready) return;
    if (TIERS[r.tier].shadows) run_shadow_pass();
    run_scene_pass();
    run_bloom();
    run_composite();
}

/* ---- quality governor ----
 * Phase 1 (benchmark): after a short settle, measure ~60 frames at the starting tier and move
 * one tier per measurement until the average is under budget (or we hit MOBILE).
 * Phase 2: if the frame EMA stays above ~22 ms for 3 s, drop one tier. Never raises quality by itself. */
void renderer_governor_update(float dt, bool enabled) {
    if (!r.ready || !enabled) return;
    float ms = dt * 1000.0f;
    if (ms > 250.0f) return; /* ignore stalls (tab switch, shader compile) */
    r.gov_ema += (ms - r.gov_ema) * 0.05f;
    r.gov_frames++;
    if (!r.gov_settled) {
        const int warm = 30, window = 60;
        static float sum; static int n;
        if (r.gov_frames <= warm) { sum = 0; n = 0; return; }
        sum += ms; n++;
        if (n >= window) {
            float avg = sum / (float)n;
            sum = 0; n = 0; r.gov_frames = 0;
            if (avg > 19.0f && r.tier < TIER_MOBILE) { renderer_set_tier(r.tier + 1); r.gov_step++; }
            else { r.gov_settled = true; LOGI("governor settled on %s (%.1f ms avg)", TIERS[r.tier].name, avg); }
        }
        return;
    }
    if (r.gov_ema > 22.0f && r.tier < TIER_MOBILE) {
        r.gov_slow_time += dt;
        if (r.gov_slow_time > 3.0f) { renderer_set_tier(r.tier + 1); r.gov_slow_time = 0; r.gov_ema = 16.7f; }
    } else r.gov_slow_time = 0;
}
bool renderer_governor_settled(void) { return r.gov_settled; }
