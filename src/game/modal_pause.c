#include "inversion.h"

#define OPT_RESUME 1
#define OPT_RESTART 2
#define OPT_SKIP 3
#define OPT_MENU 4

/* End.
 */
 
void pause_end() {
  g.pause.active=0;
  if (g.song_playing==RID_song_inversion) {
    if (g.song_lead==NS_physics_vacant) {
      egg_song_set(1,1,EGG_SONG_PROP_TRIM,0.388f);
    } else {
      egg_song_set(1,2,EGG_SONG_PROP_TRIM,0.286f);
    }
    egg_song_set(1,3,EGG_SONG_PROP_TRIM,0.470f);
    egg_song_set(1,5,EGG_SONG_PROP_TRIM,0.243f);
    egg_song_set(1,6,EGG_SONG_PROP_TRIM,0.215f);
  }
}

/* Begin.
 */
 
void pause_begin() {
  g.pause.active=1;
  
  egg_song_set(1,1,EGG_SONG_PROP_TRIM,0.0f);
  egg_song_set(1,2,EGG_SONG_PROP_TRIM,0.0f);
  egg_song_set(1,3,EGG_SONG_PROP_TRIM,0.0f);
  egg_song_set(1,5,EGG_SONG_PROP_TRIM,0.0f);
  egg_song_set(1,6,EGG_SONG_PROP_TRIM,0.0f);
  
  if (!g.pause.labels.c) {
  
    label_new(&g.pause.labels,"- PAUSED -",-1,0xe0e0f0ff,0);
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
    case OPT_MENU: hello_begin(); pause_end(); break;
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
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x204010c0);
  
  if ((g.pause.optp>=0)&&(g.pause.optp<g.pause.labels.c)) {
    struct label *label=g.pause.labels.v+g.pause.optp;
    graf_fill_rect(&g.graf,label->x-5,label->y-5,label->srcc*8+1,9,0x000000ff);
  }
  
  label_list_render(&g.pause.labels);
}
