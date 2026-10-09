#include "core/story.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include "core/log.h"

/* ============================ strings ============================ */
static int cmp_entry(const void *a, const void *b) { return strcmp(((const StrEntry *)a)->id, ((const StrEntry *)b)->id); }

static char *read_all(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { LOGE("cannot open %s", path); return NULL; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *b = malloc((size_t)n + 1);
    if (!b) { fclose(f); return NULL; }
    size_t got = fread(b, 1, (size_t)n, f); fclose(f); b[got] = 0;
    return b;
}

bool strings_load(StringTable *t, const char *path) {
    memset(t, 0, sizeof *t);
    char *text = read_all(path);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text);
    free(text);
    if (!root) { LOGE("%s: JSON parse error", path); return false; }
    int n = cJSON_GetArraySize(root);
    size_t bytes = 0;
    const cJSON *it;
    cJSON_ArrayForEach(it, root) if (cJSON_IsString(it) && it->string) bytes += strlen(it->string) + strlen(it->valuestring) + 2;
    t->pool = malloc(bytes + 1); t->e = calloc((size_t)n + 1, sizeof(StrEntry));
    if (!t->pool || !t->e) { cJSON_Delete(root); return false; }
    char *p = t->pool;
    cJSON_ArrayForEach(it, root) {
        if (!cJSON_IsString(it) || !it->string) continue;
        size_t a = strlen(it->string) + 1, b = strlen(it->valuestring) + 1;
        memcpy(p, it->string, a); t->e[t->n].id = p; p += a;
        memcpy(p, it->valuestring, b); t->e[t->n].text = p; p += b;
        t->n++;
    }
    cJSON_Delete(root);
    qsort(t->e, (size_t)t->n, sizeof(StrEntry), cmp_entry);
    return t->n > 0;
}

void strings_free(StringTable *t) { free(t->e); free(t->pool); memset(t, 0, sizeof *t); }

const char *str_get(const StringTable *t, const char *id) {
    if (!id || !id[0]) return NULL;
    StrEntry key = {id, NULL};
    StrEntry *hit = bsearch(&key, t->e, (size_t)t->n, sizeof(StrEntry), cmp_entry);
    return hit ? hit->text : NULL;
}

const char *str_format(const char *text, const char *player_name, char *out, size_t cap) {
    size_t n = 0;
    for (const char *p = text; *p && n + 1 < cap;) {
        if (*p == '{') {
            const char *e = strchr(p, '}');
            if (e && e - p < 32) {
                char var[32]; size_t l = (size_t)(e - p - 1); memcpy(var, p + 1, l); var[l] = 0;
                const char *rep = !strcmp(var, "player_name") ? player_name : NULL;
                if (rep) { for (const char *q = rep; *q && n + 1 < cap; ++q) out[n++] = *q; p = e + 1; continue; }
            }
        }
        out[n++] = *p++;
    }
    out[n] = 0;
    return out;
}

/* ============================ conditions & actions ============================ */
static const char *QSTATE_WORDS[] = {"locked", "available", "active", "ready", "complete"};
static int qstate_word(const char *w) { for (int i = 0; i < 5; ++i) if (!strcmp(QSTATE_WORDS[i], w)) return i; return -1; }

typedef struct { bool neg; char var[56]; int op; /* 0 none (truthy), 1 == 2 != 3 >= 4 <= 5 > 6 < */ char val[40]; } Cond;

static bool parse_cond(const char *s, Cond *c) {
    memset(c, 0, sizeof *c);
    while (isspace((unsigned char)*s)) s++;
    if (*s == '!') { c->neg = true; s++; }
    int n = 0;
    while (*s && !isspace((unsigned char)*s) && n < 55) c->var[n++] = *s++;
    c->var[n] = 0;
    if (!n) return false;
    while (isspace((unsigned char)*s)) s++;
    if (!*s) return true;
    int op = 0, l = 0;
    if (s[0] == '=' && s[1] == '=') { op = 1; l = 2; } else if (s[0] == '!' && s[1] == '=') { op = 2; l = 2; }
    else if (s[0] == '>' && s[1] == '=') { op = 3; l = 2; } else if (s[0] == '<' && s[1] == '=') { op = 4; l = 2; }
    else if (s[0] == '>') { op = 5; l = 1; } else if (s[0] == '<') { op = 6; l = 1; }
    if (!op) return false;
    s += l;
    while (isspace((unsigned char)*s)) s++;
    n = 0;
    while (*s && !isspace((unsigned char)*s) && n < 39) c->val[n++] = *s++;
    c->val[n] = 0;
    c->op = op;
    return n > 0;
}

bool cond_valid(const char *cond) {
    Cond c;
    if (!parse_cond(cond, &c)) return false;
    if (!strncmp(c.var, "quest.", 6)) return c.op == 0 || ((c.op == 1 || c.op == 2) && qstate_word(c.val) >= 0);
    bool numeric = c.op == 0 || isdigit((unsigned char)c.val[0]) || (c.val[0] == '-' && isdigit((unsigned char)c.val[1]));
    if (!numeric) return false;
    if (!strcmp(c.var, "act") || !strcmp(c.var, "mq") || !strcmp(c.var, "awakening") || !strcmp(c.var, "gold") || !strcmp(c.var, "level")) return c.op != 0;
    return !strncmp(c.var, "bond.", 5) || !strncmp(c.var, "rep.", 4) || !strncmp(c.var, "flag.", 5) || !strncmp(c.var, "item.", 5);
}

int flag_get(const StoryState *s, const char *key) {
    for (int i = 0; i < s->nflags; ++i) if (!strcmp(s->flags[i].key, key)) return s->flags[i].value;
    return 0;
}
void flag_set(StoryState *s, const char *key, int v) {
    for (int i = 0; i < s->nflags; ++i) if (!strcmp(s->flags[i].key, key)) { s->flags[i].value = v; return; }
    if (s->nflags >= STORY_MAX_FLAGS) { LOGE("story: flag table full (%s)", key); return; }
    snprintf(s->flags[s->nflags].key, sizeof s->flags[0].key, "%s", key);
    s->flags[s->nflags++].value = v;
}
void flag_add(StoryState *s, const char *key, int d) { flag_set(s, key, flag_get(s, key) + d); }

