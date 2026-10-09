#include "core/data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include "core/log.h"

static char *read_all(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { LOGE("cannot open %s", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = 0;
    return buf;
}

static const char *ELEMENT_NAMES[EL_COUNT] = {"phys", "fire", "ice", "lightning", "holy", "shadow"};
static const char *STATUS_NAMES[ST_COUNT] = {"burn", "poison", "slow", "stun", "silence", "hollow_curse", "taunt",
                                              "buff_atk", "buff_def", "def_down", "regen", "mana_regen"};
static const char *KIND_NAMES[] = {"melee_arc", "dash_strike", "aoe_circle", "projectile", "heal", "heal_aoe", "buff",
                                   "barrier", "taunt", "revive", "cleanse", "passive"};
static const char *ROLE_NAMES[] = {"hero", "tank", "mage", "healer", "enemy"};
static const char *BEHAVIOR_NAMES[] = {"melee", "ranged", "brute"};

const char *status_name(StatusKind s) { return s >= 0 && s < ST_COUNT ? STATUS_NAMES[s] : "none"; }

static int index_of(const char **names, int n, const char *s);
StatusKind status_from_name(const char *s) { int i = index_of(STATUS_NAMES, ST_COUNT, s); return i >= 0 ? (StatusKind)i : ST_NONE; }

static int index_of(const char **names, int n, const char *s) {
    for (int i = 0; i < n; ++i) if (strcmp(names[i], s) == 0) return i;
    return -1;
}

Element element_from_name(const char *s, bool *ok) {
    int i = index_of(ELEMENT_NAMES, EL_COUNT, s);
    if (ok) *ok = i >= 0;
    return i >= 0 ? (Element)i : EL_PHYS;
}

static float num(const cJSON *o, const char *k, float def) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k);
    return cJSON_IsNumber(v) ? (float)v->valuedouble : def;
}
static bool boolean(const cJSON *o, const char *k) { return cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(o, k)); }
static const char *str(const cJSON *o, const char *k) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k);
    return cJSON_IsString(v) ? v->valuestring : "";
}
static void copy(char *dst, size_t cap, const char *src) { snprintf(dst, cap, "%s", src); }

bool skilldb_load(SkillDB *db, const char *path) {
    memset(db, 0, sizeof *db);
    char *text = read_all(path);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text);
    free(text);
    if (!root) { LOGE("%s: JSON parse error near '%.20s'", path, cJSON_GetErrorPtr() ? cJSON_GetErrorPtr() : ""); return false; }
    const cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "skills");
    bool ok = cJSON_IsArray(arr);
    const cJSON *it;
    cJSON_ArrayForEach(it, arr) {
        if (db->n >= DB_MAX_SKILLS) { LOGE("%s: too many skills", path); ok = false; break; }
        Skill *s = &db->s[db->n];
        copy(s->id, sizeof s->id, str(it, "id")); copy(s->name, sizeof s->name, str(it, "name"));
        copy(s->anim, sizeof s->anim, str(it, "anim")); copy(s->desc, sizeof s->desc, str(it, "desc"));
        int role = index_of(ROLE_NAMES, 5, str(it, "role")), kind = index_of(KIND_NAMES, 12, str(it, "kind"));
        bool eok; Element el = element_from_name(str(it, "element"), &eok);
        const char *stn = str(it, "status");
        int stk = stn[0] ? index_of(STATUS_NAMES, ST_COUNT, stn) : ST_NONE;
        if (!s->id[0] || role < 0 || kind < 0 || !eok || (stn[0] && stk < 0)) {
            LOGE("%s: skill #%d '%s' has an invalid id/role/kind/element/status", path, db->n, s->id);
            ok = false; continue;
        }
        if (skilldb_find(db, s->id) >= 0) { LOGE("%s: duplicate skill id '%s'", path, s->id); ok = false; continue; }
        s->role = (Role)role; s->kind = (SkillKind)kind; s->element = el; s->status = (StatusKind)stk;
        s->magic = boolean(it, "magic"); s->basic = boolean(it, "basic"); s->passive = boolean(it, "passive");
        s->power = num(it, "power", 1); s->mp = num(it, "mp", 0); s->cooldown = num(it, "cooldown", 0);
        s->range = num(it, "range", 2); s->arc_deg = num(it, "arc", 90); s->radius = num(it, "radius", 0);
        s->hit_time = num(it, "hit_time", 0.3f); s->total_time = num(it, "total_time", 0.6f);
        s->stagger = num(it, "stagger", 0); s->proj_speed = num(it, "proj_speed", 0); s->dash = num(it, "dash", 0);
        s->status_time = num(it, "status_time", 0); s->status_power = num(it, "status_power", 0);
        s->awakening = (int)num(it, "awakening", 0); s->gauge = num(it, "gauge", 0);
        if (s->hit_time > s->total_time) { LOGE("%s: skill '%s' hits after it ends", path, s->id); ok = false; }
        db->n++;
    }
    cJSON_Delete(root);
    return ok && db->n > 0;
}

int skilldb_find(const SkillDB *db, const char *id) {
    for (int i = 0; i < db->n; ++i) if (strcmp(db->s[i].id, id) == 0) return i;
    return -1;
}

