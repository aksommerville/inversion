#include "inversion.h"

/* Nonzero if these two tiles are the same, for neighbor-joining purposes.
 */
 
static int sametile(uint8_t a,uint8_t b) {
  if (a==NS_physics_vacant) {
    if (b==NS_physics_goal) return 1; // Vacant doesn't recognize goal. (NOT vice-versa).
  }
  return (a==b);
}

/* Rewrite bgbits.
 * (g.map) is ready.
 */
 
static void draw_tile(int dstx,int dsty,const uint8_t *src,int col,int row) {

  // Everything past the first row is just a plain tile.
  if (*src>=0x10) {
    graf_tile(&g.graf,dstx,dsty,*src,0);
    return;
  }
  
  // Collect neighbor mask.
  int lx=col-1; if (lx<0) lx+=NS_sys_mapw;
  int rx=col+1; if (rx>=NS_sys_mapw) rx-=NS_sys_mapw;
  int ty=row-1; if (ty<0) ty+=NS_sys_maph;
  int by=row+1; if (by>=NS_sys_maph) by-=NS_sys_maph;
  uint8_t neighbors=0;
  if (sametile(*src,g.map[ty*NS_sys_mapw+lx])) neighbors|=0x80;
  if (sametile(*src,g.map[ty*NS_sys_mapw+col])) neighbors|=0x40;
  if (sametile(*src,g.map[ty*NS_sys_mapw+rx])) neighbors|=0x20;
  if (sametile(*src,g.map[row*NS_sys_mapw+lx])) neighbors|=0x10;
  if (sametile(*src,g.map[row*NS_sys_mapw+rx])) neighbors|=0x08;
  if (sametile(*src,g.map[by*NS_sys_mapw+lx])) neighbors|=0x04;
  if (sametile(*src,g.map[by*NS_sys_mapw+col])) neighbors|=0x02;
  if (sametile(*src,g.map[by*NS_sys_mapw+rx])) neighbors|=0x01;
  
  // Each of the special tiles in the top row kind of does its own thing.
  switch (*src) {
  
    // Vacant or solid: Use 4 subtiles, which are fully symmetic, so just 4 sources.
    case 0x00: case 0x01: {
        uint8_t tileid=(*src==0x00)?0x30:0x40;
        // NW quadrant.
             if ((neighbors&0x50)==0x50) graf_tile(&g.graf,dstx,dsty,tileid,0); // Full. We don't care about the diagonal neighbor.
        else if ((neighbors&0x90)==0x90) graf_tile(&g.graf,dstx,dsty,tileid+3,0); // Concave, upward.
        else if ((neighbors&0xc0)==0xc0) graf_tile(&g.graf,dstx,dsty,tileid+3,EGG_XFORM_SWAP); // Concave, leftward.
        else if (neighbors&0x10) graf_tile(&g.graf,dstx,dsty,tileid+2,0); // Horizontal.
        else if (neighbors&0x40) graf_tile(&g.graf,dstx,dsty,tileid+2,EGG_XFORM_SWAP); // Vertical.
        else graf_tile(&g.graf,dstx,dsty,tileid+1,0); // Convex.
        // NE quadrant.
             if ((neighbors&0x48)==0x48) graf_tile(&g.graf,dstx,dsty,tileid,EGG_XFORM_XREV); // Full.
        else if ((neighbors&0x28)==0x28) graf_tile(&g.graf,dstx,dsty,tileid+3,EGG_XFORM_XREV); // Concave, upward.
        else if ((neighbors&0x60)==0x60) graf_tile(&g.graf,dstx,dsty,tileid+3,EGG_XFORM_YREV|EGG_XFORM_SWAP); // Concave, rightward.
        else if (neighbors&0x08) graf_tile(&g.graf,dstx,dsty,tileid+2,EGG_XFORM_XREV); // Horizontal.
        else if (neighbors&0x40) graf_tile(&g.graf,dstx,dsty,tileid+2,EGG_XFORM_SWAP|EGG_XFORM_YREV); // Vertical.
        else graf_tile(&g.graf,dstx,dsty,tileid+1,EGG_XFORM_XREV); // Convex.
        // SW quadrant.
             if ((neighbors&0x12)==0x12) graf_tile(&g.graf,dstx,dsty,tileid,EGG_XFORM_YREV); // Full.
        else if ((neighbors&0x14)==0x14) graf_tile(&g.graf,dstx,dsty,tileid+3,EGG_XFORM_YREV); // Concave, downward.
        else if ((neighbors&0x06)==0x06) graf_tile(&g.graf,dstx,dsty,tileid+3,EGG_XFORM_SWAP|EGG_XFORM_XREV); // Concave, leftward.
        else if (neighbors&0x10) graf_tile(&g.graf,dstx,dsty,tileid+2,EGG_XFORM_YREV); // Horizontal.
        else if (neighbors&0x02) graf_tile(&g.graf,dstx,dsty,tileid+2,EGG_XFORM_SWAP|EGG_XFORM_XREV); // Vertical.
        else graf_tile(&g.graf,dstx,dsty,tileid+1,EGG_XFORM_YREV); // Convex.
        // SE quadrant.
             if ((neighbors&0x0a)==0x0a) graf_tile(&g.graf,dstx,dsty,tileid,EGG_XFORM_XREV|EGG_XFORM_YREV); // Full.
        else if ((neighbors&0x09)==0x09) graf_tile(&g.graf,dstx,dsty,tileid+3,EGG_XFORM_XREV|EGG_XFORM_YREV); // Concave, downward.
        else if ((neighbors&0x03)==0x03) graf_tile(&g.graf,dstx,dsty,tileid+3,EGG_XFORM_XREV|EGG_XFORM_YREV|EGG_XFORM_SWAP); // Concave, rightward.
        else if (neighbors&0x08) graf_tile(&g.graf,dstx,dsty,tileid+2,EGG_XFORM_XREV|EGG_XFORM_YREV); // Horizontal.
        else if (neighbors&0x02) graf_tile(&g.graf,dstx,dsty,tileid+2,EGG_XFORM_XREV|EGG_XFORM_YREV|EGG_XFORM_SWAP); // Vertical.
        else graf_tile(&g.graf,dstx,dsty,tileid+1,EGG_XFORM_XREV|EGG_XFORM_YREV); // Convex.
      } break;
      
    // Goal is one of 16 tiles. Shape must be rectangular, we don't have concave corners.
    // Owing to that limit, we only need to examine the cardinal neighbors.
    case 0x02: switch (neighbors&0x5a) {
        case 0x00: graf_tile(&g.graf,dstx,dsty,0x50,0); break;
        case 0x02: graf_tile(&g.graf,dstx,dsty,0x60,0); break;
        case 0x08: graf_tile(&g.graf,dstx,dsty,0x51,0); break;
        case 0x0a: graf_tile(&g.graf,dstx,dsty,0x61,0); break;
        case 0x10: graf_tile(&g.graf,dstx,dsty,0x53,0); break;
        case 0x12: graf_tile(&g.graf,dstx,dsty,0x63,0); break;
        case 0x18: graf_tile(&g.graf,dstx,dsty,0x52,0); break;
        case 0x1a: graf_tile(&g.graf,dstx,dsty,0x62,0); break;
        case 0x40: graf_tile(&g.graf,dstx,dsty,0x80,0); break;
        case 0x42: graf_tile(&g.graf,dstx,dsty,0x70,0); break;
        case 0x48: graf_tile(&g.graf,dstx,dsty,0x81,0); break;
        case 0x4a: graf_tile(&g.graf,dstx,dsty,0x71,0); break;
        case 0x50: graf_tile(&g.graf,dstx,dsty,0x83,0); break;
        case 0x52: graf_tile(&g.graf,dstx,dsty,0x73,0); break;
        case 0x58: graf_tile(&g.graf,dstx,dsty,0x82,0); break;
        case 0x5a: graf_tile(&g.graf,dstx,dsty,0x72,0); break;
      } break;
      
    // Anything else is undefined, but emit it as the raw tile.
    default: graf_tile(&g.graf,dstx,dsty,*src,0);
  }
}
 
