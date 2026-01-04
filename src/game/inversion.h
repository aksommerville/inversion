#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/font/font.h"
#include "util/res/res.h"
#include "util/text/text.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "sprite.h"
#include "modals.h"

#define FBW 160
#define FBH 96

#define FADE_OUT_TIME 0.333
#define FADE_IN_TIME  0.333
#define DEAD_TIME     2.000

extern struct g {
  void *rom;
  int romc;
  struct rom_entry *resv;
  int resc,resa;
  struct graf graf;
  int texid_tiles;
  int texid_font;
  int texid_bgbits;
  int input,pvinput;
  uint8_t map[NS_sys_mapw*NS_sys_maph]; // NS_physics_*, NB not tiles.
  int mapid;
  struct sprite **spritev;
  int spritec,spritea;
  uint8_t gravity; // Cardinal bit: (0x40,0x10,0x08,0x02)=(N,W,E,S)
  int song_playing;
  int on_goal; // Global layer clears, hero sets, global checks at the end of each frame.
  double goalclock;
  double fadeinclock;
  double deadclock;
  uint8_t song_lead; // NS_physics_vacant or NS_physics_solid, whichever is passable to the hero.
  int pause_jump_blackout; // modal_pause and modal_hello set, sprite_type_hero clears.
  
  // Non-game modes, in precedence order:
  struct hello hello;
  struct gameover gameover;
  struct pause pause;
} g;

int game_reset(int mapid); // Do not call during sprite updates!
void game_update(double elapsed);
void game_render();

int res_search(int tid,int rid);
int res_get(void *dstpp,int tid,int rid);

void inv_sound(int rid);
void inv_song(int rid);
void inv_song_lead(uint8_t lead);

void gravity_reverse();
void gravity_rotate(int d); // <0=deasil, >0=clockwise

#endif
