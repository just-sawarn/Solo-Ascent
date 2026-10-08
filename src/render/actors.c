#include "render/actors.h"
#include <raymath.h>
#include <string.h>
#include "render/character.h"

static int c_idle, c_walk, c_jog, c_sword, c_spell, c_punch, c_hit, c_death;

void actors_init(void) {
    c_idle = anim_find("Idle_Loop"); c_walk = anim_find("Walk_Loop"); c_jog = anim_find("Jog_Fwd_Loop");
    c_sword = anim_find("Sword_Attack"); c_spell = anim_find("Spell_Simple_Shoot"); c_punch = anim_find("Punch_Cross");
    c_hit = anim_find("Hit_Chest"); c_death = anim_find("Death01");
}

static void fit(ActorAnim *a, int clip, float fade, float seconds) {
    anim_play(&a->anim, clip, fade, true);
    float len = anim_clip_length(clip);
    a->anim.speed = (len > 0.01f && seconds > 0.01f) ? len / seconds : 1.0f;
}

void actor_enemy_update(ActorAnim *a, const Combatant *c, const EnemyDef *d, const SkillDB *skills, float speed, float dt) {
    if (!a->init) { anim_player_init(&a->anim, c_idle); a->init = true; }
    if (c->dead) {
        if (a->anim.clip[0] != c_death) { anim_play(&a->anim, c_death, 0.08f, true); a->anim.speed = 1.0f; }
    } else if (c->stagger_t > 0) {
        if (!a->was_staggered) { anim_play(&a->anim, c_hit, 0.04f, true); a->anim.speed = 1.3f; }
    } else if (c->cast_skill >= 0) {
        if (!a->was_casting || a->cur_cast != c->cast_skill) {
            const Skill *s = &skills->s[c->cast_skill];
            int clip = s->kind == SK_PROJECTILE ? c_spell : (d->behavior == AI_BRUTE ? c_punch : c_sword);
            fit(a, clip, 0.08f, s->total_time);
        }
    } else {
        int clip = speed < 0.3f ? c_idle : speed < 2.9f ? c_walk : c_jog;
        anim_play(&a->anim, clip, 0.2f, false);
        a->anim.speed = 1.0f;
    }
    a->was_staggered = c->stagger_t > 0;
    a->was_casting = c->cast_skill >= 0;
    a->cur_cast = c->cast_skill;
    if (!(c->dead && a->anim.finished)) anim_update(&a->anim, dt); /* hold the last death frame */
}

void actor_enemy_submit(const ActorAnim *a, const Combatant *c, const EnemyDef *d, Vector3 pos, float yaw) {
    if (!characters_ready() || !a->init) return;
    Pose pose;
    character_pose_anim(SEX_MALE, &a->anim, NULL, 0, &pose);
    float s = d->scale;
    if (c->dead && c->dead_t > 3.0f) s *= fmaxf(0.0f, 1.0f - (c->dead_t - 3.0f)); /* shrink away */
    if (s < 0.02f) return;
    Matrix root = MatrixMultiply(MatrixMultiply(MatrixScale(0.9f * s, s, 0.92f * s), MatrixRotateY(yaw)), MatrixTranslate(pos.x, pos.y, pos.z));
    CharStyle st = {
        .sex = SEX_MALE, .light_skin = false, .skin_tint = (Color){26, 20, 40, 255}, .hair = HAIRSTYLE_NONE,
        .hair_color = (Color){10, 8, 18, 255},
        .skin_detail = -1.0f, .eye_color = (Color){d->eye[0], d->eye[1], d->eye[2], 255}, .eye_glow = 1.0f,
        .top = (Color){d->paint[0][0], d->paint[0][1], d->paint[0][2], 255},
        .bottom = (Color){d->paint[1][0], d->paint[1][1], d->paint[1][2], 255},
        .shoes = (Color){d->paint[2][0], d->paint[2][1], d->paint[2][2], 255},
        .sleeve_reach = 0.95f, .hem_y = 0.06f, .shoe_h = 0.08f, .waist_y = 0.95f,
        .flash = c->hit_flash > 0 ? fminf(1.0f, c->hit_flash / 0.12f) : 0.0f,
    };
    character_submit(&st, &pose, root);
}
