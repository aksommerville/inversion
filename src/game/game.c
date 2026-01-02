#include "inversion.h"

/* Rewrite bgbits.
 * (g.map) is ready.
 */
 
static void draw_tile(int dstx,int dsty,const uint8_t *src,int col,int row) {

  // Everything past the first row is just a plain tile.
  if (*src>=0x10) {
    graf_tile(&g.graf,dstx,dsty,*src,0);
    return;
  }
  
  // Collect the neighbor mask. OOB clamps, it doesn't wrap.
  // Logically, it should wrap, but we'll design with matching edges to avoid the issue.
  uint8_t neighbors=0;
  if ((row<=0)||(col<=0)||(src[0]==src[-NS_sys_mapw-1])) neighbors|=0x80;
  if ((row<=0)||(src[0]==src[-NS_sys_mapw])) neighbors|=0x40;
  if ((row<=0)||(col>=NS_sys_mapw-1)||(src[0]==src[-NS_sys_mapw+1])) neighbors|=0x20;
  if ((col<=0)||(src[0]==src[-1])) neighbors|=0x10;
  if ((col>=NS_sys_mapw-1)||(src[0]==src[1])) neighbors|=0x08;
  if ((row>=NS_sys_maph-1)||(col<=0)||(src[0]==src[NS_sys_mapw-1])) neighbors|=0x04;
  if ((row>=NS_sys_maph-1)||(src[0]==src[NS_sys_mapw])) neighbors|=0x02;
  if ((row>=NS_sys_maph-1)||(col>=NS_sys_mapw-1)||(src[0]==src[NS_sys_mapw+1])) neighbors|=0x01;
  
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
 
void game_reset(int mapid) {

  const void *serial=0;
  int serialc=res_get(&serial,EGG_TID_map,mapid);
  if (serialc<1) {
    fprintf(stderr,"map:%d not found\n",mapid);
    return;
  }
  struct map_res rmap;
  if (map_res_decode(&rmap,serial,serialc)<0) return;
  if ((rmap.w!=NS_sys_mapw)||(rmap.h!=NS_sys_maph)) {
    fprintf(stderr,"map:%d: expected %dx%d, found %dx%d\n",mapid,NS_sys_mapw,NS_sys_maph,rmap.w,rmap.h);
    return;
  }
  memcpy(g.map,rmap.v,NS_sys_mapw*NS_sys_maph);
  
  struct cmdlist_reader reader={.v=rmap.cmd,.c=rmap.cmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      //TODO?
    }
  }
  
  bgbits_render();
  
  //TODO Refresh sprites.
  fprintf(stderr,"loaded map:%d ok\n",mapid);
}

/* Update.
 */
 
void game_update(double elapsed) {
  //TODO
}

/* Render.
 */
 
void game_render() {
  graf_set_input(&g.graf,g.texid_bgbits);
  graf_decal(&g.graf,0,0,0,0,FBW,FBH);
  graf_set_input(&g.graf,g.texid_tiles);
  //TODO sprites
  graf_tile(&g.graf,40,84,0x20,0);
  graf_tile(&g.graf,40,76,0x10,0);
}