int storydb_find_quest(const StoryDB *db, const char *id) { for (int i = 0; i < db->nquest; ++i) if (!strcmp(db->quest[i].id, id)) return i; return -1; }
int storydb_find_npc(const StoryDB *db, const char *id) { for (int i = 0; i < db->nnpc; ++i) if (!strcmp(db->npc[i].id, id)) return i; return -1; }
int storydb_find_tree(const StoryDB *db, const char *id) { for (int i = 0; i < db->ntree; ++i) if (!strcmp(db->tree[i].id, id)) return i; return -1; }
int storydb_find_loc(const StoryDB *db, const char *id) { for (int i = 0; i < db->nloc; ++i) if (!strcmp(db->loc[i].id, id)) return i; return -1; }

int story_mq(const StoryState *s, const StoryDB *db) {
    int best = 0;
    for (int i = 0; i < db->nquest; ++i) if (db->quest[i].type == QT_MAIN && s->q[i].state == QS_COMPLETE && db->quest[i].number > best) best = db->quest[i].number;
    return best;
}

int story_act(const StoryState *s, const StoryDB *db) {
    int lowest = 1000, act = 5;
    for (int i = 0; i < db->nquest; ++i) {
        const QuestDef *q = &db->quest[i];
        if (q->type == QT_MAIN && s->q[i].state != QS_COMPLETE && q->number < lowest) { lowest = q->number; act = q->act; }
    }
    return act;
}

bool cond_eval(const StoryState *s, const StoryDB *db, const StoryHooks *h, const char *cond) {
    Cond c;
    if (!parse_cond(cond, &c)) return false;
    bool res;
    if (!strncmp(c.var, "quest.", 6)) {
        int q = storydb_find_quest(db, c.var + 6);
        int st = q >= 0 ? s->q[q].state : -1;
        if (c.op == 0) res = st == QS_COMPLETE;
        else { int w = qstate_word(c.val); res = c.op == 1 ? st == w : st != w; }
    } else {
        long v = 0;
        if (!strcmp(c.var, "act")) v = story_act(s, db);
        else if (!strcmp(c.var, "mq")) v = story_mq(s, db);
        else if (!strcmp(c.var, "awakening")) v = s->awakening;
        else if (!strcmp(c.var, "level")) v = s->level;
        else if (!strcmp(c.var, "gold")) v = h && h->gold ? h->gold(h->user) : 0;
        else if (!strncmp(c.var, "item.", 5)) v = h && h->item_count ? h->item_count(h->user, c.var + 5) : 0;
        else v = flag_get(s, c.var); /* flag.x, bond.x, rep.x are stored under their full names */
        long rhs = atol(c.val);
        switch (c.op) {
        case 0: res = v > 0; break;
        case 1: res = v == rhs; break; case 2: res = v != rhs; break;
        case 3: res = v >= rhs; break; case 4: res = v <= rhs; break;
        case 5: res = v > rhs; break; default: res = v < rhs; break;
        }
    }
    return c.neg ? !res : res;
}

bool conds_eval(const StoryState *s, const StoryDB *db, const StoryHooks *h, const char *const *conds, int n) {
    for (int i = 0; i < n; ++i) if (!cond_eval(s, db, h, conds[i])) return false;
    return true;
}

typedef struct { char kind[16]; char a[48]; char b[48]; int n; } Act;
static bool parse_act(const char *s, Act *a) {
    memset(a, 0, sizeof *a);
    const char *p = strchr(s, ':');
    size_t kl = p ? (size_t)(p - s) : strlen(s);
    if (kl == 0 || kl >= sizeof a->kind) return false;
    memcpy(a->kind, s, kl);
    if (!p) return true;
    p++;
    const char *q = strchr(p, ':');
    size_t al = q ? (size_t)(q - p) : strlen(p);
    if (al >= sizeof a->a) return false;
    memcpy(a->a, p, al);
    if (q) { snprintf(a->b, sizeof a->b, "%s", q + 1); a->n = atoi(a->b); }
    else a->n = atoi(a->a);
    return true;
}

bool act_valid(const char *act) {
    Act a;
    if (!parse_act(act, &a)) return false;
    static const char *known[] = {"quest.start", "quest.complete", "flag.set", "item.give", "item.take", "gold.give", "bond.add", "rep.add", "shop.open", "cutscene", "toast", "learn", "xp.give", "track", "awaken"};
    for (unsigned i = 0; i < sizeof known / sizeof known[0]; ++i) if (!strcmp(known[i], a.kind)) return a.a[0] != 0;
    return false;
}

void act_run(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *act) {
    Act a;
    if (!parse_act(act, &a)) return;
    char key[64];
    if (!strcmp(a.kind, "quest.start")) { int q = storydb_find_quest(db, a.a); if (q >= 0) story_start_quest(s, db, h, q); }
    else if (!strcmp(a.kind, "quest.complete")) { int q = storydb_find_quest(db, a.a); if (q >= 0) story_complete_quest(s, db, h, q); }
    else if (!strcmp(a.kind, "flag.set")) flag_set(s, a.a[0] && !strncmp(a.a, "flag.", 5) ? a.a : (snprintf(key, sizeof key, "flag.%s", a.a), key), a.b[0] ? a.n : 1);
    else if (!strcmp(a.kind, "item.give")) { if (h && h->give_item) h->give_item(h->user, a.a, a.b[0] ? a.n : 1); }
    else if (!strcmp(a.kind, "item.take")) { if (h && h->take_item) h->take_item(h->user, a.a, a.b[0] ? a.n : 1); }
    else if (!strcmp(a.kind, "gold.give")) { if (h && h->give_gold) h->give_gold(h->user, a.n); }
    else if (!strcmp(a.kind, "xp.give")) { if (h && h->give_xp) h->give_xp(h->user, a.n); }
    else if (!strcmp(a.kind, "bond.add")) { snprintf(key, sizeof key, "bond.%s", a.a); flag_add(s, key, a.b[0] ? a.n : 1); }
    else if (!strcmp(a.kind, "rep.add")) { snprintf(key, sizeof key, "rep.%s", a.a); flag_add(s, key, a.b[0] ? a.n : 1); }
    else if (!strcmp(a.kind, "shop.open")) { if (h && h->open_shop) h->open_shop(h->user, a.a); }
    else if (!strcmp(a.kind, "cutscene")) { if (h && h->cutscene) h->cutscene(h->user, a.a); }
    else if (!strcmp(a.kind, "toast")) { const char *t = str_get(&db->strings, a.a); if (h && h->toast) h->toast(h->user, t ? t : a.a); }
    else if (!strcmp(a.kind, "learn")) { if (h && h->learn_skill) h->learn_skill(h->user, a.a, a.b); }
    else if (!strcmp(a.kind, "awaken")) { if (a.n > s->awakening) s->awakening = a.n; }
    else if (!strcmp(a.kind, "track")) { int q = storydb_find_quest(db, a.a); if (q >= 0) s->tracked = q; }
}

