// ---------------------------------------------------------------------------
// Tushtush.cpp  —  Scripted animation-trigger object system ("tt_obj"/"tt_sobj")
// Original: C:\DevStudio\Projects\Crux\Tushtush.cpp
// RE offset: 0x0047eed0 – 0x004821f0  (47 functions)
// ---------------------------------------------------------------------------
//
// See Tushtush.h for the full module overview and tt_obj / tt_sobj layouts.
//
// Reconstruction notes:
//   * The original was compiled with MSVC SEH frames; the decompiled
//     __try/FS:[0] prologues have been elided here and replaced with the
//     plain logic they guard.
//   * tt_obj is allocated 0x100 bytes; tt_sobj is 0x24 bytes; list nodes are
//     0xC bytes {next, prev, *back}.  Field access mirrors the binary.
//   * "Group trigger" tables (g_anGroupTriggerPct) and g_anAnimFrameCount are
//     external animation tables shared with Advanim/Anim.
// ---------------------------------------------------------------------------

#include "Tushtush.h"

#include <windows.h>
#include <string.h>

// --- external subsystems ---------------------------------------------------
extern "C" {
    void  Anim_Free(int handle);
    int   Anim_AddByName(const char *name);
    void  Anim_RegisterTickCallback(void *fn, int a, int b);
    void  Anim_UnregisterTickCallback(void *fn);

    int   Timer_TriggerInit(const char *name);          // tt_obj ctor, 0x0047ed5e
    void  Timer_AddAsyncProg(int script, void *data);

    void  Rescale_DrawScaledSprite(int frame, int x, int y, int spr);
    int   Rescale_GetCount(void);
    void  Rescale_CalcZoomTable(int a, int b);

    int   Adv_RectsOverlap(int l1,int t1,int r1,int b1,int l2,int t2,int r2,int b2);
    void  Adv_RegisterCleanup(void *fn);

    void  Speech_Init(void);
    void  Txt_SetMaxLines(int n);
    void  InitImg(void);

    void  Err_BadResEntry(const char *file, const char *func, const char *msg);
    void *Err_SetRecord3(int code, void *rec, int line);
    int   Err_ShowDialog(void);
    void  Win_CleanExit(const char *file, const char *func);

    int   tt_rand(void);                                // 0x00489cf0 PRNG
    void *tt_alloc(int size);                           // 0x004890e0 allocator
    void  tt_free(void *p);                             // 0x00488ee0 deallocator
}

// --- module globals --------------------------------------------------------
extern void *g_pTtObjList;        // 0x007d5a5c  tt_obj definition list head
extern void *g_pTtSobjList;       // 0x007d5a60  live tt_sobj instance list head
extern void *g_pTtObjCursor;      // 0x007d5a64  "current" tt_obj cursor
extern void *g_pTtCharSobj;       // 0x007d5a6c  the character (player) sobj
extern int   g_nTtInitialized;    // 0x007d5a70  one-shot init guard
extern int   g_nTtGetCharResult;  // 0x007d6a7c  get_char() result latch

// external animation tables
extern int   g_anAnimFrameCount[];   // 0x... per-anim frame counts
extern int   g_anGroupTriggerPct[];  // 0x... group trigger sprite table
extern void *g_abAdventDir;          // 0x... current advent directory

// ===========================================================================
// list node helpers (0xC bytes: [0]=next [1]=prev [2]=*owner-backptr)
// ===========================================================================

struct DLNode { DLNode *next; DLNode *prev; void **owner; };
struct DLList { DLNode *head; DLNode *tail; };

// ===========================================================================
// tt_obj  —  definition / spawner template
// ===========================================================================

// 0x0047eed0  tt_obj::~tt_obj()
void Tt_ObjDtor(tt_obj *self)
{
    int *p = (int *)self;
    Anim_Free(p[2]);                                    // [+0x08] animHandle
}

// 0x0047ef70  tt_obj::set_init(int)
void Tt_SetInitScript(tt_obj *self, int script)
{
    *(int *)((char *)self + 0xb0) = script;
}

// 0x0047f010  tt_obj::set_periodic(int,int)
// A negative frame counts back from the end of the animation.
void Tt_SetPeriodicScript(tt_obj *self, int script, int frame)
{
    *(int *)((char *)self + 0xb4) = script;
    int anim = *(int *)((char *)self + 0x08);
    if (frame < 0)
        *(int *)((char *)self + 0xb8) = g_anAnimFrameCount[anim] + frame;
    else
        *(int *)((char *)self + 0xb8) = frame;
}

