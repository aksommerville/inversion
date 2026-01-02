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

#define FBW 160
#define FBH 96

extern struct g {
  void *rom;
  int romc;
  struct rom_entry *resv;
  int resc,resa;
  struct graf graf;
  int texid_tiles;
  int texid_bgbits;
  int input,pvinput;
  uint8_t map[NS_sys_mapw*NS_sys_maph]; // NS_physics_*, NB not tiles.
} g;

void game_reset(int mapid);
void game_update(double elapsed);
void game_render();

int res_get(void *dstpp,int tid,int rid);

#endif