bool enemydb_load(EnemyDB *db, const SkillDB *skills, const char *path) {
    memset(db, 0, sizeof *db);
    char *text = read_all(path);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text);
    free(text);
    if (!root) { LOGE("%s: JSON parse error", path); return false; }
    const cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "enemies");
    bool ok = cJSON_IsArray(arr);
    const cJSON *it;
    cJSON_ArrayForEach(it, arr) {
        if (db->n >= DB_MAX_ENEMIES) { ok = false; break; }
        EnemyDef *e = &db->e[db->n];
        copy(e->id, sizeof e->id, str(it, "id")); copy(e->name, sizeof e->name, str(it, "name"));
        int beh = index_of(BEHAVIOR_NAMES, 3, str(it, "behavior"));
        if (!e->id[0] || beh < 0) { LOGE("%s: enemy '%s' has no valid id/behavior", path, e->id); ok = false; continue; }
        e->behavior = (AiBehavior)beh;
        e->level = (int)num(it, "level", 1);
        const cJSON *st = cJSON_GetObjectItemCaseSensitive(it, "stats");
        e->stats = (Stats){(int)num(st, "str", 5), (int)num(st, "int", 5), (int)num(st, "vit", 5), (int)num(st, "wis", 5), (int)num(st, "agi", 5)};
        e->speed = num(it, "speed", 3); e->aggro_range = num(it, "aggro_range", 12); e->leash = num(it, "leash", 24);
        e->poise = num(it, "poise", 50); e->preferred_range = num(it, "preferred_range", 2); e->scale = num(it, "scale", 1);
        e->xp = (int)num(it, "xp", 10); e->boss = boolean(it, "boss");
        for (int k = 0; k < EL_COUNT; ++k) e->resist[k] = 1.0f;
        const cJSON *rs = cJSON_GetObjectItemCaseSensitive(it, "resist");
        for (int k = 0; k < EL_COUNT; ++k) e->resist[k] = num(rs, ELEMENT_NAMES[k], 1.0f);
        const cJSON *sk = cJSON_GetObjectItemCaseSensitive(it, "skills");
        const cJSON *sid;
        cJSON_ArrayForEach(sid, sk) {
            int idx = cJSON_IsString(sid) ? skilldb_find(skills, sid->valuestring) : -1;
            if (idx < 0 || e->nskills >= 4) { LOGE("%s: enemy '%s' references unknown skill", path, e->id); ok = false; continue; }
            e->skills[e->nskills++] = idx;
        }
        if (e->nskills == 0) { LOGE("%s: enemy '%s' has no skills", path, e->id); ok = false; }
        const cJSON *pt = cJSON_GetObjectItemCaseSensitive(it, "paint");
        static const char *pk[3] = {"top", "bottom", "shoes"};
        for (int k = 0; k < 3; ++k) {
            const cJSON *c = cJSON_GetObjectItemCaseSensitive(pt, pk[k]);
            for (int j = 0; j < 3; ++j) e->paint[k][j] = (unsigned char)(cJSON_IsArray(c) ? cJSON_GetArrayItem(c, j)->valueint : 20);
        }
        const cJSON *ey = cJSON_GetObjectItemCaseSensitive(pt, "eye");
        for (int j = 0; j < 3; ++j) e->eye[j] = (unsigned char)(cJSON_IsArray(ey) ? cJSON_GetArrayItem(ey, j)->valueint : 200);
        db->n++;
    }
    cJSON_Delete(root);
    return ok && db->n > 0;
}

int enemydb_find(const EnemyDB *db, const char *id) {
    for (int i = 0; i < db->n; ++i) if (strcmp(db->e[i].id, id) == 0) return i;
    return -1;
}

bool partydb_load(PartyDB *db, const SkillDB *skills, const char *path) {
    memset(db, 0, sizeof *db);
    char *text = read_all(path);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text);
    free(text);
    if (!root) { LOGE("%s: JSON parse error", path); return false; }
    const cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "party");
    bool ok = cJSON_IsArray(arr);
    const cJSON *it;
    cJSON_ArrayForEach(it, arr) {
        if (db->n >= DB_MAX_PARTY) { ok = false; break; }
        PartyDef *p = &db->m[db->n];
        copy(p->id, sizeof p->id, str(it, "id")); copy(p->name, sizeof p->name, str(it, "name"));
        int role = index_of(ROLE_NAMES, 4, str(it, "role"));
        if (!p->id[0] || role < 0) { LOGE("%s: party member '%s' has no valid id/role", path, p->id); ok = false; continue; }
        p->role = (Role)role;
        p->level = (int)num(it, "level", 1);
        const cJSON *st = cJSON_GetObjectItemCaseSensitive(it, "stats");
        p->stats = (Stats){(int)num(st, "str", 5), (int)num(st, "int", 5), (int)num(st, "vit", 5), (int)num(st, "wis", 5), (int)num(st, "agi", 5)};
        p->poise = num(it, "poise", 100); p->weapon_atk = num(it, "weapon_atk", 0); p->weapon_matk = num(it, "weapon_matk", 0); p->armor = num(it, "armor", 0);
        const cJSON *lists[3] = {cJSON_GetObjectItemCaseSensitive(it, "skills"), cJSON_GetObjectItemCaseSensitive(it, "slots"), cJSON_GetObjectItemCaseSensitive(it, "basic")};
        for (int s = 0; s < 4; ++s) p->slots[s] = -1;
        for (int l = 0; l < 3; ++l) {
            int k = 0;
            const cJSON *sid;
            cJSON_ArrayForEach(sid, lists[l]) {
                int idx = cJSON_IsString(sid) ? skilldb_find(skills, sid->valuestring) : -1;
                if (idx < 0) { LOGE("%s: member '%s' references unknown skill", path, p->id); ok = false; continue; }
                if (l == 0 && p->nskills < 16) p->skills[p->nskills++] = idx;
                else if (l == 1 && k < 4) p->slots[k++] = idx;
                else if (l == 2 && p->nbasic < 3) p->basic[p->nbasic++] = idx;
            }
        }
        if (p->nbasic == 0) { LOGE("%s: member '%s' has no basic attack", path, p->id); ok = false; }
        db->n++;
    }
    cJSON_Delete(root);
    return ok && db->n > 0;
}