/* ============================ loading ============================ */
static const char *pool_add(StoryDB *db, const char *s) {
    char **np = realloc(db->pool, sizeof(char *) * (size_t)(db->npool + 1));
    if (!np) return "";
    db->pool = np;
    db->pool[db->npool] = strdup(s);
    return db->pool[db->npool++];
}

static cJSON *load_json(const char *dir, const char *file) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", dir, file);
    char *t = read_all(path);
    if (!t) return NULL;
    cJSON *r = cJSON_Parse(t);
    free(t);
    if (!r) LOGE("%s: JSON parse error", path);
    return r;
}

static const char *jstr(const cJSON *o, const char *k) { const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k); return cJSON_IsString(v) ? v->valuestring : ""; }
static float jnum(const cJSON *o, const char *k, float d) { const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k); return cJSON_IsNumber(v) ? (float)v->valuedouble : d; }
static bool jbool(const cJSON *o, const char *k) { return cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(o, k)); }
static void cp(char *dst, size_t cap, const char *src) { snprintf(dst, cap, "%s", src); }
static void rgb(const cJSON *o, const char *k, unsigned char out[3], unsigned char d) {
    const cJSON *a = cJSON_GetObjectItemCaseSensitive(o, k);
    for (int i = 0; i < 3; ++i) out[i] = (unsigned char)(cJSON_IsArray(a) && cJSON_GetArrayItem(a, i) ? cJSON_GetArrayItem(a, i)->valueint : d);
}

static int g_errors;
#define ERR(...) do { if (g_errors++ < 60) LOGE(__VA_ARGS__); } while (0)

static const char *need_str(StoryDB *db, const char *id, const char *ctx) {
    const char *t = str_get(&db->strings, id);
    if (!t) { ERR("story: missing string '%s' (%s)", id, ctx); return ""; }
    return t;
}

static int collect_strs(StoryDB *db, const cJSON *arr, const char **out, int cap, bool conds, const char *ctx) {
    int n = 0;
    const cJSON *it;
    cJSON_ArrayForEach(it, arr) {
        if (!cJSON_IsString(it)) continue;
        if (n >= cap) { ERR("story: too many entries in %s", ctx); break; }
        if (conds ? !cond_valid(it->valuestring) : !act_valid(it->valuestring)) ERR("story: bad %s '%s' in %s", conds ? "condition" : "action", it->valuestring, ctx);
        out[n++] = pool_add(db, it->valuestring);
    }
    return n;
}

/* open-addressing map: node id -> index (load time only) */
typedef struct { const char **keys; int *vals; int cap; } IdMap;
static unsigned hash_str(const char *s) { unsigned h = 2166136261u; while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; } return h; }
static void map_init(IdMap *m, int n) { m->cap = 1; while (m->cap < n * 2 + 8) m->cap <<= 1; m->keys = calloc((size_t)m->cap, sizeof(char *)); m->vals = calloc((size_t)m->cap, sizeof(int)); }
static bool map_put(IdMap *m, const char *k, int v) { unsigned i = hash_str(k) & (unsigned)(m->cap - 1); while (m->keys[i]) { if (!strcmp(m->keys[i], k)) return false; i = (i + 1) & (unsigned)(m->cap - 1); } m->keys[i] = k; m->vals[i] = v; return true; }
static int map_get(const IdMap *m, const char *k) { unsigned i = hash_str(k) & (unsigned)(m->cap - 1); while (m->keys[i]) { if (!strcmp(m->keys[i], k)) return m->vals[i]; i = (i + 1) & (unsigned)(m->cap - 1); } return -1; }
static void map_free(IdMap *m) { free(m->keys); free(m->vals); }

static bool known_actor_ref(const StoryDB *db, const char *s) { return !s[0] || !strcmp(s, "auto") || !strcmp(s, "none") || storydb_find_npc(db, s) >= 0; }

