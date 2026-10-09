#include "ui/ending.h"
#include <raylib.h>
#include <stdio.h>

void ending_start(EndingUI *e, bool true_ending, int ng_plus, int kills, float play_seconds) {
    *e = (EndingUI){.t = 0, .true_ending = true_ending, .ng_plus = ng_plus, .kills = kills, .play_seconds = play_seconds};
}

static const char *EPILOGUE_STD[] = {
    "Vorathis has fallen. The Demon Lord is no more.",
    "The party returns to Aurelheim as heroes.",
    "",
    "Aurel's spirit passes on:",
    "\"You did not carry it alone. That was the part I never learned.\"",
};
static const char *EPILOGUE_TRUE[] = {
    "Vorathis has fallen. The Demon Lord is no more.",
    "Aurel's spirit is fully freed, and blesses the party.",
    "Mirelle regains her full lifespan. Lyra opens a new Academy wing.",
    "Bram rebuilds the Shield Guard. Darius becomes the Guildmaster's apprentice.",
    "Ashbrook is rebuilt, with Orwyn's spirit watching from the shrine.",
    "",
    "\"You did not carry it alone. That was the part I never learned.\"",
};
static const char *CREDITS[] = {
    "SOLO ASCENT: The S-Rank Hero", "",
    "Design, story and code", "Claude Code, with the player",  "",
    "Characters, animations and weapons", "Quaternius (CC0): Universal Base Characters,",
    "Universal Animation Library, Ultimate Weapons", "",
    "Engine", "raylib, cJSON, Emscripten", "",
    "Thank you for playing.", "",
    "You are not alone.",
};

int ending_update_draw(EndingUI *e, float dt) {
    e->t += dt;
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float s = sh / 720.0f;
    DrawRectangle(0, 0, sw, sh, (Color){6, 8, 18, (unsigned char)(e->t < 1.5f ? 255.0f * e->t / 1.5f : 245)});
    const char **ep = e->true_ending ? EPILOGUE_TRUE : EPILOGUE_STD;
    int nep = e->true_ending ? (int)(sizeof EPILOGUE_TRUE / sizeof EPILOGUE_TRUE[0]) : (int)(sizeof EPILOGUE_STD / sizeof EPILOGUE_STD[0]);
    const char *head = e->true_ending ? "TRUE ENDING" : "THE END";
    int fs = (int)(52 * s);
    if (e->t > 0.8f) DrawText(head, (sw - MeasureText(head, fs)) / 2, (int)(60 * s), fs, (Color){255, 226, 140, 255});
    for (int i = 0; i < nep; ++i) {
        float a = e->t - 2.0f - (float)i * 1.4f;
        if (a <= 0) continue;
        unsigned char al = (unsigned char)(255.0f * (a > 1.0f ? 1.0f : a));
        int f2 = (int)(22 * s);
        DrawText(ep[i], (sw - MeasureText(ep[i], f2)) / 2, (int)(150 * s) + i * (int)(34 * s), f2, (Color){235, 238, 255, al});
    }
    /* credits scroll up the screen after the epilogue */
    float start = 2.0f + (float)nep * 1.4f + 2.0f;
    if (e->t > start) {
        float y = (float)sh - (e->t - start) * 42.0f * s;
        int n = (int)(sizeof CREDITS / sizeof CREDITS[0]);
        for (int i = 0; i < n; ++i) { int f3 = (int)(20 * s); DrawText(CREDITS[i], (sw - MeasureText(CREDITS[i], f3)) / 2, (int)(y + (float)i * 30 * s), f3, (Color){200, 210, 240, 255}); }
    }
    char st[96]; snprintf(st, sizeof st, "Time played %d:%02d   Foes defeated %d%s", (int)e->play_seconds / 3600, ((int)e->play_seconds / 60) % 60, e->kills, e->ng_plus ? "   (New Game+)" : "");
    DrawText(st, 14, sh - (int)(60 * s), (int)(16 * s), (Color){170, 180, 210, 230});
    if (e->t > 3.0f) {
        const char *p = "Enter: New Game+ (keep your levels, skills and gear)     Esc: Title";
        if (((int)(e->t * 2)) % 2 == 0 || e->t > 8.0f) DrawText(p, (sw - MeasureText(p, (int)(20 * s))) / 2, sh - (int)(30 * s), (int)(20 * s), (Color){255, 240, 190, 255});
        if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && e->t > 5.0f)) return 1;
        if (IsKeyPressed(KEY_ESCAPE)) return 2;
        if (GetTouchPointCount() > 0 && e->t > 6.0f) return 1;
    }
    return 0;
}