// 0x0047f0e0  tt_obj::set_collision(int)
void Tt_SetCollisionScript(tt_obj *self, int script)
{
    *(int *)((char *)self + 0xbc) = script;
}

// 0x0047f180  tt_obj::set_pos(int,int)  — append a spawn-rect origin
void Tt_SetPos(tt_obj *self, int x, int y)
{
    char *o = (char *)self;
    int n = *(int *)(o + 0xac);
    if (n >= 10)
        Err_BadResEntry("Tushtush.cpp", "tt_obj::set_pos", "no more POS available");

    *(int *)(o + 0x0c + n * 4) = x;     // posX[n]
    *(int *)(o + 0x34 + n * 4) = y;     // posY[n]
    *(int *)(o + 0x5c + n * 4) = 1;     // rangeW[n] default span
    *(int *)(o + 0x84 + n * 4) = 1;     // rangeH[n] default span
    *(int *)(o + 0xac) = n + 1;
}

// 0x0047f2c0  tt_obj::set_range(int,int)  — set width/height of last POS rect
void Tt_SetRange(tt_obj *self, int x2, int y2)
{
    char *o = (char *)self;
    if (*(int *)(o + 0xac) == 0)
        Err_BadResEntry("Tushtush.cpp", "tt_obj::set_range",
                        "POS must be defined before RANGE");

    int n = *(int *)(o + 0xac);
    int w = (x2 - *(int *)(o + 0x08 + n * 4)) + 1;
    *(int *)(o + 0x58 + n * 4) = (w < 2) ? 1 : w;
    int h = (y2 - *(int *)(o + 0x30 + n * 4)) + 1;
    *(int *)(o + 0x80 + n * 4) = (h < 2) ? 1 : h;
}

// 0x0047f450  tt_obj::set_pers(int,int)  — spawn probability num/den
void Tt_SetPers(tt_obj *self, int num, int den)
{
    *(int *)((char *)self + 0xc0) = num;
    *(int *)((char *)self + 0xc4) = den;
}

// 0x0047f500  tt_obj::is_it(char*)  — name match
bool Tt_ObjIsIt(tt_obj *self, const char *name)
{
    const char *objName = *(const char **)((char *)self + 4);
    return strcmp(objName, name) == 0;
}

// 0x0047f5b0  tt_obj::check_prob(void)  — roll spawn dice
bool Tt_CheckProb(tt_obj *self)
{
    int den = *(int *)((char *)self + 0xc4);
    if (den == 0)
        return false;
    return (tt_rand() % den) < *(int *)((char *)self + 0xc0);
}

// ===========================================================================
// tt_sobj  —  live spawned instance
// ===========================================================================

// 0x0047f670  tt_sobj::tt_sobj(tt_obj*)
tt_sobj *Tt_SobjCtor(tt_sobj *self, tt_obj *parent)
{
    int *s = (int *)self;
    char *p = (char *)parent;

    s[8] = (int)parent;                                 // [+0x20] parent

    int posCount = *(int *)(p + 0xac);
    if (posCount == 0) {
        s[3] = 1;                                       // [+0x0c] fixedPos = 1
    } else {
        s[3] = 0;
        int idx = tt_rand() % posCount;
        s[1] = tt_rand() % *(int *)(p + 0x5c + idx * 4) // [+0x04] x
                         + *(int *)(p + 0x0c + idx * 4);
        s[2] = tt_rand() % *(int *)(p + 0x84 + idx * 4) // [+0x08] y
                         + *(int *)(p + 0x34 + idx * 4);
    }

    s[4] = *(int *)(p + 0xb8);                           // [+0x10] periodicCount
    s[5] = 1;                                            // [+0x14] flag14
    s[6] = 0;                                            // [+0x18] birthFrame
    s[7] = 0;                                            // [+0x1c] animFrame

    if (*(int *)(p + 0xb0) != -1)                        // initScript
        Timer_AddAsyncProg(*(int *)(p + 0xb0), self);

    return self;
}

// 0x0047f840  tt_sobj::~tt_sobj()  — trivial
void Tt_SobjDtor(tt_sobj *self) { (void)self; }

