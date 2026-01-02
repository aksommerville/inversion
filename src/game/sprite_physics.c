#include "inversion.h"

#define SMIDGEON 0.001

/* Gravity-adjusted hitbox.
 * Note that upside-down is just a flip of the Y axis, but the two horizontal orientations are proper rotation.
 * I expect sprites to be horizontally symmetric so it won't matter. But if they aren't, I'm not sure what the correct approach is.
 */
 
void sprite_get_hitbox(struct aabb *dst,const struct sprite *sprite) {
  switch (g.gravity) {
    case 0x40: {
        dst->l=sprite->x+sprite->hbl;
        dst->r=sprite->x+sprite->hbr;
        dst->t=sprite->y-sprite->hbb;
        dst->b=sprite->y-sprite->hbt;
      } break;
    case 0x10: {
        dst->l=sprite->x-sprite->hbb;
        dst->r=sprite->x-sprite->hbt;
        dst->t=sprite->y+sprite->hbl;
        dst->b=sprite->y+sprite->hbr;
      } break;
    case 0x08: {
        dst->l=sprite->x+sprite->hbb;
        dst->r=sprite->x+sprite->hbt;
        dst->t=sprite->y-sprite->hbl;
        dst->b=sprite->y-sprite->hbr;
      } break;
    case 0x02: {
        dst->l=sprite->x+sprite->hbl;
        dst->r=sprite->x+sprite->hbr;
        dst->t=sprite->y+sprite->hbt;
        dst->b=sprite->y+sprite->hbb;
      } break;
  }
}

static double hbl(const struct sprite *sprite) {
  switch (g.gravity) {
    case 0x40: return sprite->hbl;
    case 0x10: return -sprite->hbb;
    case 0x08: return sprite->hbt;
    case 0x02: return sprite->hbl;
  }
  return sprite->hbl;
}
static double hbr(const struct sprite *sprite) {
  switch (g.gravity) {
    case 0x40: return sprite->hbr;
    case 0x10: return -sprite->hbt;
    case 0x08: return sprite->hbb;
    case 0x02: return sprite->hbr;
  }
  return sprite->hbr;
}
static double hbt(const struct sprite *sprite) {
  switch (g.gravity) {
    case 0x40: return -sprite->hbb;
    case 0x10: return sprite->hbl;
    case 0x08: return -sprite->hbr;
    case 0x02: return sprite->hbt;
  }
  return sprite->hbt;
}
static double hbb(const struct sprite *sprite) {
  switch (g.gravity) {
    case 0x40: return -sprite->hbt;
    case 0x10: return sprite->hbr;
    case 0x08: return -sprite->hbl;
    case 0x02: return sprite->hbb;
  }
  return sprite->hbb;
}

/* Detect collisions.
 * Caller provides the sprite for self detection, and maybe other parameters.
 * The bounds being tested are strictly (aabb), not pulled from (sprite).
 * We'll never return more than (colla); we might miss collisions if the list fills.
 */
 