bool storydb_load(StoryDB *db, const char *dir, const EnemyDB *enemies, const ItemDB *items) {
    memset(db, 0, sizeof *db);
    g_errors = 0;
    char path[512];
    snprintf(path, sizeof path, "%s/strings_en.json", dir);
    if (!strings_load(&db->strings, path)) return false;

    /* ---- locations ---- */
    cJSON *root = load_json(dir, "locations.json");
    if (!root) return false;
    const cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "locations"), *it;
    db->loc = calloc((size_t)cJSON_GetArraySize(arr) + 1, sizeof(Location));
    cJSON_ArrayForEach(it, arr) {
        Location *l = &db->loc[db->nloc];
        cp(l->id, sizeof l->id, jstr(it, "id")); cp(l->name, sizeof l->name, jstr(it, "name")); cp(l->region, sizeof l->region, jstr(it, "region")); cp(l->kind, sizeof l->kind, jstr(it, "kind"));
        const cJSON *m = cJSON_GetObjectItemCaseSensitive(it, "map"), *lo = cJSON_GetObjectItemCaseSensitive(it, "local");
        if (cJSON_IsArray(m)) { l->map_x = (float)cJSON_GetArrayItem(m, 0)->valuedouble; l->map_y = (float)cJSON_GetArrayItem(m, 1)->valuedouble; }
        if (cJSON_IsArray(lo)) { l->lx = (float)cJSON_GetArrayItem(lo, 0)->valuedouble; l->lz = (float)cJSON_GetArrayItem(lo, 1)->valuedouble; l->radius = (float)cJSON_GetArrayItem(lo, 2)->valuedouble; }
        l->in_sandbox = jbool(it, "sandbox");
        if (!l->id[0] || storydb_find_loc(db, l->id) >= 0) ERR("story: bad or duplicate location '%s'", l->id);
        db->nloc++;
    }
    cJSON_Delete(root);

    /* ---- npcs ---- */
    root = load_json(dir, "npcs.json");
    if (!root) return false;
    arr = cJSON_GetObjectItemCaseSensitive(root, "npcs");
    db->npc = calloc((size_t)cJSON_GetArraySize(arr) + 1, sizeof(NpcDef));
    cJSON_ArrayForEach(it, arr) {
        NpcDef *n = &db->npc[db->nnpc];
        cp(n->id, sizeof n->id, jstr(it, "id")); cp(n->name, sizeof n->name, jstr(it, "name")); cp(n->region, sizeof n->region, jstr(it, "region"));
        cp(n->role, sizeof n->role, jstr(it, "role")); cp(n->archetype, sizeof n->archetype, jstr(it, "archetype")); cp(n->shop, sizeof n->shop, jstr(it, "shop"));
        cp(n->trainer, sizeof n->trainer, jstr(it, "trainer")); cp(n->tree, sizeof n->tree, jstr(it, "tree"));
        n->personality = need_str(db, jstr(it, "personality"), n->id);
        n->sex = (int)jnum(it, "sex", 0); n->hair = (int)jnum(it, "hair", 0); n->skin = (unsigned char)jnum(it, "skin", 0); n->scale = jnum(it, "scale", 1.0f); n->required = jbool(it, "required");
        rgb(it, "hair_col", n->hair_col, 40); rgb(it, "top", n->top, 120); rgb(it, "bottom", n->bottom, 60); rgb(it, "shoes", n->shoes, 40); rgb(it, "eye", n->eye, 80);
        n->nvisible = collect_strs(db, cJSON_GetObjectItemCaseSensitive(it, "visible_if"), n->visible, 2, true, n->id);
        const cJSON *sc = cJSON_GetObjectItemCaseSensitive(it, "schedule");
        for (int k = 0; k < 3; ++k) {
            const cJSON *e = cJSON_IsArray(sc) ? cJSON_GetArrayItem(sc, k) : NULL;
            if (!e) { ERR("story: npc '%s' needs a 3-slot schedule", n->id); break; }
            cp(n->sched[k].place, sizeof n->sched[k].place, jstr(e, "place")); n->sched[k].x = jnum(e, "x", 0); n->sched[k].z = jnum(e, "z", 0);
            if (strcmp(n->sched[k].place, "home") && strcmp(n->sched[k].place, "work") && strcmp(n->sched[k].place, "tavern")) ERR("story: npc '%s' has bad schedule place '%s'", n->id, n->sched[k].place);
        }
        if (!n->id[0] || storydb_find_npc(db, n->id) >= 0) ERR("story: bad or duplicate npc '%s'", n->id);
        db->nnpc++;
    }
    cJSON_Delete(root);

    /* ---- dialogue ---- */
    root = load_json(dir, "dialogue.json");
    if (!root) return false;
    const cJSON *nodes = cJSON_GetObjectItemCaseSensitive(root, "nodes"), *trees = cJSON_GetObjectItemCaseSensitive(root, "trees");
    int nn = cJSON_GetArraySize(nodes), nchoices_total = 0;
    cJSON_ArrayForEach(it, nodes) nchoices_total += cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(it, "choices"));
    db->node = calloc((size_t)nn + 1, sizeof(DlgNode)); db->choice = calloc((size_t)nchoices_total + 1, sizeof(DlgChoice));
    db->tree = calloc((size_t)cJSON_GetArraySize(trees) + 1, sizeof(DlgTree));
    IdMap nmap; map_init(&nmap, nn);
    cJSON_ArrayForEach(it, nodes) {
        DlgNode *d = &db->node[db->nnode];
        cp(d->id, sizeof d->id, jstr(it, "id")); cp(d->speaker, sizeof d->speaker, jstr(it, "speaker"));
        if (!map_put(&nmap, db->node[db->nnode].id, db->nnode)) ERR("story: duplicate dialogue node '%s'", d->id);
        db->nnode++;
    }
    int ni = 0;
    cJSON_ArrayForEach(it, nodes) {
        DlgNode *d = &db->node[ni++];
        d->text = need_str(db, jstr(it, "text"), d->id);
        if (d->speaker[0] && strcmp(d->speaker, "narrator") && storydb_find_npc(db, d->speaker) < 0 && strcmp(d->speaker, "player") && strcmp(d->speaker, "kael") && strcmp(d->speaker, "bram") && strcmp(d->speaker, "lyra") && strcmp(d->speaker, "mirelle") && strcmp(d->speaker, "darius") && strcmp(d->speaker, "vorathis") && strcmp(d->speaker, "skarn") && strcmp(d->speaker, "aurel") && strcmp(d->speaker, "orwyn") && strcmp(d->speaker, "vessara") && strcmp(d->speaker, "khemet_ur") && strcmp(d->speaker, "isrevna") && strcmp(d->speaker, "pyrrhax") && strcmp(d->speaker, "grukk") && strcmp(d->speaker, "voice"))
            ERR("story: node '%s' has unknown speaker '%s'", d->id, d->speaker);
        d->nconds = collect_strs(db, cJSON_GetObjectItemCaseSensitive(it, "if"), d->conds, 4, true, d->id);
        d->nacts = collect_strs(db, cJSON_GetObjectItemCaseSensitive(it, "do"), d->acts, 6, false, d->id);
        const char *nx = jstr(it, "next");
        d->next = nx[0] ? map_get(&nmap, nx) : -1;
        if (nx[0] && d->next < 0) ERR("story: node '%s' continues to unknown node '%s'", d->id, nx);
        const cJSON *chs = cJSON_GetObjectItemCaseSensitive(it, "choices"), *c;
        d->choice_first = db->nchoice;
        cJSON_ArrayForEach(c, chs) {
            DlgChoice *ch = &db->choice[db->nchoice++];
            ch->text = need_str(db, jstr(c, "text"), d->id);
            ch->nconds = collect_strs(db, cJSON_GetObjectItemCaseSensitive(c, "if"), ch->conds, 3, true, d->id);
            ch->nacts = collect_strs(db, cJSON_GetObjectItemCaseSensitive(c, "do"), ch->acts, 4, false, d->id);
            const char *cn = jstr(c, "next");
            ch->next = cn[0] ? map_get(&nmap, cn) : -1;
            if (cn[0] && ch->next < 0) ERR("story: choice in '%s' goes to unknown node '%s'", d->id, cn);
            d->nchoices++;
            if (d->nchoices > 6) ERR("story: node '%s' has more than 6 choices", d->id);
        }
    }
    cJSON_ArrayForEach(it, trees) {
        DlgTree *t = &db->tree[db->ntree];
        cp(t->id, sizeof t->id, jstr(it, "id"));
        if (storydb_find_tree(db, t->id) >= 0) ERR("story: duplicate dialogue tree '%s'", t->id);
        const cJSON *en;
        cJSON_ArrayForEach(en, cJSON_GetObjectItemCaseSensitive(it, "entries")) {
            if (t->nentries >= 16) { ERR("story: tree '%s' has too many entries", t->id); break; }
            DlgEntry *e = &t->entries[t->nentries++];
            e->nconds = collect_strs(db, cJSON_GetObjectItemCaseSensitive(en, "if"), e->conds, 3, true, t->id);
            e->node = map_get(&nmap, jstr(en, "node"));
            if (e->node < 0) ERR("story: tree '%s' starts at unknown node '%s'", t->id, jstr(en, "node"));
        }
        if (t->nentries == 0) ERR("story: tree '%s' has no entries", t->id);
        db->ntree++;
    }
    map_free(&nmap);
    cJSON_Delete(root);

    /* ---- quests ---- */
    root = load_json(dir, "quests.json");
    if (!root) return false;
    arr = cJSON_GetObjectItemCaseSensitive(root, "quests");
    db->quest = calloc((size_t)cJSON_GetArraySize(arr) + 1, sizeof(QuestDef));
    static const char *types[] = {"main", "side", "companion", "bounty", "hidden", "collection", "lore"};
    static const char *okinds[] = {"kill", "collect", "talk", "explore", "escort", "boss", "flag"};
    cJSON_ArrayForEach(it, arr) {
        if (db->nquest >= STORY_MAX_QUESTS) { ERR("story: too many quests"); break; }
        QuestDef *q = &db->quest[db->nquest];
        cp(q->id, sizeof q->id, jstr(it, "id"));
        int ty = -1; for (int i = 0; i < 7; ++i) if (!strcmp(types[i], jstr(it, "type"))) ty = i;
        if (!q->id[0] || ty < 0 || storydb_find_quest(db, q->id) >= 0) { ERR("story: bad or duplicate quest '%s'", q->id); db->nquest++; continue; }
        q->type = (QuestType)ty; q->act = (int)jnum(it, "act", 1); q->number = (int)jnum(it, "number", 0); q->level = (int)jnum(it, "level", 1);
        cp(q->giver, sizeof q->giver, jstr(it, "giver")); cp(q->turn_in, sizeof q->turn_in, jstr(it, "turn_in")); cp(q->region, sizeof q->region, jstr(it, "region"));
        q->auto_start = jbool(it, "auto_start"); q->repeatable = jbool(it, "repeatable");
        q->title = need_str(db, jstr(it, "title"), q->id); q->summary = need_str(db, jstr(it, "summary"), q->id);
        q->nprereq = collect_strs(db, cJSON_GetObjectItemCaseSensitive(it, "prereq"), q->prereq, 4, true, q->id);
        const cJSON *ob;
        cJSON_ArrayForEach(ob, cJSON_GetObjectItemCaseSensitive(it, "objectives")) {
            if (q->nobj >= Q_MAX_OBJ) { ERR("story: quest '%s' has too many objectives", q->id); break; }
            Objective *o = &q->obj[q->nobj++];
            int ok = -1; for (int i = 0; i < 7; ++i) if (!strcmp(okinds[i], jstr(ob, "kind"))) ok = i;
            if (ok < 0) { ERR("story: quest '%s' has a bad objective kind '%s'", q->id, jstr(ob, "kind")); continue; }
            o->kind = (ObjKind)ok; cp(o->target, sizeof o->target, jstr(ob, "target")); o->count = (int)jnum(ob, "count", 1); if (o->count < 1) o->count = 1;
            o->text = need_str(db, jstr(ob, "text"), q->id); cp(o->marker, sizeof o->marker, jstr(ob, "marker"));
        }
        const cJSON *rw = cJSON_GetObjectItemCaseSensitive(it, "rewards"), *r;
        q->xp = (int)jnum(rw, "xp", 0); q->gold = (int)jnum(rw, "gold", 0);
        cJSON_ArrayForEach(r, cJSON_GetObjectItemCaseSensitive(rw, "items")) if (q->nitems < 4) { cp(q->items[q->nitems].item, 40, jstr(r, "item")); q->items[q->nitems++].qty = (int)jnum(r, "qty", 1); }
        cJSON_ArrayForEach(r, cJSON_GetObjectItemCaseSensitive(rw, "skills")) if (q->nskills < 2) { cp(q->skills[q->nskills].member, 16, jstr(r, "member")); cp(q->skills[q->nskills++].skill, 32, jstr(r, "skill")); }
        cJSON_ArrayForEach(r, cJSON_GetObjectItemCaseSensitive(rw, "bond")) if (q->nbonds < 2) { cp(q->bonds[q->nbonds].name, 16, jstr(r, "name")); q->bonds[q->nbonds++].n = (int)jnum(r, "n", 1); }
        const cJSON *rep = cJSON_GetObjectItemCaseSensitive(rw, "rep");
        if (rep) { cp(q->rep_region, sizeof q->rep_region, jstr(rep, "region")); q->rep_amount = (int)jnum(rep, "n", 0); }
        q->non_complete = collect_strs(db, cJSON_GetObjectItemCaseSensitive(it, "on_complete"), q->on_complete, 4, false, q->id);
        q->non_start = collect_strs(db, cJSON_GetObjectItemCaseSensitive(it, "on_start"), q->on_start, 3, false, q->id);
        cp(q->offer_tree, sizeof q->offer_tree, jstr(it, "offer_tree")); cp(q->progress_tree, sizeof q->progress_tree, jstr(it, "progress_tree")); cp(q->complete_tree, sizeof q->complete_tree, jstr(it, "complete_tree"));
        db->nquest++;
    }
    cJSON_Delete(root);

    /* ---- cross references ---- */
    for (int i = 0; i < db->nnpc; ++i) {
        const NpcDef *n = &db->npc[i];
        if (storydb_find_tree(db, n->tree) < 0) ERR("story: npc '%s' greeting tree '%s' missing", n->id, n->tree);
        if (n->shop[0] && items && itemdb_find_shop(items, n->shop) < 0) ERR("story: npc '%s' uses unknown shop '%s'", n->id, n->shop);
    }
    for (int i = 0; i < db->nquest; ++i) {
        const QuestDef *q = &db->quest[i];
        if (!q->id[0]) continue;
        if (!known_actor_ref(db, q->giver)) ERR("story: quest '%s' giver '%s' unknown", q->id, q->giver);
        if (!known_actor_ref(db, q->turn_in)) ERR("story: quest '%s' turn-in '%s' unknown", q->id, q->turn_in);
        if (q->nobj == 0) ERR("story: quest '%s' has no objectives", q->id);
        for (int k = 0; k < q->nprereq; ++k) { /* quest.X references must exist */
            const char *p = strstr(q->prereq[k], "quest.");
            if (p) { char id[40]; int l = 0; p += 6; while (*p && !isspace((unsigned char)*p) && l < 39) id[l++] = *p++; id[l] = 0; if (storydb_find_quest(db, id) < 0) ERR("story: quest '%s' requires unknown quest '%s'", q->id, id); }
        }
        for (int k = 0; k < q->nobj; ++k) {
            const Objective *o = &q->obj[k];
            switch (o->kind) {
            case OBJ_KILL: if (strcmp(o->target, "*") && enemydb_find(enemies, o->target) < 0) ERR("story: quest '%s' kills unknown enemy '%s'", q->id, o->target); break;
            case OBJ_BOSS: if (enemydb_find(enemies, o->target) < 0) ERR("story: quest '%s' fights unknown boss '%s'", q->id, o->target); break;
            case OBJ_COLLECT: if (items && itemdb_find(items, o->target) < 0) ERR("story: quest '%s' collects unknown item '%s'", q->id, o->target); break;
            case OBJ_TALK: if (storydb_find_npc(db, o->target) < 0) ERR("story: quest '%s' talks to unknown npc '%s'", q->id, o->target); break;
            case OBJ_EXPLORE: if (storydb_find_loc(db, o->target) < 0) ERR("story: quest '%s' explores unknown location '%s'", q->id, o->target); break;
            case OBJ_ESCORT: { char npc[32], loc[32]; if (sscanf(o->target, "%31[^@]@%31s", npc, loc) != 2 || storydb_find_npc(db, npc) < 0 || storydb_find_loc(db, loc) < 0) ERR("story: quest '%s' has a bad escort target '%s'", q->id, o->target); } break;
            case OBJ_FLAG: break;
            }
            if (o->marker[0] && storydb_find_loc(db, o->marker) < 0) ERR("story: quest '%s' marker '%s' unknown", q->id, o->marker);
        }
        const char *trees_[3] = {q->offer_tree, q->progress_tree, q->complete_tree};
        for (int k = 0; k < 3; ++k) if (trees_[k][0] && storydb_find_tree(db, trees_[k]) < 0) ERR("story: quest '%s' references missing tree '%s'", q->id, trees_[k]);
        if (!q->auto_start && strcmp(q->giver, "auto") && strcmp(q->giver, "none") && q->giver[0] && !q->offer_tree[0]) ERR("story: quest '%s' has a giver but no offer tree", q->id);
        if (strcmp(q->turn_in, "auto") && q->turn_in[0] && !q->complete_tree[0]) ERR("story: quest '%s' needs a turn-in tree", q->id);
        if (items) {
            for (int k = 0; k < q->nitems; ++k) if (itemdb_find(items, q->items[k].item) < 0) ERR("story: quest '%s' rewards unknown item '%s'", q->id, q->items[k].item);
        }
        for (int k = 0; k < q->nskills; ++k) { (void)k; }
    }
    return g_errors == 0 && db->nquest > 0 && db->nnpc > 0;
}