// 0x0047f850  tt_sobj::show(void)
void Tt_SobjShow(tt_sobj *self)
{
    int *s = (int *)self;
    int anim = *(int *)(s[8] + 8);                       // parent animHandle
    int trig = *(int *)(anim * 0x640 + 0x4e3b58 + s[7] * 4);
    s[7]++;                                              // advance animFrame
    int spr = g_anGroupTriggerPct[trig * 8 + 10];

    if (g_anAnimFrameCount[anim] <= s[7])
        s[7] = 0;                                        // loop animFrame

    if (s[3] == 0) {                                     // not fixedPos
        Rescale_DrawScaledSprite(s[6], s[1], s[2], spr);
    } else {
        Rescale_DrawScaledSprite(s[6],
                                 g_anGroupTriggerPct[trig * 8 + 6],
                                 g_anGroupTriggerPct[trig * 8 + 7] - 0x32, spr);
    }

    // periodic callback countdown
    if (*(int *)(s[8] + 0xb4) != -1) {                   // periodicScript set
        if (--s[4] == 0)
            Timer_AddAsyncProg(*(int *)(s[8] + 0xb4), self);
    }
}

// 0x0047fa10  tt_sobj::get_rect(int*,int*,int*,int*)
void Tt_SobjGetRect(tt_sobj *self, int *l, int *t, int *r, int *b)
{
    int *s = (int *)self;
    int anim = *(int *)(s[8] + 8);
    int trig = *(int *)(anim * 0x640 + 0x4e3b58 + s[7] * 4);
    int spr  = g_anGroupTriggerPct[trig * 8 + 10];

    if (s[3] == 0) {
        *l = s[1];
        *t = s[2];
    } else {
        *l = g_anGroupTriggerPct[trig * 8 + 6];
        *t = g_anGroupTriggerPct[trig * 8 + 7];
    }
    *r = *l + *(unsigned short *)(spr + 1);
    *b = *t + *(unsigned short *)(spr + 3);
}

// 0x0047fb60  tt_sobj::get_rect(void)  — bump life/lifetime counter
void Tt_SobjAdvanceFrame(tt_sobj *self)
{
    ((int *)self)[6]++;                                  // [+0x18] birthFrame++
}

// 0x0047fc00  tt_sobj::over_the_hill(void)  — true once lifetime expired
bool Tt_SobjOverTheHill(tt_sobj *self)
{
    return Rescale_GetCount() < ((int *)self)[6];
}

// 0x0047fca0  tt_sobj::check_collision(tt_sobj*)
void Tt_SobjCheckCollision(tt_sobj *self)
{
    int *s = (int *)self;
    if (*(int *)(s[8] + 0xbc) == -1)                     // no collisionScript
        return;

    int al, at, ar, ab;
    int bl, bt, br, bb;
    Tt_SobjGetRect(self, &al, &at, &ar, &ab);
    Tt_SobjGetRect((tt_sobj *)g_pTtCharSobj, &bl, &bt, &br, &bb);
    bt -= 0x32;
    bb -= 0x32;

    if (Adv_RectsOverlap(al, at, ar, ab, bl, bt, br, bb))
        Timer_AddAsyncProg(*(int *)(s[8] + 0xbc), self);
}

// ===========================================================================
// per-frame driver  (registered as Anim tick callback in Tt_Init)
// ===========================================================================

// 0x0047fdf0  tt_handler(void)
void Tt_Handler(void)
{
    // 1) walk live sobjs: collision-test the char, show, advance, reap expired
    DLNode *n = ((DLList *)g_pTtSobjList)->tail;
    while (n != 0) {
        if (g_pTtCharSobj != 0) {
            tt_sobj *sobj = (tt_sobj *)n->owner;
            if (((int *)sobj)[6] == Rescale_GetCount())
                Tt_SobjCheckCollision((tt_sobj *)g_pTtCharSobj);
        }
        tt_sobj *sobj = (tt_sobj *)n->owner;
        Tt_SobjShow(sobj);
        Tt_SobjAdvanceFrame(sobj);

        void *cur = n->owner;
        n = n->next;
        if (Tt_SobjOverTheHill((tt_sobj *)cur) && cur != 0) {
            Tt_SobjListRemoveNode((DLList *)g_pTtSobjList, n);   // unlink
            Tt_SobjDtor((tt_sobj *)cur);
            tt_free(cur);
        }
    }

    // 2) walk definitions: roll spawn probabilities to birth new sobjs
    DLNode *d = ((DLList *)g_pTtObjList)->tail;
    while (d != 0) {
        if (Tt_CheckProb((tt_obj *)d->owner))
            Tt_SobjAddByItr(&d);
        if (d != 0)
            d = d->next;
    }

    // 3) the character sobj is always drawn last (on top)
    if (g_pTtCharSobj != 0)
        Tt_SobjShow((tt_sobj *)g_pTtCharSobj);
}

