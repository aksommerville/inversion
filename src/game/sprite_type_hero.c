#include "inversion.h"

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  return 0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  
  // Animate.
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.125;
    if (++(SPRITE->animframe)>=10) SPRITE->animframe=0;
  }
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {
  uint8_t tileid=sprite->tileid;
  if (g.input&EGG_BTN_UP) tileid+=3;
  else if (g.input&EGG_BTN_DOWN) tileid+=9;
  else switch (SPRITE->animframe) {
    case 7: tileid+=6; break;
    case 8: tileid+=7; break;
    case 9: tileid+=8; break;
    default: tileid+=5; break;
  }
  uint8_t xform=xform_plus_gravity(sprite->xform);
  graf_tile(&g.graf,x,y,tileid,xform);
  int headdx=0,headdy=-NS_sys_tilesize;
  deltai_plus_gravity(&headdx,&headdy);
  graf_tile(&g.graf,x+headdx,y+headdy,tileid-0x10,xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