static int sprite_detect_collisions(struct collision *collv,int colla,const struct aabb *aabb,const struct sprite *sprite) {
  int collc=0;

  /* Check the grid.
   */
  {
    int cola=(int)aabb->l; if (aabb->l<0.0) cola--;
    int colz=(int)(aabb->r-SMIDGEON); if (aabb->r<0.0) colz--;
    int rowa=(int)aabb->t; if (aabb->t<0.0) rowa--;
    int rowz=(int)(aabb->b-SMIDGEON); if (aabb->b<0.0) rowz--;
    // OOB handling is unlike most games: We automatically wrap around the screen. So no clamping here.
    int prerow=rowa;
    for (;prerow<=rowz;prerow++) {
      int row=prerow;
      if (row>=NS_sys_maph) { if ((row-=NS_sys_maph)>=NS_sys_maph) continue; }
      if (row<0) { if ((row+=NS_sys_maph)<0) continue; }
      int precol=cola;
      for (;precol<=colz; precol++) {
        int col=precol;
        if (col>=NS_sys_mapw) { if ((col-=NS_sys_mapw)>=NS_sys_mapw) continue; }
        if (col<0) { if ((col+=NS_sys_mapw)<0) continue; }
        // The map contains physics values directly, no tilesheet lookup.
        uint8_t physics=g.map[row*NS_sys_mapw+col];
        if (physics==sprite->pass_physics) continue;
        if (collc>=colla) return collc;
        struct collision *coll=collv+collc++;
        coll->sprite=0;
        coll->l=col;
        coll->t=row;
        // If we're OOB, move the collision by a screenful to match (sprite).
             if (prerow<row) coll->t-=NS_sys_maph;
        else if (prerow>row) coll->t+=NS_sys_maph;
             if (precol<col) coll->l-=NS_sys_mapw;
        else if (precol>col) coll->l+=NS_sys_mapw;
        coll->r=coll->l+1.0;
        coll->b=coll->t+1.0;
      }
    }
  }
  
  /* Check other sprites.
   */
  {
    struct sprite **p=g.spritev;
    int i=g.spritec;
    for (;i-->0;p++) {
      struct sprite *other=*p;
      if (other==sprite) continue;
      if (!other->solid) continue;
      //TODO For now, we are only checking the untransformed boundaries of each sprite.
      // Is there much risk of sprite-on-sprite collisions happening near the screen edges? Can we avoid that in design?
      // It would be pretty expensive to do it right.
      struct aabb ob;
      sprite_get_hitbox(&ob,other);
      if (aabb->r<=ob.l) continue;
      if (aabb->l>=ob.r) continue;
      if (aabb->b<=ob.t) continue;
      if (aabb->t>=ob.b) continue;
      if (collc>=colla) return collc;
      struct collision *coll=collv+collc++;
      coll->sprite=other;
      coll->l=ob.l;
      coll->r=ob.r;
      coll->t=ob.t;
      coll->b=ob.b;
    }
  }

  return collc;
}

/* Move sprite with physics.
 */
 
int sprite_move(struct sprite *sprite,double dx,double dy) {

  if (!sprite->solid) {
    sprite->x+=dx;
    sprite->y+=dy;
    return 1;
  }
  
  if (
    ((dx<-SMIDGEON)||(dx>SMIDGEON))&&
    ((dy<-SMIDGEON)||(dy>SMIDGEON))
  ) {
    int xok=sprite_move(sprite,dx,0.0);
    int yok=sprite_move(sprite,0.0,dy);
    return xok||yok;
  }
  
  /* Get my adjusted hitbox, and then another (nr) including the newly-covered space.
   * Pull (nr)'s butt forward just a little to avoid certain rounding problems.
   */
  struct aabb or;
  sprite_get_hitbox(&or,sprite);
  struct aabb nr=or;
  const double BUTT_PULL=0.010;
       if (dx<0.0) { nr.l+=dx; nr.r-=BUTT_PULL; }
  else if (dx>0.0) { nr.r+=dx; nr.l+=BUTT_PULL; }
  else if (dy<0.0) { nr.t+=dy; nr.b-=BUTT_PULL; }
  else if (dy>0.0) { nr.b+=dy; nr.t+=BUTT_PULL; }
  else return 0;
  
  /* Identify all collisions against (nr).
   * If there's none, declare victory and get out.
   */
  struct collision collv[32];
  int collc=sprite_detect_collisions(collv,32,&nr,sprite);
  if (collc<1) {
    sprite->x+=dx;
    sprite->y+=dy;
    return 1;
  }
  
  /* Find the nearest collision with respect to our direction of travel.
   * Then we'll clamp to its near edge.
   * Check (or)'s leading edge against it first; if <=0, we must do nothing and return zero.
   */
  struct collision *nearest=collv,*q=collv;
  int i=collc;
  double d;
  if (dx<0.0) {
    for (;i-->0;q++) {
      if (q->r>nearest->r) nearest=q;
    }
    if ((d=or.l-nearest->r)<=0.0) return 0;
    sprite->x=nearest->r-hbl(sprite);
    
  } else if (dx>0.0) {
    for (;i-->0;q++) {
      if (q->l<nearest->l) nearest=q;
    }
    if ((d=nearest->l-or.r)<=0.0) return 0;
    sprite->x=nearest->l-hbr(sprite);
    
  } else if (dy<0.0) {
    for (;i-->0;q++) {
      if (q->b>nearest->b) nearest=q;
    }
    if ((d=or.t-nearest->b)<=0.0) return 0;
    sprite->y=nearest->b-hbt(sprite);
    
  } else {
    for (;i-->0;q++) {
      if (q->t<nearest->t) nearest=q;
    }
    if ((d=nearest->t-or.b)<=0.0) return 0;
    sprite->y=nearest->t-hbb(sprite);
  }
  return 1;
}
