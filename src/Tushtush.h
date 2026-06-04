#ifndef TUSHTUSH_H
#define TUSHTUSH_H

// ---------------------------------------------------------------------------
// Tushtush.h  —  Scripted animation-trigger object system ("tt_obj" / "tt_sobj")
// Original: C:\DevStudio\Projects\Crux\Tushtush.cpp
// RE offset: 0x0047eed0 – 0x004821f0  (47 functions)
// ---------------------------------------------------------------------------
//
// "Tushtush" is the internal codename for the game's scripted animation-trigger
// object system.  It is the glue between the ADVENT script engine, the TIMERS
// async-program scheduler and the animation / sprite-rescale renderer.
//
// There are two cooperating object types:
//
//   tt_obj  — a *definition* / spawner template (size 0x100+ bytes).  Created by
//             ADVENT verb tt_obj_add() via Timer_TriggerInit (constructor at
//             0x0047ed5e in TIMERS).  It owns a named animation (Anim_AddByName),
//             up to 10 spawn POS/RANGE rectangles, script callbacks to fire on
//             init / periodic / collision events, a spawn probability, and the
//             frame at which the periodic callback fires.  Definitions live in a
//             doubly-linked list (g_pTtObjList); ADVENT setter verbs operate on
//             the "current" definition pointed to by the cursor g_pTtObjCursor.
//
//   tt_sobj — a live *spawned instance* (size 0x24 bytes).  Constructed from a
//             tt_obj (Tt_SobjCtor), it picks a random position inside one of the
//             parent's POS/RANGE rectangles, advances its animation frame each
//             tick, draws via Rescale_DrawScaledSprite, and fires the parent's
//             init / periodic / collision callbacks through Timer_AddAsyncProg.
//             Live instances live in a second doubly-linked list (g_pTtSobjList)
//             and are reaped when they go "over the hill" (lifetime expired).
//
// Per-frame driver:  Tt_Handler is registered as an Anim tick callback in
// Tt_Init.  Each frame it walks g_pTtSobjList (show + advance + collision-test
// the character sobj + reap expired ones) and walks g_pTtObjList (rolling
// spawn-probability dice to birth new sobjs).
//
// ---------------------------------------------------------------------------
// tt_obj layout (definition / spawner template)
// ---------------------------------------------------------------------------
//   [+0x04] char*   name           — object name (set by ctor, matched by is_it)
//   [+0x08] int     animHandle     — Anim_AddByName handle (Anim_Free'd in dtor)
//   [+0x0c] int     posX[10]       — spawn-rect left edges   (Tt_SetPos)
//   [+0x34] int     posY[10]       — spawn-rect top edges    (Tt_SetPos)
//   [+0x58] int     rangeW[10]     — spawn-rect widths       (Tt_SetRange, >=1)
//   [+0x5c] int     rangeH[10]     ... (interleaved with rangeW per index *4)
//   [+0x80] int     ...            — RANGE/width storage continues
//   [+0x84] int     ...            — default span = 1
//   [+0xac] int     posCount       — number of POS rects defined (max 10)
//   [+0xb0] int     initScript     — script id fired when sobj spawns (-1 = none)
//   [+0xb4] int     periodicScript — script id fired periodically (-1 = none)
//   [+0xb8] int     periodicFrame  — anim frame countdown for periodic fire
//                                     (negative arg => relative to anim length)
//   [+0xbc] int     collisionScript— script id fired on collision (-1 = none)
//   [+0xc0] int     probNum        — spawn probability numerator   (Tt_SetPers)
//   [+0xc4] int     probDen        — spawn probability denominator (Tt_SetPers)
//
// ---------------------------------------------------------------------------
// tt_sobj layout (live spawned instance, 0x24 bytes)
// ---------------------------------------------------------------------------
//   [+0x04] int     x              — world x
//   [+0x08] int     y              — world y
//   [+0x0c] int     fixedPos       — 1 = no POS defined, draw at anim default
//   [+0x10] int     periodicCount  — frames until periodic callback fires
//   [+0x14] int     flag14         — init flag (set to 1 in ctor)
//   [+0x18] int     birthFrame     — Rescale frame count tracking (life/anim)
//   [+0x1c] int     animFrame      — current animation frame index
//   [+0x20] tt_obj* parent         — owning definition
//
// ---------------------------------------------------------------------------

class tt_obj;
class tt_sobj;

