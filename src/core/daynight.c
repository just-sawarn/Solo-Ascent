#include "core/daynight.h"
#include <math.h>
#include <string.h>
#include "core/vec.h"

static const char *NAMES[WX_COUNT] = {"clear", "cloudy", "rain", "storm", "snow", "sandstorm", "ash", "fog"};
const char *weather_name(WeatherKind k) { return k >= 0 && k < WX_COUNT ? NAMES[k] : "clear"; }
WeatherKind weather_from_name(const char *s, bool *ok) {
    for (int i = 0; i < WX_COUNT; ++i) if (!strcmp(NAMES[i], s)) { if (ok) *ok = true; return (WeatherKind)i; }
    if (ok) *ok = false;
    return WX_CLEAR;
}

/* ---- keyframes over the 24 h day ---- */
typedef struct { float hour; Rgb top, bot, glow, sun, amb_top, amb_bot, fog; float fog_den, stars, exposure; } Key;
#define C3(r, g, b) {r, g, b}
static const Key KEYS[] = {
    {0.0f,  C3(.010f,.015f,.060f), C3(.060f,.050f,.180f), C3(.10f,.07f,.28f), C3(.40f,.52f,.95f), C3(.050f,.070f,.180f), C3(.015f,.015f,.050f), C3(.040f,.045f,.120f), 0.014f, 1.0f, 1.05f},
    {5.0f,  C3(.030f,.040f,.160f), C3(.350f,.200f,.350f), C3(.50f,.25f,.30f), C3(.60f,.50f,.70f), C3(.080f,.090f,.200f), C3(.030f,.025f,.070f), C3(.200f,.140f,.250f), 0.014f, 0.5f, 1.05f},
    {6.5f,  C3(.200f,.330f,.700f), C3(.950f,.550f,.350f), C3(.60f,.28f,.12f), C3(1.30f,.90f,.60f), C3(.150f,.170f,.300f), C3(.070f,.050f,.080f), C3(.600f,.420f,.380f), 0.013f, 0.0f, 1.00f},
    {9.0f,  C3(.180f,.400f,.850f), C3(.650f,.820f,1.00f), C3(.20f,.15f,.06f), C3(1.25f,1.10f,.90f), C3(.220f,.260f,.420f), C3(.100f,.090f,.110f), C3(.620f,.780f,.950f), 0.010f, 0.0f, 1.00f},
    {13.0f, C3(.160f,.380f,.880f), C3(.600f,.800f,1.00f), C3(.15f,.12f,.05f), C3(1.35f,1.25f,1.05f), C3(.240f,.280f,.440f), C3(.110f,.100f,.120f), C3(.600f,.770f,.950f), 0.009f, 0.0f, 1.00f},
    {17.0f, C3(.190f,.340f,.760f), C3(.900f,.700f,.520f), C3(.40f,.22f,.10f), C3(1.30f,.95f,.65f), C3(.200f,.220f,.380f), C3(.090f,.070f,.090f), C3(.750f,.600f,.520f), 0.012f, 0.0f, 1.00f},
    {18.8f, C3(.030f,.050f,.220f), C3(.700f,.280f,.150f), C3(.28f,.06f,.14f), C3(.95f,.45f,.30f), C3(.100f,.090f,.220f), C3(.040f,.025f,.060f), C3(.350f,.200f,.300f), 0.015f, 0.2f, 1.05f},
    {20.5f, C3(.012f,.020f,.090f), C3(.100f,.060f,.230f), C3(.14f,.06f,.30f), C3(.45f,.56f,1.00f), C3(.060f,.080f,.200f), C3(.020f,.018f,.060f), C3(.050f,.050f,.140f), 0.015f, 1.0f, 1.05f},
    {24.0f, C3(.010f,.015f,.060f), C3(.060f,.050f,.180f), C3(.10f,.07f,.28f), C3(.40f,.52f,.95f), C3(.050f,.070f,.180f), C3(.015f,.015f,.050f), C3(.040f,.045f,.120f), 0.014f, 1.0f, 1.05f},
};
#define NKEYS ((int)(sizeof KEYS / sizeof KEYS[0]))