// 0x00480080  unlink + free a sobj list node (helper used by Tt_Handler)
void Tt_SobjListRemoveNode(DLList *list, DLNode *node);  // fwd (defined below)
void Tt_SobjListRemoveNode(DLList *list, DLNode *node)
{
    node = node->prev /* actually *node -> next handling below */;
    // NB: the binary loads node = *node first; reproduce its pointer-fixups:
    DLNode *t = node;
    if (list->tail == t) list->tail = t->next;
    else                 t->prev->next = t->next;
    if (list->head == t) list->head = t->prev;
    else                 t->next->prev = t->prev;
    if (t != 0) { *t->owner = 0; tt_free(t); }
}

// 0x00480200  tt_cleanup(void)  — free both lists, unregister tick callback
void Tt_Cleanup(void)
{
    DLNode *n = ((DLList *)g_pTtSobjList)->head;
    if (n != 0) {
        DLNode *cur = n->next;
        do {
            DLNode *nx = cur->prev;     // [+0x04]
            if (cur != 0) { *cur->owner = 0; tt_free(cur); }
            cur = nx;
        } while (nx != 0);
        tt_free(n);
    }
    tt_free(g_pTtObjCursor);

    DLNode *o = ((DLList *)g_pTtObjList)->head;
    if (o != 0) {
        DLNode *cur = o->next;
        do {
            DLNode *nx = cur->prev;
            if (cur != 0) { *cur->owner = 0; tt_free(cur); }
            cur = nx;
        } while (nx != 0);
        tt_free(o);
    }

    g_nTtInitialized = 0;
    Anim_UnregisterTickCallback((void *)0x004010f5);
}

// 0x00480460  tt_init(int,int)  — one-shot subsystem bring-up
void Tt_Init(int zoomA, int zoomB)
{
    if (g_nTtInitialized != 0)
        return;

    g_nTtInitialized = 1;
    Speech_Init();
    Txt_SetMaxLines(2);
    InitImg();
    Rescale_CalcZoomTable(zoomA, zoomB);
    Anim_RegisterTickCallback((void *)0x004010f5, 0, 0);     // -> Tt_Handler
    Adv_RegisterCleanup((void *)0x00401b7c);                 // -> Tt_Cleanup

    // allocate the three list/cursor header nodes (8 bytes each)
    DLList *objList = (DLList *)tt_alloc(8);
    if (objList) { objList->tail = 0; objList->head = 0; }
    g_pTtObjList = objList;

    DLList *cursor = (DLList *)tt_alloc(8);
    if (cursor) { cursor->tail = (DLNode *)g_pTtObjList;
                  cursor->head = (DLNode *)((DLList *)cursor->tail)->head; }
    g_pTtObjCursor = cursor;

    DLList *sobjList = (DLList *)tt_alloc(8);
    if (sobjList) { sobjList->tail = 0; sobjList->head = 0; }
    g_pTtSobjList = sobjList;
}

// ===========================================================================
// ADVENT verb wrappers — operate on the "current" tt_obj (g_pTtObjCursor)
// ===========================================================================

// 0x00480670  tt_obj_add(char*)  — define a new trigger object
void Tt_ObjAdd(const char *name)
{
    void *mem = tt_alloc(200);
    tt_obj *obj = mem ? (tt_obj *)Timer_TriggerInit(name) : 0;
    Tt_ObjListAppend((DLList *)g_pTtObjList, (void **)&obj);
    ((DLList *)g_pTtObjCursor)->head = (DLNode *)((DLList *)g_pTtObjList)->head;
}