// --- tt_obj methods (__thiscall) -------------------------------------------
void  Tt_ObjDtor(tt_obj *self);                              // 0x0047eed0  ~tt_obj
void  Tt_SetInitScript(tt_obj *self, int script);            // 0x0047ef70  set_init
void  Tt_SetPeriodicScript(tt_obj *self, int script, int frame); // 0x0047f010 set_periodic
void  Tt_SetCollisionScript(tt_obj *self, int script);       // 0x0047f0e0  set_collision
void  Tt_SetPos(tt_obj *self, int x, int y);                 // 0x0047f180  set_pos
void  Tt_SetRange(tt_obj *self, int x2, int y2);             // 0x0047f2c0  set_range
void  Tt_SetPers(tt_obj *self, int num, int den);            // 0x0047f450  set_pers
bool  Tt_ObjIsIt(tt_obj *self, const char *name);            // 0x0047f500  is_it
bool  Tt_CheckProb(tt_obj *self);                            // 0x0047f5b0  check_prob

// --- tt_sobj methods (__thiscall) ------------------------------------------
tt_sobj *Tt_SobjCtor(tt_sobj *self, tt_obj *parent);         // 0x0047f670  tt_sobj()
void  Tt_SobjDtor(tt_sobj *self);                            // 0x0047f840  ~tt_sobj
void  Tt_SobjShow(tt_sobj *self);                            // 0x0047f850  show
void  Tt_SobjGetRect(tt_sobj *self, int *l, int *t, int *r, int *b); // 0x0047fa10 get_rect
void  Tt_SobjAdvanceFrame(tt_sobj *self);                    // 0x0047fb60  get_rect(void)
bool  Tt_SobjOverTheHill(tt_sobj *self);                     // 0x0047fc00  over_the_hill
void  Tt_SobjCheckCollision(tt_sobj *self);                  // 0x0047fca0  check_collision

// --- per-frame driver / lifecycle ------------------------------------------
void  Tt_Handler(void);                                      // 0x0047fdf0  tt_handler
void  Tt_Cleanup(void);                                      // 0x00480200  tt_cleanup
void  Tt_Init(int zoomA, int zoomB);                         // 0x00480460  tt_init

// --- ADVENT verb wrappers (operate on the cursor "current" tt_obj) ---------
void  Tt_ObjAdd(const char *name);                           // 0x00480670  tt_obj_add
void  Tt_ObjSetCur(const char *name);                        // 0x004808d0  tt_obj_set_cur
void  Tt_ObjSetInitScript(int script);                       // 0x00480a00  tt_obj_set_init_script
void  Tt_ObjSetCollisionScript(int script);                  // 0x00480aa0  tt_obj_set_collision_script
void  Tt_ObjSetPeriodicScript(int script, int frame);        // 0x00480b40  tt_obj_set_periodic_script
void  Tt_ObjSetPos(int x, int y);                            // 0x00480bf0  tt_obj_set_pos
void  Tt_ObjSetRange(int x2, int y2);                        // 0x00480ca0  tt_obj_set_range
void  Tt_ObjSetPers(int num, int den);                       // 0x00480d50  tt_obj_set_pers
void  Tt_ObjRem(const char *name);                           // 0x00480e00  tt_obj_rem

// --- tt_sobj spawn / list management ---------------------------------------
tt_sobj *Tt_SobjAddByItr(void *itr);                         // 0x00481220  tt_sobj_add_by_itr
void  Tt_SobjAdd(void);                                      // 0x00481470  tt_sobj_add
tt_sobj *Tt_SobjInsert(void *before);                        // 0x00481500  tt_sobj_insert
tt_sobj *Tt_SobjAppend(void *after);                         // 0x00481770  tt_sobj_append
void  Tt_SobjRemove(tt_sobj *self);                          // 0x004819f0  tt_sobj_remove
tt_sobj *Tt_GetSobj(tt_sobj *self);                          // 0x00481af0  tt_get_sobj
void  Tt_SobjLink(tt_sobj *dst, tt_sobj *src, int dx, int dy); // 0x00481b70 tt_sobj_link

// --- "character" sobj (player-driven) --------------------------------------
void  Tt_CharSet(const char *name);                          // 0x00481c70  tt_char_set
void  Tt_CharRemove(void);                                   // 0x00481ef0  tt_char_remove
int   Tt_GetChar(void);                                      // 0x00481fe0  get_char (pumps msg loop)
int   Tt_GetCharResult(void);                                // 0x004820c0  (peek g_nTtGetCharResult)

// --- CD / advent-dir discovery ---------------------------------------------
void  Tt_SetAdventDir(void);                                 // 0x00482150
void  Tt_CdFind(void *path);                                 // 0x004821f0  cd_find

#endif // TUSHTUSH_H