static Rgb mix3(Rgb a, Rgb b, float t) { return (Rgb){a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }
static Rgb mul3(Rgb a, Rgb b) { return (Rgb){a.r * b.r, a.g * b.g, a.b * b.b}; }
static Rgb scale3(Rgb a, float s) { return (Rgb){a.r * s, a.g * s, a.b * s}; }
static float luma(Rgb c) { return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b; }
static Rgb grey_toward(Rgb c, float t) { float l = luma(c); return mix3(c, (Rgb){l, l, l}, t); }

void weather_init(Weather *w, uint32_t seed) {
    memset(w, 0, sizeof *w);
    w->kind = w->next = WX_CLEAR;
    rng_seed(&w->rng, seed);
    w->timer = 60.0f;
}

void weather_set(Weather *w, WeatherKind k, float intensity) {
    w->kind = w->next = k; w->intensity = clampf(intensity, 0.0f, 1.0f); w->timer = 120.0f;
}

void weather_update(Weather *w, float dt, const float weights[WX_COUNT]) {
    /* ramp: fade the current kind out, switch, fade the next one in */
    if (w->next != w->kind) {
        w->intensity -= dt / 8.0f;
        if (w->intensity <= 0.0f) { w->intensity = 0.0f; w->kind = w->next; }
    } else if (w->kind != WX_CLEAR && w->intensity < 1.0f) {
        w->intensity = fminf(1.0f, w->intensity + dt / 10.0f);
    }
    w->timer -= dt;
    if (w->timer <= 0.0f) {
        float total = 0;
        for (int i = 0; i < WX_COUNT; ++i) total += weights[i];
        if (total <= 0.0f) { w->next = WX_CLEAR; }
        else {
            float r = rng_float(&w->rng) * total;
            for (int i = 0; i < WX_COUNT; ++i) { if (r < weights[i]) { w->next = (WeatherKind)i; break; } r -= weights[i]; }
        }
        w->timer = rng_range(&w->rng, 90.0f, 240.0f);
    }
    /* a kind the region doesn't allow (travelled mid-weather) is dropped */
    if (w->kind != WX_CLEAR && weights[w->kind] <= 0.0f) w->next = WX_CLEAR;
    if (w->kind == WX_CLEAR && w->next == WX_CLEAR) w->intensity = 0.0f;
    if (w->kind == WX_CLEAR && w->next != WX_CLEAR) { w->kind = w->next; w->intensity = 0.0f; }
    /* lightning */
    w->flash_t = fmaxf(0.0f, w->flash_t - dt * 3.0f);
    if (w->kind == WX_STORM && w->intensity > 0.5f) {
        w->strike_timer -= dt;
        if (w->strike_timer <= 0.0f) { w->flash_t = 1.0f; w->strike_timer = rng_range(&w->rng, 4.0f, 14.0f); }
    }
}

float weather_visibility(const Weather *w) {
    float k = 0.0f;
    switch (w->kind) {
        case WX_FOG: k = 0.55f; break; case WX_SANDSTORM: k = 0.6f; break; case WX_SNOW: k = 0.3f; break;
        case WX_STORM: k = 0.35f; break; case WX_RAIN: k = 0.2f; break; case WX_ASH: k = 0.25f; break; default: k = 0.0f;
    }
    return 1.0f - k * w->intensity;
}
bool weather_precipitating(const Weather *w) { return (w->kind == WX_RAIN || w->kind == WX_STORM || w->kind == WX_SNOW) && w->intensity > 0.05f; }

void env_compute(Env *e, float hour, const Weather *w, const BiomeTint *tint) {
    hour = fmodf(fmodf(hour, 24.0f) + 24.0f, 24.0f);
    int i = 0;
    while (i < NKEYS - 2 && hour >= KEYS[i + 1].hour) ++i;
    const Key *a = &KEYS[i], *b = &KEYS[i + 1];
    float t = clampf((hour - a->hour) / (b->hour - a->hour), 0.0f, 1.0f);
    t = t * t * (3.0f - 2.0f * t);
    e->sky_top = mix3(a->top, b->top, t); e->sky_bot = mix3(a->bot, b->bot, t); e->sky_glow = mix3(a->glow, b->glow, t);
    e->light_col = mix3(a->sun, b->sun, t); e->amb_top = mix3(a->amb_top, b->amb_top, t); e->amb_bot = mix3(a->amb_bot, b->amb_bot, t);
    e->fog_col = mix3(a->fog, b->fog, t);
    e->fog_den = a->fog_den + (b->fog_den - a->fog_den) * t;
    e->stars = a->stars + (b->stars - a->stars) * t;
    e->exposure = a->exposure + (b->exposure - a->exposure) * t;
    e->sat = 1.0f; e->flash = 0.0f;

    /* sun by day, moon by night: elevation follows a sine over the lit half */
    e->is_day = hour >= 6.0f && hour < 18.5f;
    float u = e->is_day ? (hour - 6.0f) / 12.5f : fmodf(hour - 18.5f + 24.0f, 24.0f) / 11.5f;   /* 0..1 across the arc */
    float elev = fmaxf(0.10f, sinf(u * 3.14159265f));
    float az = (e->is_day ? -2.2f : -0.9f) + u * 3.14159265f;
    float ce = sqrtf(1.0f - elev * elev);
    e->light_dir[0] = cosf(az) * ce; e->light_dir[1] = elev; e->light_dir[2] = sinf(az) * ce;
    /* disc position on the sky backdrop (x across, y up) follows the same arc */
    e->disc_x = 0.15f + 0.7f * u; e->disc_y = 0.15f + 0.65f * sinf(u * 3.14159265f);
    if (!e->is_day) e->light_col = scale3(e->light_col, 0.8f);

    if (tint) {
        e->sky_top = mul3(e->sky_top, tint->sky_tint); e->sky_bot = mul3(e->sky_bot, tint->sky_tint);
        e->fog_col = mul3(e->fog_col, tint->fog_tint); e->fog_den += tint->fog_add;
    }

    /* weather: overcast greys the light and sky, thickens fog, hides stars */
    float k = w ? w->intensity : 0.0f;
    if (w && k > 0.0f) {
        float grey = 0.0f, dim = 0.0f, fog = 0.0f; Rgb fogtint = e->fog_col;
        switch (w->kind) {
            case WX_CLOUDY: grey = 0.45f; dim = 0.25f; fog = 0.002f; break;
            case WX_RAIN:   grey = 0.65f; dim = 0.40f; fog = 0.006f; break;
            case WX_STORM:  grey = 0.80f; dim = 0.65f; fog = 0.010f; break;
            case WX_SNOW:   grey = 0.55f; dim = 0.25f; fog = 0.012f; fogtint = (Rgb){0.80f, 0.85f, 0.95f}; break;
            case WX_SANDSTORM: grey = 0.30f; dim = 0.35f; fog = 0.030f; fogtint = (Rgb){0.80f, 0.60f, 0.35f}; break;
            case WX_ASH:    grey = 0.60f; dim = 0.45f; fog = 0.014f; fogtint = (Rgb){0.22f, 0.15f, 0.14f}; break;
            case WX_FOG:    grey = 0.40f; dim = 0.20f; fog = 0.035f; fogtint = mix3(e->fog_col, (Rgb){0.72f, 0.75f, 0.80f}, 0.7f); break;
            default: break;
        }
        float lum_day = e->is_day ? 0.8f : 0.25f;
        e->sky_top = mix3(e->sky_top, grey_toward(e->sky_top, 1.0f), grey * k);
        e->sky_bot = mix3(e->sky_bot, grey_toward(e->sky_bot, 1.0f), grey * k);
        e->sky_top = scale3(e->sky_top, 1.0f - 0.45f * dim * k); e->sky_bot = scale3(e->sky_bot, 1.0f - 0.30f * dim * k);
        e->light_col = scale3(grey_toward(e->light_col, grey * k), 1.0f - dim * k * 0.8f);
        e->fog_col = mix3(e->fog_col, scale3(fogtint, lum_day), 0.6f * k);
        e->fog_den += fog * k;
        e->stars *= 1.0f - fminf(1.0f, grey * k * 1.4f);
        e->sat = 1.0f - 0.25f * grey * k;
        e->sky_glow = scale3(e->sky_glow, 1.0f - 0.8f * grey * k);
        if (w->kind == WX_STORM) e->flash = w->flash_t;
    }
    if (e->flash > 0.0f) { /* lightning: brief white-blue wash */
        Rgb fl = {0.8f, 0.85f, 1.0f};
        e->sky_top = mix3(e->sky_top, fl, e->flash * 0.6f); e->sky_bot = mix3(e->sky_bot, fl, e->flash * 0.7f);
        e->amb_top = mix3(e->amb_top, fl, e->flash * 0.5f);
    }
}