void storydb_free(StoryDB *db) {
    strings_free(&db->strings);
    for (int i = 0; i < db->npool; ++i) free(db->pool[i]);
    free(db->pool); free(db->loc); free(db->npc); free(db->quest); free(db->node); free(db->choice); free(db->tree);
    memset(db, 0, sizeof *db);
}

/* ============================ runtime state ============================ */
void story_init(StoryState *s, const StoryDB *db, const char *player_name) {
    memset(s, 0, sizeof *s);
    s->tracked = -1; s->level = 5;
    snprintf(s->player_name, sizeof s->player_name, "%s", player_name && player_name[0] ? player_name : "Kael");
    for (int i = 0; i < db->nquest && i < STORY_MAX_QUESTS; ++i) s->q[i].state = QS_LOCKED;
}

bool quest_objectives_done(const StoryState *s, const StoryDB *db, int q) {
    const QuestDef *d = &db->quest[q];
    for (int k = 0; k < d->nobj; ++k) if (s->q[q].prog[k] < d->obj[k].count) return false;
    return true;
}

static void reset_progress(StoryState *s, int q) { for (int k = 0; k < Q_MAX_OBJ; ++k) s->q[q].prog[k] = 0; }

void story_update(StoryState *s, const StoryDB *db, const StoryHooks *h) {
    for (int pass = 0; pass < 4; ++pass) {
        bool changed = false;
        for (int i = 0; i < db->nquest; ++i) {
            const QuestDef *q = &db->quest[i];
            QuestRt *rt = &s->q[i];
            if (rt->state == QS_LOCKED && conds_eval(s, db, h, q->prereq, q->nprereq)) { rt->state = QS_AVAILABLE; changed = true; }
            if (rt->state == QS_AVAILABLE && q->auto_start) { story_start_quest(s, db, h, i); changed = true; }
            if (rt->state == QS_ACTIVE) { /* flag objectives complete themselves */
                for (int k = 0; k < q->nobj; ++k) if (q->obj[k].kind == OBJ_FLAG && rt->prog[k] < q->obj[k].count) {
                    char key[64]; snprintf(key, sizeof key, "flag.%s", q->obj[k].target);
                    if (flag_get(s, key) > 0) { rt->prog[k] = (int16_t)q->obj[k].count; changed = true; }
                }
                if (quest_objectives_done(s, db, i)) {
                    rt->state = QS_READY; changed = true;
                    if (!strcmp(q->turn_in, "auto") || !q->turn_in[0]) story_complete_quest(s, db, h, i);
                }
            }
        }
        if (!changed) break;
    }
}

