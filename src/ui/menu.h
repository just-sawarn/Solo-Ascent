#ifndef SA_MENU_H
#define SA_MENU_H
/* Pause menu: Items, Gear, Craft, Smith, Shop. Mouse / touch (tap rows and buttons, wheel or drag to scroll) and keyboard
 * (Tab = next tab, arrows = select / switch member, Enter = main action, Esc or I = close). */
#include <stdbool.h>
#include "core/inventory.h"
#include "core/party.h"

typedef struct {
    bool open;
    int tab, sel, scroll, member, shop, sell_mode, station, enchant_sel;
    char msg[112]; float msg_t;
    bool gear_dirty;           /* inventory/equipment changed: the game must re-apply gear */
} Menu;

typedef struct {
    Inventory *inv; const ItemDB *db; const SkillDB *skills; PartyDB *pdb;
    Party *party; CombatWorld *cw;
    int gear_level;            /* level used for equip/craft requirements (sandbox: everything unlocked) */
} MenuCtx;

void menu_init(Menu *m);
void menu_toggle(Menu *m);
/* Handles input and draws. Call between BeginDrawing/EndDrawing while `m->open`. */
void menu_update_draw(Menu *m, MenuCtx *c);

#endif