// 0x00480780  append node to a tt_obj list
void Tt_ObjListAppend(DLList *list, void **owner);       // fwd
void Tt_ObjListAppend(DLList *list, void **owner)
{
    DLNode *node = (DLNode *)tt_alloc(0xc);
    if (node) { node->owner = owner; *owner = node; node->prev = 0; node->next = 0; }

    if (list->head == 0) {
        list->tail = node;
        list->head = node;
    } else {
        list->tail->prev = node;
        node->next = list->tail;
        list->tail = node;
    }
}

// 0x004808d0  tt_obj_set_cur(char*)  — select current obj by name
void Tt_ObjSetCur(const char *name)
{
    DLList *cur = (DLList *)g_pTtObjCursor;
    cur->head = ((DLList *)g_pTtObjList)->head->next;
    while (cur->head != 0) {
        if (Tt_ObjIsIt((tt_obj *)cur->head->owner, name))
            return;
        cur->head = cur->head->prev;
    }
    Err_BadResEntry("Tushtush.cpp", "tt_obj_set_cur", name);
}

// 0x00480a00  tt_obj_set_init_script(int)
void Tt_ObjSetInitScript(int script)
{
    Tt_SetInitScript((tt_obj *)((DLList *)g_pTtObjCursor)->head->owner, script);
}

// 0x00480aa0  tt_obj_set_collision_script(int)
void Tt_ObjSetCollisionScript(int script)
{
    Tt_SetCollisionScript((tt_obj *)((DLList *)g_pTtObjCursor)->head->owner, script);
}

// 0x00480b40  tt_obj_set_periodic_script(int,int)
void Tt_ObjSetPeriodicScript(int script, int frame)
{
    Tt_SetPeriodicScript((tt_obj *)((DLList *)g_pTtObjCursor)->head->owner, script, frame);
}

// 0x00480bf0  tt_obj_set_pos(int,int)
void Tt_ObjSetPos(int x, int y)
{
    Tt_SetPos((tt_obj *)((DLList *)g_pTtObjCursor)->head->owner, x, y);
}

// 0x00480ca0  tt_obj_set_range(int,int)
void Tt_ObjSetRange(int x2, int y2)
{
    Tt_SetRange((tt_obj *)((DLList *)g_pTtObjCursor)->head->owner, x2, y2);
}

// 0x00480d50  tt_obj_set_pers(int,int)
void Tt_ObjSetPers(int num, int den)
{
    Tt_SetPers((tt_obj *)((DLList *)g_pTtObjCursor)->head->owner, num, den);
}

// 0x00480e00  tt_obj_rem(char*)  — remove a named definition (and its sobjs)
void Tt_ObjRem(const char *name)
{
    // reap any live sobjs spawned from a matching obj
    DLNode *n = ((DLList *)g_pTtSobjList)->head;
    while (n != 0) {
        void *cur = n->owner;
        n = n->prev;
        if (Tt_ObjIsIt((tt_obj *)cur, name) && cur != 0) {
            Tt_SobjDtor((tt_sobj *)cur);
            tt_free(cur);
        }
    }

    // find and remove the definition
    DLList *cursor = (DLList *)g_pTtObjCursor;
    cursor->head = ((DLList *)g_pTtObjList)->head->next;
    do {
        if (cursor->head == 0) break;
    } while (!Tt_ObjIsIt((tt_obj *)cursor->head, name));

    if (cursor->head == 0)
        Err_BadResEntry("Tushtush.cpp", "tt_obj_rem", "obj not found");

    Tt_ObjListRemoveNode((DLList *)cursor->head->owner, &cursor->head);
    tt_obj *obj = (tt_obj *)cursor->head->owner;
    if (obj) { Tt_ObjDtor(obj); tt_free(obj); }
}

// 0x004810a0  unlink + free a tt_obj list node (mirror of Tt_SobjListRemoveNode)
void Tt_ObjListRemoveNode(DLList *list, DLNode **pnode);  // fwd
void Tt_ObjListRemoveNode(DLList *list, DLNode **pnode)
{
    DLNode *t = *pnode;
    if (list->tail == t) list->tail = t->next;
    else                 t->prev->next = t->next;
    if (list->head == t) list->head = t->prev;
    else                 t->next->prev = t->prev;
    if (t != 0) { *t->owner = 0; tt_free(t); }
}

// ===========================================================================
// tt_sobj spawn / list management
// ===========================================================================