bool story_start_quest(StoryState *s, const StoryDB *db, const StoryHooks *h, int q) {
    (void)h;
    QuestRt *rt = &s->q[q];
    if (rt->state != QS_AVAILABLE) return false;
    rt->state = QS_ACTIVE; reset_progress(s, q);
    if (s->tracked < 0 || db->quest[q].type == QT_MAIN) s->tracked = q;
    for (int k = 0; k < db->quest[q].non_start; ++k) act_run(s, db, h, db->quest[q].on_start[k]);
    story_sync_collect(s, db, h); /* items already in the bag count immediately */
    story_update(s, db, h);
    return true;
}

bool story_complete_quest(StoryState *s, const StoryDB *db, const StoryHooks *h, int q) {
    QuestRt *rt = &s->q[q];
    if (rt->state != QS_ACTIVE && rt->state != QS_READY) return false;
    const QuestDef *d = &db->quest[q];
    if (!quest_objectives_done(s, db, q)) return false;
    /* collect objectives consume their items on turn-in */
    for (int k = 0; k < d->nobj; ++k) if (d->obj[k].kind == OBJ_COLLECT && h && h->take_item) h->take_item(h->user, d->obj[k].target, d->obj[k].count);
    rt->times_done++;
    rt->state = d->repeatable ? QS_AVAILABLE : QS_COMPLETE;
    reset_progress(s, q);
    if (d->repeatable) rt->state = QS_AVAILABLE;
    if (s->tracked == q) s->tracked = -1;
    if (h) {
        if (d->xp && h->give_xp) h->give_xp(h->user, d->xp);
        if (d->gold && h->give_gold) h->give_gold(h->user, d->gold);
        for (int k = 0; k < d->nitems; ++k) if (h->give_item) h->give_item(h->user, d->items[k].item, d->items[k].qty);
        for (int k = 0; k < d->nskills; ++k) if (h->learn_skill) h->learn_skill(h->user, d->skills[k].member, d->skills[k].skill);
    }
    for (int k = 0; k < d->nbonds; ++k) { char key[40]; snprintf(key, sizeof key, "bond.%s", d->bonds[k].name); flag_add(s, key, d->bonds[k].n); }
    if (d->rep_region[0]) { char key[40]; snprintf(key, sizeof key, "rep.%s", d->rep_region); flag_add(s, key, d->rep_amount); }
    for (int k = 0; k < d->non_complete; ++k) act_run(s, db, h, d->on_complete[k]);
    char key[64]; snprintf(key, sizeof key, "flag.done_%s", d->id); flag_set(s, key, 1);
    if (h && h->toast) { char t[128]; snprintf(t, sizeof t, "Quest complete: %s", d->title); h->toast(h->user, t); }
    if (s->tracked < 0) { /* track the next main quest, else any active quest */
        for (int i = 0; i < db->nquest; ++i) if (db->quest[i].type == QT_MAIN && s->q[i].state == QS_ACTIVE) { s->tracked = i; break; }
    }
    story_update(s, db, h);
    return true;
}