static void bgbits_render() {
  graf_reset(&g.graf);
  graf_set_output(&g.graf,g.texid_bgbits);
  graf_set_input(&g.graf,g.texid_tiles);
  int dsty=NS_sys_tilesize>>1;
  int row=0;
  const uint8_t *src=g.map;
  for (;row<NS_sys_maph;row++,dsty+=NS_sys_tilesize) {
    int dstx=NS_sys_tilesize>>1;
    int col=0;
    for (;col<NS_sys_mapw;col++,dstx+=NS_sys_tilesize,src++) {
      draw_tile(dstx,dsty,src,col,row);
    }
  }
  graf_set_output(&g.graf,1);
  graf_flush(&g.graf);
}

/* Reset.
 */
 
int game_reset(int mapid) {

  inv_song(RID_song_inversion);
  
  //mapid=1;//XXX even temper

  int actual_mapid=mapid;
  #if 1 /* XXX TEMP, run maps backward during dev. */
    int resp=res_search(EGG_TID_map+1,0);
    if (resp<0) resp=-resp-1;
    if (resp>0) {
      resp--;
      int mapidz=g.resv[resp].rid;
      actual_mapid=mapidz-mapid+1;
      fprintf(stderr,"!!! REVERSED MAPS !!! Using map:%d in place of map:%d.\n",actual_mapid,mapid);
    }
  #endif

  const void *serial=0;
  int serialc=res_get(&serial,EGG_TID_map,actual_mapid);
  if (serialc<1) {
    fprintf(stderr,"map:%d not found\n",mapid);
    return -1;
  }
  struct map_res rmap;
  if (map_res_decode(&rmap,serial,serialc)<0) return -1;
  if ((rmap.w!=NS_sys_mapw)||(rmap.h!=NS_sys_maph)) {
    fprintf(stderr,"map:%d: expected %dx%d, found %dx%d\n",mapid,NS_sys_mapw,NS_sys_maph,rmap.w,rmap.h);
    return -1;
  }
  memcpy(g.map,rmap.v,NS_sys_mapw*NS_sys_maph);
  g.mapid=mapid;
  
  sprites_kill_all();
  g.gravity=0x02;
  g.fadeinclock=FADE_IN_TIME;
  g.goalclock=0.0;
  g.deadclock=0.0;
  
  struct cmdlist_reader reader={.v=rmap.cmd,.c=rmap.cmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
    
      case CMD_map_sprite: {
          double x=cmd.arg[0]+0.5;
          double y=cmd.arg[1]+0.5;
          int rid=(cmd.arg[2]<<8)|cmd.arg[3];
          const uint8_t *arg=cmd.arg+4;
          struct sprite *sprite=sprite_spawn(x,y,rid,arg,0,0,0);
        } break;
    }
  }
  
  bgbits_render();
  return 0;
}

