#ifndef SA_DATA_H
#define SA_DATA_H
/* Data-driven content tables loaded from JSON (see the data directory). All content ids are stable strings. */
#include <stdbool.h>

typedef enum { EL_PHYS, EL_FIRE, EL_ICE, EL_LIGHTNING, EL_HOLY, EL_SHADOW, EL_COUNT } Element;
typedef enum {
    ST_BURN, ST_POISON, ST_SLOW, ST_STUN, ST_SILENCE, ST_HOLLOW_CURSE, ST_TAUNT,
    ST_BUFF_ATK, ST_BUFF_DEF, ST_DEF_DOWN, ST_REGEN, ST_MANA_REGEN, ST_COUNT, ST_NONE = -1
} StatusKind;
typedef enum {
    SK_MELEE_ARC, SK_DASH_STRIKE, SK_AOE_CIRCLE, SK_PROJECTILE, SK_HEAL, SK_HEAL_AOE, SK_BUFF,
    SK_BARRIER, SK_TAUNT, SK_REVIVE, SK_CLEANSE, SK_PASSIVE
} SkillKind;
typedef enum { ROLE_HERO, ROLE_TANK, ROLE_MAGE, ROLE_HEALER, ROLE_ENEMY } Role;
typedef enum { AI_MELEE, AI_RANGED, AI_BRUTE } AiBehavior;

typedef struct {
    char id[32], name[40], anim[32], desc[112];
    Role role; SkillKind kind; Element element; bool magic, basic, passive;
    float power, mp, cooldown, range, arc_deg, radius, hit_time, total_time, stagger, proj_speed, dash;
    StatusKind status; float status_time, status_power;
    int awakening;       /* Ascendant Awakening level required (hero skills) */
    float gauge;         /* Resonance Gauge cost */
} Skill;

#define DB_MAX_SKILLS 96
typedef struct { Skill s[DB_MAX_SKILLS]; int n; } SkillDB;

typedef struct { int str, intel, vit, wis, agi; } Stats;

typedef struct {
    char id[32], name[40];
    int level; Stats stats;
    float speed, aggro_range, leash, poise, preferred_range, scale;
    AiBehavior behavior;
    int skills[4], nskills;
    int xp;
    float resist[EL_COUNT];
    unsigned char paint[3][3], eye[3];
} EnemyDef;

#define DB_MAX_ENEMIES 32
typedef struct { EnemyDef e[DB_MAX_ENEMIES]; int n; } EnemyDB;

bool skilldb_load(SkillDB *db, const char *path);
int  skilldb_find(const SkillDB *db, const char *id); /* -1 if unknown */
typedef struct {
    char id[16], name[24];
    Role role;
    int level; Stats stats;
    float poise, weapon_atk, weapon_matk, armor;
    int skills[16], nskills;   /* everything this member knows (AI picks from these) */
    int slots[4];              /* player skill-slot bindings (-1 = empty) */
    int basic[3], nbasic;
} PartyDef;
#define DB_MAX_PARTY 8
typedef struct { PartyDef m[DB_MAX_PARTY]; int n; } PartyDB;
bool partydb_load(PartyDB *db, const SkillDB *skills, const char *path);

bool enemydb_load(EnemyDB *db, const SkillDB *skills, const char *path);
int  enemydb_find(const EnemyDB *db, const char *id);

const char *status_name(StatusKind s);
Element element_from_name(const char *s, bool *ok);

#endif
