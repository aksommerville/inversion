#include "inversion.h"

#define OPT_RESUME 1
#define OPT_RESTART 2
#define OPT_SKIP 3
#define OPT_MENU 4

/* End.
 */
 
void pause_end() {
  g.pause.active=0;
}

/* Begin.
 */
 
void pause_begin() {
  g.pause.active=1;
  if (!g.pause.labels.c) {
  
    label_new(&g.pause.labels,"- PAUSED -",-1,0xffffffff,0);
    label_new(&g.pause.labels,"",0,0,0);
    label_new(&g.pause.labels,"Resume",-1,0xffffffff,OPT_RESUME);
    label_new(&g.pause.labels,"Restart Level",-1,0xffffffff,OPT_RESTART);
    label_new(&g.pause.labels,"Skip Level",-1,0xffffffff,OPT_SKIP);
    label_new(&g.pause.labels,"Main Menu",-1,0xffffffff,OPT_MENU);
    
    label_list_center_all(&g.pause.labels);
  }
  g.pause.optp=2;
  g.pause_jump_blackout=1;
}

/* Activate selection.
 */
 
static void pause_activate() {
  if ((g.pause.optp<0)||(g.pause.optp>=g.pause.labels.c)) return;
  int optid=g.pause.labels.v[g.pause.optp].id;
  switch (optid) {
    case OPT_RESUME: pause_end(); break;
    case OPT_RESTART: pause_end(); game_reset(g.mapid); break;
    case OPT_SKIP: pause_end(); game_reset(g.mapid+1); break;
    case OPT_MENU: pause_end(); hello_begin(); break;
  }
}

/* Move cursor.
 */
 
static void pause_move(int d) {
  int panic=g.pause.labels.c;
  while (panic-->0) {
    g.pause.optp+=d;
    if (g.pause.optp<0) g.pause.optp=g.pause.labels.c-1;
    else if (g.pause.optp>=g.pause.labels.c) g.pause.optp=0;
    struct label *label=g.pause.labels.v+g.pause.optp;
    if (label->id) {
      inv_sound(RID_sound_uimotion);
      return;
    }
  }
}

/* Update.
 */
 
void pause_update(double elapsed) {
  if ((g.input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)) pause_move(-1);
  if ((g.input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)) pause_move(1);
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) pause_activate();
}

/* Render.
 */
 
void pause_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000c0);
  
  if ((g.pause.optp>=0)&&(g.pause.optp<g.pause.labels.c)) {
    struct label *label=g.pause.labels.v+g.pause.optp;
    graf_fill_rect(&g.graf,label->x-5,label->y-5,label->srcc*8+1,9,0x000040ff);
  }
  
  label_list_render(&g.pause.labels);
}