/* Update.
 */
 
void game_update(double elapsed) {
  g.on_goal=0;
  sprites_update(elapsed);
  if (g.deadclock>0.0) {
    if ((g.deadclock-=elapsed)<=0.0) {
      game_reset(g.mapid);
    }
  } else if (g.fadeinclock>0.0) {
    g.fadeinclock-=elapsed;
  } else if (g.on_goal) {
    if (g.goalclock<=0.0) g.goalclock=FADE_OUT_TIME;
    if ((g.goalclock-=elapsed)<=0.0) {
      if (game_reset(g.mapid+1)<0) {
        fprintf(stderr,"GAME OVER, YOU WIN!\n");//TODO Game-over modal.
        game_reset(1);
      }
    }
  } else {
    g.goalclock=0.0;
  }
}

/* Render.
 */
 
void game_render() {
  graf_set_input(&g.graf,g.texid_bgbits);
  graf_decal(&g.graf,0,0,0,0,FBW,FBH);
  sprites_render();
  
  int alpha=0;
  if (g.deadclock>0.0) alpha=0xff-(int)((g.deadclock*255.0)/FADE_OUT_TIME);
  else if (g.goalclock>0.0) alpha=0xff-(int)((g.goalclock*255.0)/FADE_OUT_TIME);
  else if (g.fadeinclock>0.0) alpha=(int)((g.fadeinclock*255.0)/FADE_IN_TIME);
  if (alpha>0) {
    if (alpha>0xff) alpha=0xff;
    graf_fill_rect(&g.graf,0,0,FBW,FBH,0x00000000|alpha);
  }
}

/* Gravity.
 */

void gravity_reverse() {
  switch (g.gravity) {
    case 0x40: g.gravity=0x02; break;
    case 0x10: g.gravity=0x08; break;
    case 0x08: g.gravity=0x10; break;
    case 0x02: g.gravity=0x40; break;
    default: g.gravity=0x02;
  }
}

void gravity_rotate(int d) {
  switch (g.gravity) {
    case 0x40: g.gravity=(d<0)?0x10:0x08; break;
    case 0x10: g.gravity=(d<0)?0x02:0x40; break;
    case 0x08: g.gravity=(d<0)?0x40:0x02; break;
    case 0x02: g.gravity=(d<0)?0x08:0x10; break;
    default: g.gravity=0x02;
  }
}
