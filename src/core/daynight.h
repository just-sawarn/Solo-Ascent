#ifndef SA_DAYNIGHT_H
#define SA_DAYNIGHT_H
/*
 * Time of day + weather -> a lighting/sky environment. Pure functions of (hour, weather, biome), no engine calls.
 * The renderer applies an Env on top of the selected art look; the game owns the clock and the weather state.
 */
#include <stdbool.h>
#include <stdint.h>
#include "core/noise.h"

typedef struct { float r, g, b; } Rgb;

typedef enum { WX_CLEAR, WX_CLOUDY, WX_RAIN, WX_STORM, WX_SNOW, WX_SANDSTORM, WX_ASH, WX_FOG, WX_COUNT } WeatherKind;
const char *weather_name(WeatherKind k);
WeatherKind weather_from_name(const char *s, bool *ok);

typedef struct {
    float light_dir[3];      /* direction TO the light (sun by day, moon by night) */
    Rgb   light_col, amb_top, amb_bot;
    Rgb   sky_top, sky_bot, sky_glow, fog_col;
    float fog_den;
    float stars;             /* 0..1 */
    float disc_x, disc_y;    /* sun/moon disc position on the sky, 0..1 screen-space like the looks */
    float sat;               /* colour saturation multiplier (overcast washes out) */
    float exposure;          /* multiplier */
    bool  is_day;
    float flash;             /* lightning flash 0..1 */
} Env;

/* biome tint: multiplies sky/fog toward the region's identity */
typedef struct { Rgb sky_tint, fog_tint; float fog_add; } BiomeTint;

typedef struct {
    WeatherKind kind, next;
    float intensity;         /* 0..1 of the current kind (ramps) */
    float timer;             /* seconds until a new pick */
    float flash_t;           /* storm lightning */
    float strike_timer;
    Rng   rng;
} Weather;

void weather_init(Weather *w, uint32_t seed);
/* weights: probability weight per WeatherKind for the current region (all zero => always clear) */
void weather_update(Weather *w, float dt, const float weights[WX_COUNT]);
void weather_set(Weather *w, WeatherKind k, float intensity);   /* force (dev, cutscenes) */
float weather_visibility(const Weather *w);                    /* 1 = clear, lower = reduced sight (aggro/fog) */
bool  weather_precipitating(const Weather *w);                 /* rain / storm / snow */

void env_compute(Env *e, float hour, const Weather *w, const BiomeTint *tint);
/* length of a game day in real seconds */
#define DAY_SECONDS 1440.0f

#endif