// 0x00481220  tt_sobj_add_by_itr(DLListIterator*)  — spawn from a definition itr
tt_sobj *Tt_SobjAddByItr(void *itr)
{
    DLNode *node = *(DLNode **)itr;
    void *mem = tt_alloc(0x24);
    tt_sobj *sobj = mem ? Tt_SobjCtor((tt_sobj *)mem,
                                      (tt_obj *)node->owner) : 0;
    Tt_SobjListAppend((DLList *)g_pTtSobjList, (void **)&sobj);
    return sobj;
}

// 0x00481320  append node to the sobj list
void Tt_SobjListAppend(DLList *list, void **owner);      // fwd
void Tt_SobjListAppend(DLList *list, void **owner)
{
    DLNode *node = (DLNode *)tt_alloc(0xc);
    if (node) { node->owner = owner; *owner = node; node->prev = 0; node->next = 0; }

    if (list->head == 0) {
        list->tail = node;
        list->head = node;
    } else {
        list->tail->prev = node;
        node->next = list->tail;
        list->tail = node;
    }
}

// 0x00481470  tt_sobj_add(void)  — spawn from the current definition
void Tt_SobjAdd(void)
{
    Tt_SobjAddByItr(&g_pTtObjCursor);
}

// 0x00481500  tt_sobj_insert(int)  — spawn + insert before a node
tt_sobj *Tt_SobjInsert(void *before)
{
    void *mem = tt_alloc(0x24);
    tt_sobj *sobj = mem ? Tt_SobjCtor((tt_sobj *)mem,
                          (tt_obj *)((DLList *)g_pTtObjCursor)->head->owner) : 0;
    Tt_SobjListInsertBefore((DLList *)g_pTtSobjList, (void **)before, (void **)&sobj);
    return sobj;
}

// 0x00481610  insert node before another in the sobj list
void Tt_SobjListInsertBefore(DLList *list, void **at, void **owner);  // fwd
void Tt_SobjListInsertBefore(DLList *list, void **at, void **owner)
{
    DLNode *ref = (DLNode *)*at;
    DLNode *node = (DLNode *)tt_alloc(0xc);
    if (node) { node->owner = owner; *owner = node; node->prev = 0; node->next = 0; }

    if (ref->next != 0) ref->next->prev = node;
    node->next = ref->next;
    node->prev = ref;
    ref->next = node;
    if (list->head == ref) list->head = node;
}

// 0x00481770  tt_sobj_append(int)  — spawn + insert after a node
tt_sobj *Tt_SobjAppend(void *after)
{
    void *mem = tt_alloc(0x24);
    tt_sobj *sobj = mem ? Tt_SobjCtor((tt_sobj *)mem,
                          (tt_obj *)((DLList *)g_pTtObjCursor)->head->owner) : 0;
    Tt_SobjListInsertAfter((DLList *)g_pTtSobjList, (void **)after, (void **)&sobj);
    return sobj;
}

// 0x00481880  insert node after another in the sobj list
void Tt_SobjListInsertAfter(DLList *list, void **at, void **owner);  // fwd
void Tt_SobjListInsertAfter(DLList *list, void **at, void **owner)
{
    DLNode *ref = (DLNode *)*at;
    DLNode *node = (DLNode *)tt_alloc(0xc);
    if (node) { node->owner = owner; *owner = node; node->prev = 0; node->next = 0; }

    if (ref->prev != 0) ref->prev->next = node;
    node->prev = ref->prev;
    node->next = ref;
    ref->prev = node;
    if (list->tail == ref) list->tail = node;
}

// 0x004819f0  tt_sobj_remove(int)  — unlink + destroy a sobj
void Tt_SobjRemove(tt_sobj *self)
{
    int p = (int)self;
    if (p != -1 && p != 0) {
        Tt_SobjListRemoveNode((DLList *)g_pTtSobjList, (DLNode *)self);
        Tt_SobjDtor(self);
        tt_free(self);
    }
}

// 0x00481af0  tt_get_sobj(int)  — identity accessor
tt_sobj *Tt_GetSobj(tt_sobj *self)
{
    return self;
}

