#ifndef SA_INPUT_H
#define SA_INPUT_H
/* Unified input: keyboard + pointer-locked mouse, gamepad, and touch (floating joystick, swipe-to-look,
 * on-screen buttons). One-shot actions are latched until the simulation consumes them. */
#include <stdbool.h>
#include <raylib.h>

typedef struct {
    float move_x, move_y;      /* -1..1, y: +1 = forward */
    float look_dx, look_dy;    /* radians this frame */
    bool jump, attack, roll, lock;   /* latched presses */
    bool skill[4];                   /* latched skill-slot presses */
    bool talk_visible;               /* the game shows the TALK button (an NPC is near) */
    bool talk;                       /* latched: the context action (talk / Call Out / Quartet Strike) */
    const char *ctx_label;           /* touch label of that button (NULL = TALK) */
    bool swap_next, cmd_cycle, menu; /* latched: switch character / cycle the party command / open the menu */
    bool quick[4];                   /* latched quick-use: HP potion, MP potion, revive, cleanse */
    int swap_to;                     /* latched direct selection 0..3, -1 none */
    bool sprint, walk, guard;        /* held */
    bool touch_ui;                   /* a touch was seen: show on-screen controls */
    /* touch tracking */
    int joy_id, look_id;
    Vector2 joy_origin, joy_pos, look_last;
    int btn_id[12];                  /* touch id holding each button, -1 none */
} Input;

void input_init(Input *in);
void input_update(Input *in, float dt, bool gameplay_active);
void input_consume(Input *in);       /* clear latched presses after the sim has seen them */
void input_draw_touch(const Input *in);

#endif
