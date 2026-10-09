#include "core/physics.h"
#include <math.h>
#include <string.h>

float phys_ground(const PhysWorld *w, float x, float z) { return w->height ? w->height(w->height_user, x, z) : w->ground_y; }

static int cell_coord(float v) {
    int c = (int)floorf(v / PH_CELL) + PH_GRID / 2;
    return c < 0 ? 0 : (c >= PH_GRID ? PH_GRID - 1 : c);
}

void phys_init(PhysWorld *w, float ground_y) {
    memset(w, 0, sizeof *w);
    w->ground_y = ground_y;
}

static void insert_item(PhysWorld *w, int item, float x0, float z0, float x1, float z1) {
    int cx0 = cell_coord(x0), cx1 = cell_coord(x1), cz0 = cell_coord(z0), cz1 = cell_coord(z1);
    bool overflowed = false;
    for (int cz = cz0; cz <= cz1; ++cz)
        for (int cx = cx0; cx <= cx1; ++cx) {
            int ci = cz * PH_GRID + cx;
            if (w->cell_n[ci] < PH_CELL_CAP) w->cell_item[ci][w->cell_n[ci]++] = (uint16_t)item;
            else overflowed = true;
        }
    if (overflowed && w->noverflow < (int)(sizeof w->overflow / sizeof w->overflow[0])) w->overflow[w->noverflow++] = (uint16_t)item;
}

bool phys_add_box(PhysWorld *w, Vec3 mn, Vec3 mx) {
    if (w->nboxes >= PH_MAX_BOXES) return false;
    w->boxes[w->nboxes] = (PhBox){mn, mx};
    insert_item(w, w->nboxes, mn.x, mn.z, mx.x, mx.z);
    w->nboxes++;
    return true;
}

bool phys_add_cyl(PhysWorld *w, float x, float z, float r, float y0, float y1) {
    if (w->ncyls >= PH_MAX_CYLS) return false;
    w->cyls[w->ncyls] = (PhCyl){x, z, r, y0, y1};
    insert_item(w, PH_MAX_BOXES + w->ncyls, x - r, z - r, x + r, z + r);
    w->ncyls++;
    return true;
}

/* Visit every static whose cells overlap the xz rectangle, once each. */
typedef void (*ItemFn)(PhysWorld *w, int item, void *user);
static void query(PhysWorld *w, float x0, float z0, float x1, float z1, ItemFn fn, void *user) {
    w->stamp++;
    int cx0 = cell_coord(x0), cx1 = cell_coord(x1), cz0 = cell_coord(z0), cz1 = cell_coord(z1);
    for (int cz = cz0; cz <= cz1; ++cz)
        for (int cx = cx0; cx <= cx1; ++cx) {
            int ci = cz * PH_GRID + cx;
            for (int k = 0; k < w->cell_n[ci]; ++k) {
                int it = w->cell_item[ci][k];
                if (w->seen[it] == w->stamp) continue;
                w->seen[it] = w->stamp;
                fn(w, it, user);
            }
        }
    for (int k = 0; k < w->noverflow; ++k) {
        int it = w->overflow[k];
        if (w->seen[it] == w->stamp) continue;
        w->seen[it] = w->stamp;
        fn(w, it, user);
    }
}

/* ---------------- ray casting ---------------- */
typedef struct { Vec3 o, d; float best; } RayQ;

static void ray_item(PhysWorld *w, int item, void *user) {
    RayQ *q = user;
    if (item < PH_MAX_BOXES) {
        const PhBox *b = &w->boxes[item];
        float tmin = 0.0f, tmax = q->best;
        const float o[3] = {q->o.x, q->o.y, q->o.z}, d[3] = {q->d.x, q->d.y, q->d.z};
        const float lo[3] = {b->min.x, b->min.y, b->min.z}, hi[3] = {b->max.x, b->max.y, b->max.z};
        for (int a = 0; a < 3; ++a) {
            if (fabsf(d[a]) < 1e-8f) { if (o[a] < lo[a] || o[a] > hi[a]) return; continue; }
            float t0 = (lo[a] - o[a]) / d[a], t1 = (hi[a] - o[a]) / d[a];
            if (t0 > t1) { float t = t0; t0 = t1; t1 = t; }
            if (t0 > tmin) tmin = t0;
            if (t1 < tmax) tmax = t1;
            if (tmin > tmax) return;
        }
        if (tmin < q->best) q->best = tmin;
    } else {
        const PhCyl *c = &w->cyls[item - PH_MAX_BOXES];
        float ox = q->o.x - c->x, oz = q->o.z - c->z;
        float a = q->d.x * q->d.x + q->d.z * q->d.z;
        if (a < 1e-10f) return;
        float bq = 2.0f * (ox * q->d.x + oz * q->d.z), cq = ox * ox + oz * oz - c->r * c->r;
        float disc = bq * bq - 4.0f * a * cq;
        if (disc < 0) return;
        float t = (-bq - sqrtf(disc)) / (2.0f * a);
        if (cq < 0) t = 0.0f; /* origin inside */
        if (t < 0 || t >= q->best) return;
        float y = q->o.y + q->d.y * t;
        if (y >= c->y0 && y <= c->y1) q->best = t;
    }
}