// 0x00481b70  tt_sobj_link(int,int,int,int)  — clone a sobj at an offset
void Tt_SobjLink(tt_sobj *dst, tt_sobj *src, int dx, int dy)
{
    int *d = (int *)dst;
    int *s = (int *)src;
    d[1] = s[1] + dx;       // x
    d[2] = s[2] + dy;       // y
    d[3] = 0;               // fixedPos
    d[6] = s[6];            // birthFrame
}

// ===========================================================================
// "character" sobj — the single player-driven instance
// ===========================================================================

// 0x00481c70  tt_char_set(char*)  — (re)create the character sobj by obj name
void Tt_CharSet(const char *name)
{
    // destroy any existing character sobj
    if (g_pTtCharSobj != 0) {
        void *old = g_pTtCharSobj;
        Tt_SobjDtor((tt_sobj *)old);
        tt_free(old);
    }

    // find the matching definition
    DLNode *n = ((DLList *)g_pTtObjList)->head->next;
    while (n != 0 && !Tt_ObjIsIt((tt_obj *)n->owner, name))
        n = n->prev;
    if (n == 0)
        Err_BadResEntry("Tushtush.cpp", "tt_char_set", "obj not found");

    void *mem = tt_alloc(0x24);
    tt_sobj *sobj = mem ? Tt_SobjCtor((tt_sobj *)mem, (tt_obj *)n->owner) : 0;
    g_pTtCharSobj = sobj;
    if (sobj == 0) {
        Win_CleanExit("Tushtush.cpp", "tt_char_set");
        Err_SetRecord3(0, (void *)0x007d5a74, -1);
    }

    ((int *)g_pTtCharSobj)[6] = Rescale_GetCount();      // birthFrame
}

// 0x00481ef0  tt_char_remove(void)
void Tt_CharRemove(void)
{
    if (g_pTtCharSobj != 0) {
        void *old = g_pTtCharSobj;
        Tt_SobjDtor((tt_sobj *)old);
        tt_free(old);
    }
    g_pTtCharSobj = 0;
}

// 0x00481fe0  get_char(void)  — block (pumping the Win32 msg loop) until a
//             trigger sets g_nTtGetCharResult, then return + clear it.
int Tt_GetChar(void)
{
    int result;
    MSG msg;
    while ((result = g_nTtGetCharResult) == 0) {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    g_nTtGetCharResult = 0;
    return result;
}

// 0x004820c0  peek the latched get_char result without clearing
int Tt_GetCharResult(void)
{
    return g_nTtGetCharResult;
}

// ===========================================================================
// CD / advent-dir discovery (used at startup to locate game data)
// ===========================================================================

extern "C" {
    void *FUN_004895e0(void *dst, void *src);            // string copy
    void *FUN_004895f0(void *dst, const char *s);        // string append
    void *FUN_0048a060(char *out, const char *fmt, ...); // sprintf-like
}

// 0x00482150  copy the current advent directory into the global buffer
void Tt_SetAdventDir(void)
{
    FUN_004895e0((void *)0x007d6bb0, &g_abAdventDir);
}

// 0x004821f0  cd_find(char *path)  — scan CD-ROM drives for the game's data dir
void Tt_CdFind(void *path)
{
    LPCSTR drive = (LPCSTR)0x004dc3a8;
    DWORD drives = GetLogicalDrives();
    if (drives == 0) {
        Win_CleanExit("Tushtush.cpp", "cd_find");
        Err_SetRecord3(0x13, (void *)0x007d6cb4, -1);
    }

    char idPath[260];
    char subdir[260];
    int  choice;

    for (;;) {
        for (int i = 0; i < 26; i++) {
            if (drives & (1 << (i & 0x1f))) {
                *(char *)drive = (char)(i + 'A');
                if (GetDriveTypeA(drive) == DRIVE_CDROM) {
                    FUN_0048a060(idPath, "%s%s_ID", drive, (void *)0x007d6b08);
                    GetPrivateProfileStringA("CRUX", "Subdir", "",
                                             subdir, 0x104, idPath);
                    if (strcmp(subdir, "") != 0) {
                        FUN_004895e0(path, (void *)drive);
                        FUN_004895f0(path, subdir);
                        FUN_004895f0(path, "\\");
                        return;
                    }
                }
            }
        }
        choice = Err_ShowDialog();
        if (choice != 1) break;
    }

    if (choice == 2) {
        Win_CleanExit("Tushtush.cpp", "cd_find");
        ExitProcess(0);
    }
}