static void bump(StoryState *s, const StoryDB *db, const StoryHooks *h, ObjKind kind, const char *target, bool wildcard_ok) {
    bool touched = false;
    for (int i = 0; i < db->nquest; ++i) {
        if (s->q[i].state != QS_ACTIVE) continue;
        const QuestDef *q = &db->quest[i];
        for (int k = 0; k < q->nobj; ++k) {
            const Objective *o = &q->obj[k];
            if (o->kind != kind || s->q[i].prog[k] >= o->count) continue;
            if (strcmp(o->target, target) && !(wildcard_ok && !strcmp(o->target, "*"))) continue;
            s->q[i].prog[k]++; touched = true;
        }
    }
    if (touched) story_update(s, db, h);
}

void story_on_kill(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *enemy_id) { bump(s, db, h, OBJ_KILL, enemy_id, true); }
void story_on_boss(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *boss_id) { bump(s, db, h, OBJ_BOSS, boss_id, false); }
void story_on_talk(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *npc_id) { bump(s, db, h, OBJ_TALK, npc_id, false); }
void story_on_location(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *loc_id) { bump(s, db, h, OBJ_EXPLORE, loc_id, false); }
void story_on_escort(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *npc_id, const char *loc_id) {
    char key[80]; snprintf(key, sizeof key, "%s@%s", npc_id, loc_id);
    bump(s, db, h, OBJ_ESCORT, key, false);
}