float phys_raycast(PhysWorld *w, Vec3 o, Vec3 d, float maxd) {
    RayQ q = {o, d, maxd};
    if (w->height) { /* march the terrain in ~1 m steps, then refine */
        float prev_t = 0.0f;
        for (float t = 0.5f; t <= q.best + 0.5f; t += 0.5f) {
            float tt = fminf(t, q.best);
            Vec3 p = vec3_add(o, vec3_scale(d, tt));
            if (p.y < phys_ground(w, p.x, p.z)) {
                float lo = prev_t, hi = tt;
                for (int k = 0; k < 6; ++k) {
                    float mid = 0.5f * (lo + hi);
                    Vec3 m = vec3_add(o, vec3_scale(d, mid));
                    if (m.y < phys_ground(w, m.x, m.z)) hi = mid; else lo = mid;
                }
                if (hi < q.best) q.best = hi;
                break;
            }
            prev_t = tt;
            if (tt >= q.best) break;
        }
    } else if (d.y < -1e-6f && o.y > w->ground_y) {
        float t = (w->ground_y - o.y) / d.y;
        if (t < q.best) q.best = t;
    }
    Vec3 e = vec3_add(o, vec3_scale(d, maxd));
    float x0 = fminf(o.x, e.x), x1 = fmaxf(o.x, e.x), z0 = fminf(o.z, e.z), z1 = fmaxf(o.z, e.z);
    query(w, x0, z0, x1, z1, ray_item, &q);
    return q.best;
}

/* ---------------- character controller ---------------- */
void char_init(CharCtl *c, Vec3 pos) {
    *c = (CharCtl){.pos = pos, .radius = 0.35f, .height = 1.8f, .step_h = 0.45f, .ground_accel = 70.0f, .air_accel = 18.0f};
}

typedef struct { CharCtl *c; float px, pz; bool hit; } PushQ;

static void push_item(PhysWorld *w, int item, void *user) {
    PushQ *q = user;
    CharCtl *c = q->c;
    float feet = c->pos.y, head = c->pos.y + c->height, r = c->radius;
    if (item < PH_MAX_BOXES) {
        const PhBox *b = &w->boxes[item];
        if (b->max.y <= feet + c->step_h + 1e-4f) return; /* low enough to step onto: not a wall */
        if (b->min.y >= head) return;                      /* overhead */
        float cx = fminf(fmaxf(q->px, b->min.x), b->max.x), cz = fminf(fmaxf(q->pz, b->min.z), b->max.z);
        float dx = q->px - cx, dz = q->pz - cz, d2 = dx * dx + dz * dz;
        if (d2 >= r * r) return;
        q->hit = true;
        if (d2 > 1e-10f) {
            float d = sqrtf(d2), push = r - d;
            q->px += dx / d * push; q->pz += dz / d * push;
        } else { /* centre inside the box: leave through the nearest face */
            float l = q->px - b->min.x, rr = b->max.x - q->px, f = q->pz - b->min.z, bk = b->max.z - q->pz;
            float m = fminf(fminf(l, rr), fminf(f, bk));
            if (m == l) q->px = b->min.x - r; else if (m == rr) q->px = b->max.x + r;
            else if (m == f) q->pz = b->min.z - r; else q->pz = b->max.z + r;
        }
    } else {
        const PhCyl *cy = &w->cyls[item - PH_MAX_BOXES];
        if (cy->y1 <= feet + c->step_h + 1e-4f || cy->y0 >= head) return;
        float dx = q->px - cy->x, dz = q->pz - cy->z, rr = r + cy->r, d2 = dx * dx + dz * dz;
        if (d2 >= rr * rr) return;
        q->hit = true;
        float d = sqrtf(d2);
        if (d < 1e-5f) { q->px = cy->x + rr; return; }
        q->px = cy->x + dx / d * rr; q->pz = cy->z + dz / d * rr;
    }
}

