#include "inversion.h"

#define FRAME_TIME 0.080

struct sprite_dust {
  struct sprite hdr;
  double animclock;
  uint8_t tileidz;
  // Our positions and transforms get locked in immediately; further changes to gravity don't affect us.
  int ax,ay,bx,by;
  uint8_t axform,bxform;
};

#define SPRITE ((struct sprite_dust*)sprite)

static int _dust_init(struct sprite *sprite) {
  SPRITE->tileidz=sprite->tileid+3;
  SPRITE->animclock=FRAME_TIME;
  
  double ax=-1.0,ay=0.0,bx=1.0,by=0.0;
  deltaf_plus_gravity(&ax,&ay);
  deltaf_plus_gravity(&bx,&by);
  SPRITE->ax=(int)((sprite->x+ax)*NS_sys_tilesize);
  SPRITE->ay=(int)((sprite->y+ay)*NS_sys_tilesize);
  SPRITE->bx=(int)((sprite->x+bx)*NS_sys_tilesize);
  SPRITE->by=(int)((sprite->y+by)*NS_sys_tilesize);
  SPRITE->axform=xform_plus_gravity(0);
  SPRITE->bxform=xform_plus_gravity(EGG_XFORM_XREV);
  
  return 0;
}

static void _dust_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=FRAME_TIME;
    if (sprite->tileid>=SPRITE->tileidz) {
      sprite->defunct=1;
    } else {
      sprite->tileid++;
    }
  }
}

static void _dust_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,SPRITE->ax,SPRITE->ay,sprite->tileid,SPRITE->axform);
  graf_tile(&g.graf,SPRITE->bx,SPRITE->by,sprite->tileid,SPRITE->bxform);
}

const struct sprite_type sprite_type_dust={
  .name="dust",
  .objlen=sizeof(struct sprite_dust),
  .init=_dust_init,
  .update=_dust_update,
  .render=_dust_render,
};