void story_sync_collect(StoryState *s, const StoryDB *db, const StoryHooks *h) {
    if (!h || !h->item_count) return;
    bool changed = false;
    for (int i = 0; i < db->nquest; ++i) {
        if (s->q[i].state != QS_ACTIVE && s->q[i].state != QS_READY) continue;
        const QuestDef *q = &db->quest[i];
        for (int k = 0; k < q->nobj; ++k) {
            if (q->obj[k].kind != OBJ_COLLECT) continue;
            int have = h->item_count(h->user, q->obj[k].target);
            int v = have < q->obj[k].count ? have : q->obj[k].count;
            if (v != s->q[i].prog[k]) { s->q[i].prog[k] = (int16_t)v; changed = true; }
        }
        if (s->q[i].state == QS_READY && !quest_objectives_done(s, db, i)) s->q[i].state = QS_ACTIVE; /* gave the items away again */
    }
    if (changed) story_update(s, db, h);
}

/* ============================ NPC schedules and dialogue ============================ */
int sched_slot(float hour) {
    hour = hour - 24.0f * (float)((int)(hour / 24.0f));
    if (hour < 0) hour += 24.0f;
    return hour >= 6.0f && hour < 12.0f ? 0 : (hour >= 12.0f && hour < 19.0f ? 1 : 2);
}
const SchedSlot *npc_slot_at(const NpcDef *n, float hour) { return &n->sched[sched_slot(hour)]; }

int story_pick_dialogue(const StoryState *s, const StoryDB *db, const StoryHooks *h, int npc, DlgKind *kind, int *quest) {
    const NpcDef *n = &db->npc[npc];
    int best = -1;
    for (int i = 0; i < db->nquest; ++i) { /* 1. turn-ins (main quests first) */
        const QuestDef *q = &db->quest[i];
        if (s->q[i].state == QS_READY && !strcmp(q->turn_in, n->id) && q->complete_tree[0]) { if (best < 0 || (q->type == QT_MAIN && db->quest[best].type != QT_MAIN)) best = i; }
    }
    if (best >= 0) { *kind = DLG_TURN_IN; *quest = best; return storydb_find_tree(db, db->quest[best].complete_tree); }
    for (int i = 0; i < db->nquest; ++i) { /* 2. offers */
        const QuestDef *q = &db->quest[i];
        if (s->q[i].state == QS_AVAILABLE && !q->auto_start && !strcmp(q->giver, n->id) && q->offer_tree[0]) { if (best < 0 || (q->type == QT_MAIN && db->quest[best].type != QT_MAIN)) best = i; }
    }
    if (best >= 0) { *kind = DLG_OFFER; *quest = best; return storydb_find_tree(db, db->quest[best].offer_tree); }
    for (int i = 0; i < db->nquest; ++i) { /* 3. active quests that involve this NPC */
        const QuestDef *q = &db->quest[i];
        if (s->q[i].state != QS_ACTIVE || !q->progress_tree[0]) continue;
        bool involved = !strcmp(q->giver, n->id);
        for (int k = 0; k < q->nobj; ++k) if (q->obj[k].kind == OBJ_TALK && !strcmp(q->obj[k].target, n->id) && s->q[i].prog[k] < q->obj[k].count) involved = true;
        if (involved) { best = i; break; }
    }
    if (best >= 0) { *kind = DLG_PROGRESS; *quest = best; return storydb_find_tree(db, db->quest[best].progress_tree); }
    (void)h;
    *kind = DLG_GREETING; *quest = -1;
    return storydb_find_tree(db, n->tree);
}

static void settle(DlgRunner *r, StoryState *s, const StoryDB *db, const StoryHooks *h) {
    for (int guard = 0; guard < 64 && r->active; ++guard) {
        if (r->node < 0) { r->active = false; return; }
        const DlgNode *n = &db->node[r->node];
        if (!conds_eval(s, db, h, n->conds, n->nconds)) { r->node = n->next; continue; } /* line skipped */
        for (int i = 0; i < n->nacts; ++i) act_run(s, db, h, n->acts[i]);
        r->nvisible = 0;
        for (int c = 0; c < n->nchoices && r->nvisible < 6; ++c) {
            const DlgChoice *ch = &db->choice[n->choice_first + c];
            if (conds_eval(s, db, h, ch->conds, ch->nconds)) r->visible[r->nvisible++] = c;
        }
        return;
    }
    r->active = false;
}

bool dlg_start(DlgRunner *r, StoryState *s, const StoryDB *db, const StoryHooks *h, int tree, int quest) {
    memset(r, 0, sizeof *r);
    r->quest = quest;
    if (tree < 0 || tree >= db->ntree) return false;
    const DlgTree *t = &db->tree[tree];
    for (int i = 0; i < t->nentries; ++i) {
        if (!conds_eval(s, db, h, t->entries[i].conds, t->entries[i].nconds)) continue;
        r->tree = tree; r->node = t->entries[i].node; r->active = true;
        settle(r, s, db, h);
        return r->active;
    }
    return false;
}

const DlgNode *dlg_node(const DlgRunner *r, const StoryDB *db) { return r->active && r->node >= 0 ? &db->node[r->node] : NULL; }

void dlg_advance(DlgRunner *r, StoryState *s, const StoryDB *db, const StoryHooks *h) {
    const DlgNode *n = dlg_node(r, db);
    if (!n || r->nvisible > 0) return;
    r->node = n->next;
    settle(r, s, db, h);
}

void dlg_choose(DlgRunner *r, StoryState *s, const StoryDB *db, const StoryHooks *h, int vi) {
    const DlgNode *n = dlg_node(r, db);
    if (!n || vi < 0 || vi >= r->nvisible) return;
    const DlgChoice *ch = &db->choice[n->choice_first + r->visible[vi]];
    for (int i = 0; i < ch->nacts; ++i) act_run(s, db, h, ch->acts[i]);
    r->node = ch->next;
    settle(r, s, db, h);
}