static void resolve_horizontal(PhysWorld *w, CharCtl *c) {
    PushQ q = {c, c->pos.x, c->pos.z, false};
    for (int it = 0; it < 4; ++it) {
        q.hit = false;
        query(w, q.px - c->radius, q.pz - c->radius, q.px + c->radius, q.pz + c->radius, push_item, &q);
        if (!q.hit) break;
    }
    c->pos.x = q.px; c->pos.z = q.pz;
}

typedef struct { float x, z, r, max_top, best; float min_bottom; float ceil; float feet, head; } SupQ;

static bool disc_hits_rect(float x, float z, float r, const PhBox *b) {
    float cx = fminf(fmaxf(x, b->min.x), b->max.x), cz = fminf(fmaxf(z, b->min.z), b->max.z);
    float dx = x - cx, dz = z - cz;
    return dx * dx + dz * dz <= r * r;
}

static void support_item(PhysWorld *w, int item, void *user) {
    SupQ *q = user;
    if (item < PH_MAX_BOXES) {
        const PhBox *b = &w->boxes[item];
        if (!disc_hits_rect(q->x, q->z, q->r, b)) return;
        if (b->max.y <= q->max_top && b->max.y > q->best) q->best = b->max.y;
        if (b->min.y >= q->feet + 0.01f && b->min.y < q->ceil && b->min.y < q->head + 0.3f) q->ceil = b->min.y;
    } else {
        const PhCyl *c = &w->cyls[item - PH_MAX_BOXES];
        float dx = q->x - c->x, dz = q->z - c->z, rr = q->r + c->r;
        if (dx * dx + dz * dz > rr * rr) return;
        if (c->y1 <= q->max_top && c->y1 > q->best) q->best = c->y1;
        if (c->y0 >= q->feet + 0.01f && c->y0 < q->ceil && c->y0 < q->head + 0.3f) q->ceil = c->y0;
    }
}

void char_step(PhysWorld *w, CharCtl *c, CharMove in, float dt) {
    /* horizontal velocity toward the wish vector */
    float accel = (c->grounded ? c->ground_accel : c->air_accel) * dt;
    float dx = in.wish_x - c->vel.x, dz = in.wish_z - c->vel.z, dl = sqrtf(dx * dx + dz * dz);
    if (dl <= accel || dl < 1e-6f) { c->vel.x = in.wish_x; c->vel.z = in.wish_z; }
    else { c->vel.x += dx / dl * accel; c->vel.z += dz / dl * accel; }

    /* jump: buffered input + coyote time */
    c->jump_buf = in.jump ? 0.12f : fmaxf(0.0f, c->jump_buf - dt);
    c->coyote = c->grounded ? 0.10f : fmaxf(0.0f, c->coyote - dt);
    bool jumped = false;
    if (c->jump_buf > 0 && c->coyote > 0) {
        c->vel.y = PH_JUMP_SPEED; c->grounded = false; c->coyote = 0; c->jump_buf = 0; jumped = true;
    }
    if (!c->grounded) c->vel.y -= PH_GRAVITY * dt;

    c->pos.x += c->vel.x * dt;
    c->pos.z += c->vel.z * dt;
    resolve_horizontal(w, c);
    if (w->bound > 0) { c->pos.x = clampf(c->pos.x, -w->bound, w->bound); c->pos.z = clampf(c->pos.z, -w->bound, w->bound); }

    float prev_y = c->pos.y;
    bool was_grounded = c->grounded;
    c->pos.y += c->vel.y * dt;
    c->grounded = false;

    SupQ q = {.x = c->pos.x, .z = c->pos.z, .r = c->radius * 0.6f, .best = phys_ground(w, c->pos.x, c->pos.z), .ceil = 1e9f,
              .feet = prev_y, .head = prev_y + c->height};
    /* ceiling: only relevant while rising */
    if (c->vel.y > 0) {
        q.max_top = -1e9f;
        query(w, q.x - q.r, q.z - q.r, q.x + q.r, q.z + q.r, support_item, &q);
        if (q.ceil < 1e8f && c->pos.y + c->height > q.ceil) { c->pos.y = q.ceil - c->height; c->vel.y = 0; }
        return;
    }
    q.max_top = prev_y + (was_grounded ? c->step_h : 0.0f) + 1e-3f;
    q.best = phys_ground(w, q.x, q.z);
    query(w, q.x - q.r, q.z - q.r, q.x + q.r, q.z + q.r, support_item, &q);
    float s = q.best;
    if (c->pos.y <= s + 1e-3f) { c->pos.y = s; c->vel.y = 0; c->grounded = true; }
    else if (was_grounded && !jumped && c->pos.y - s <= c->step_h) { c->pos.y = s; c->vel.y = 0; c->grounded = true; } /* walk down stairs */
}
