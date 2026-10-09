#include "testutil.h"
#include <string.h>
#include "core/daynight.h"

static float lum(Rgb c) { return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b; }

int main(void) {
    Env e, e2; Weather w; weather_init(&w, 1);
    /* sun is up at noon, down at midnight; light is brighter by day */
    env_compute(&e, 12.0f, &w, NULL); CHECK(e.is_day); CHECK(e.light_dir[1] > 0.9f);
    env_compute(&e2, 0.0f, &w, NULL); CHECK(!e2.is_day);
    CHECK(lum(e.light_col) > lum(e2.light_col)); CHECK(lum(e.sky_top) > lum(e2.sky_top) * 2.0f);
    CHECK(e2.stars > 0.9f && e.stars < 0.01f);
    /* light direction is a unit vector and always above the horizon; sky evolves continuously */
    Env prev; env_compute(&prev, 0.0f, &w, NULL);
    for (int i = 1; i <= 24 * 20; ++i) {
        float h = (float)i / 20.0f; env_compute(&e, h, &w, NULL);
        float l = sqrtf(e.light_dir[0] * e.light_dir[0] + e.light_dir[1] * e.light_dir[1] + e.light_dir[2] * e.light_dir[2]);
        CHECK_NEAR(l, 1.0, 1e-3); CHECK(e.light_dir[1] > 0.05f);
        float dl = fabsf(lum(e.sky_bot) - lum(prev.sky_bot)); CHECK(dl < 0.12f);
        prev = e;
    }
    env_compute(&e, 24.0f, &w, NULL); env_compute(&e2, 0.0f, &w, NULL); CHECK_NEAR(e.sky_top.b, e2.sky_top.b, 1e-4);   /* wraps */
    env_compute(&e, 30.0f, &w, NULL); env_compute(&e2, 6.0f, &w, NULL); CHECK_NEAR(e.sky_top.b, e2.sky_top.b, 1e-4);
    /* dawn is warm (red > blue on the horizon), dusk too */
    env_compute(&e, 6.5f, &w, NULL); CHECK(e.sky_bot.r > e.sky_bot.b);

    /* weather dims, fogs and hides stars */
    Env clear, storm;
    weather_set(&w, WX_CLEAR, 0); env_compute(&clear, 13.0f, &w, NULL);
    weather_set(&w, WX_STORM, 1.0f); env_compute(&storm, 13.0f, &w, NULL);
    CHECK(lum(storm.light_col) < lum(clear.light_col) * 0.6f); CHECK(storm.fog_den > clear.fog_den); CHECK(storm.sat < 1.0f);
    CHECK(weather_visibility(&w) < 0.8f); CHECK(weather_precipitating(&w));
    weather_set(&w, WX_CLEAR, 0); CHECK_NEAR(weather_visibility(&w), 1.0, 1e-6); CHECK(!weather_precipitating(&w));
    weather_set(&w, WX_CLOUDY, 1.0f); CHECK(!weather_precipitating(&w));
    weather_set(&w, WX_SANDSTORM, 1.0f); env_compute(&storm, 13.0f, &w, NULL); CHECK(storm.fog_den > clear.fog_den + 0.02f);
    weather_set(&w, WX_CLEAR, 0);
    env_compute(&e, 0.0f, &w, NULL); weather_set(&w, WX_STORM, 1.0f); env_compute(&e2, 0.0f, &w, NULL); CHECK(e2.stars < e.stars * 0.5f);

    /* biome tint */
    BiomeTint t = {{1.2f, 0.8f, 0.8f}, {1, 1, 1}, 0.01f};
    weather_set(&w, WX_CLEAR, 0); env_compute(&clear, 12.0f, &w, NULL); env_compute(&e, 12.0f, &w, &t);
    CHECK(e.sky_top.r > clear.sky_top.r); CHECK(e.sky_top.g < clear.sky_top.g); CHECK_NEAR(e.fog_den - clear.fog_den, 0.01, 1e-5);

    /* weather state machine: only region-allowed kinds appear, changes ramp (no popping), lightning only in storms */
    float snow_only[WX_COUNT] = {0}; snow_only[WX_CLEAR] = 1; snow_only[WX_SNOW] = 2;
    weather_init(&w, 99);
    bool saw_snow = false, bad = false; float last = 0;
    for (int i = 0; i < 60 * 60 * 6; ++i) {
        weather_update(&w, 1.0f / 10.0f, snow_only);
        if (w.kind == WX_SNOW && w.intensity > 0.5f) saw_snow = true;
        if (w.kind != WX_CLEAR && w.kind != WX_SNOW) bad = true;
        if (fabsf(w.intensity - last) > 0.05f) bad = true;
        last = w.intensity;
    }
    CHECK(saw_snow); CHECK(!bad);
    float none[WX_COUNT] = {0};
    weather_init(&w, 5); weather_set(&w, WX_RAIN, 1.0f);
    for (int i = 0; i < 2000; ++i) weather_update(&w, 0.1f, none);
    CHECK(w.kind == WX_CLEAR || w.intensity == 0.0f);        /* a kind the region forbids fades away */
    CHECK_NEAR(weather_visibility(&w), 1.0, 1e-6);
    /* determinism */
    Weather a, b; float mix[WX_COUNT] = {0.4f, 0.2f, 0.2f, 0.1f, 0, 0, 0, 0.1f};
    weather_init(&a, 42); weather_init(&b, 42);
    for (int i = 0; i < 5000; ++i) { weather_update(&a, 0.2f, mix); weather_update(&b, 0.2f, mix); }
    CHECK(a.kind == b.kind && a.intensity == b.intensity);
    /* storms strike */
    weather_init(&w, 3); weather_set(&w, WX_STORM, 1.0f);
    int flashes = 0; float prevf = 0;
    float storm_only[WX_COUNT] = {0}; storm_only[WX_STORM] = 1;
    for (int i = 0; i < 3000; ++i) { weather_update(&w, 0.1f, storm_only); if (w.flash_t > 0.95f && prevf <= 0.95f) ++flashes; prevf = w.flash_t; }
    CHECK(flashes >= 5);
    CHECK(strcmp(weather_name(WX_SNOW), "snow") == 0);
    bool ok; CHECK(weather_from_name("ash", &ok) == WX_ASH && ok); weather_from_name("zzz", &ok); CHECK(!ok);
    TEST_DONE();
}
