/* sprite_type_killozap.c
 * These must be created in pairs. Each pair must be on the same row or column, pointed at each other.
 * arg: [dir,period,duty,phase]
 *  - (dir) is one of 0x40,0x10,0x08,0x02
 *  - (period) in s/60 units.
 *  - (duty) normalized; how much of the period is spent zapping.
 *  - (phase) normalized. Zero phase starts OFF and ends ON.
 * (period,duty,phase) must be the same for each member of a pair.
 */

#include "game/inversion.h"

#define ROLE_ORPHAN  0 /* I don't have a partner. */
#define ROLE_PASSIVE 1 /* My partner does the work, and he does exist. */
#define ROLE_ACTIVE  2 /* I'm doing the work. */
#define ROLE_ALWAYS  3 /* ACTIVE, but with a 100% duty cycle, ie always on. */

struct sprite_killozap {
  struct sprite hdr;
  uint8_t dir;
  int role; // ROLE_ORPHAN until established.
// Only for ROLE_ACTIVE:
  int beamc; // How many cells of beam between me and my partner? Technically can be zero.
  int zapping;
  double clock; // Counts down to change of (zapping).
  double animclock;
  int animframe;
  double idle_time,zap_time; // Constant.
  struct aabb killbox;
};

#define SPRITE ((struct sprite_killozap*)sprite)

/* Init.
 */
 
static int _killozap_init(struct sprite *sprite) {

  // xform from dir. Natural orientation is down.
  int dx=0,dy=0;
  SPRITE->dir=sprite->arg[0];
  switch (SPRITE->dir) {
    case 0x40: dy=-1; sprite->xform=EGG_XFORM_YREV; break;
    case 0x10: dx=-1; sprite->xform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
    case 0x08: dx= 1; sprite->xform=EGG_XFORM_SWAP; break;
    case 0x02: dy= 1; sprite->xform=0; break;
    default: fprintf(stderr,"killozap invalid dir 0x%02x\n",SPRITE->dir); return -1;
  }
  
  /* Sprites get initialized as they spawn, so we don't necessarily see the entire set of sprites.
   * Nevertheless, among one pair of killozaps, one of them will be created first and one second.
   * Because that's how numbers work.
   * So the second of the pair to get initialized becomes active, and first passive.
   * Mind that they are not spawned in any particular order, so the passive end could be in any direction.
   * If there's two pairs in the same row or column, make sure they're grouped by pair in the command list.
   */
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *other=*p;
    if (other->defunct) continue;
    if (other->type!=&sprite_type_killozap) continue;
    struct sprite_killozap *OTHER=(struct sprite_killozap*)other;
    if (OTHER->role) continue;
    if (memcmp(sprite->arg+1,other->arg+1,3)) continue; // (period,duty,phase) must match.
    switch (SPRITE->dir) {
      case 0x40: if (OTHER->dir!=0x02) continue; if (other->x!=sprite->x) continue; if (other->y>=sprite->y) continue; SPRITE->beamc=(int)sprite->y-(int)other->y-1; break;
      case 0x10: if (OTHER->dir!=0x08) continue; if (other->x>=sprite->x) continue; if (other->y!=sprite->y) continue; SPRITE->beamc=(int)sprite->x-(int)other->x-1; break;
      case 0x08: if (OTHER->dir!=0x10) continue; if (other->x<=sprite->x) continue; if (other->y!=sprite->y) continue; SPRITE->beamc=(int)other->x-(int)sprite->x-1; break;
      case 0x02: if (OTHER->dir!=0x40) continue; if (other->x!=sprite->x) continue; if (other->y<=sprite->y) continue; SPRITE->beamc=(int)other->y-(int)sprite->y-1; break;
    }
    SPRITE->role=ROLE_ACTIVE;
    OTHER->role=ROLE_PASSIVE;
    break;
  }
  
  /* If we assumed ROLE_ACTIVE, prep my clock.
   */
  if (SPRITE->role==ROLE_ACTIVE) {
    if (sprite->arg[2]<1) { // Zero duty cycle. We're effectively passive.
      SPRITE->role=ROLE_PASSIVE;
    } else if (sprite->arg[2]==0xff) { // Full duty cycle. No need for timing.
      SPRITE->role=ROLE_ALWAYS;
      SPRITE->zapping=1;
    } else { // Oscillating.
      int periods60=sprite->arg[1];
      if (periods60<1) periods60=1;
      double period=(double)periods60/60.0;
      double duty=sprite->arg[2]/255.0;
      SPRITE->zap_time=duty*period;
      SPRITE->idle_time=period-SPRITE->zap_time;
      double p=(sprite->arg[3]*period)/256.0;
      if (p<SPRITE->idle_time) {
        SPRITE->clock=SPRITE->idle_time-p;
      } else {
        SPRITE->zapping=1;
        SPRITE->clock=period-p;
      }
    }
    // Prep my killbox too.
    SPRITE->killbox.l=sprite->x-0.5;
    SPRITE->killbox.r=sprite->x+0.5;
    SPRITE->killbox.t=sprite->y-0.5;
    SPRITE->killbox.b=sprite->y+0.5;
    switch (SPRITE->dir) {
      case 0x40: SPRITE->killbox.t-=SPRITE->beamc; SPRITE->killbox.b-=1.0; break;
      case 0x10: SPRITE->killbox.l-=SPRITE->beamc; SPRITE->killbox.r-=1.0; break;
      case 0x08: SPRITE->killbox.r+=SPRITE->beamc; SPRITE->killbox.l+=1.0; break;
      case 0x02: SPRITE->killbox.b+=SPRITE->beamc; SPRITE->killbox.t+=1.0; break;
    }
  }
  
  return 0;
}

/* Can we kill something?
 * Keep it simple, just compare the victims' midpoint to our coverage box.
 */
 
static void killozap_check_victims(struct sprite *sprite) {
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *victim=*p;
    if (!victim->type->hurt) continue;
    if (victim->defunct) continue;
    if (victim->x<=SPRITE->killbox.l) continue;
    if (victim->x>=SPRITE->killbox.r) continue;
    if (victim->y<=SPRITE->killbox.t) continue;
    if (victim->y>=SPRITE->killbox.b) continue;
    victim->type->hurt(victim,sprite);
  }
}

/* Update.
 */
 
static void _killozap_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->role==ROLE_ACTIVE) {
    if ((SPRITE->clock-=elapsed)<=0.0) {
      if (SPRITE->zapping=!SPRITE->zapping) {
        SPRITE->clock=SPRITE->zap_time;
        SPRITE->animclock=0.0;
        SPRITE->animframe=2;
      } else {
        SPRITE->clock=SPRITE->idle_time;
      }
    }
  }
  if (SPRITE->zapping) {
    killozap_check_victims(sprite);
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=0.050;
      if (++(SPRITE->animframe)>=6) SPRITE->animframe=0;
    }
  }
}

/* Render.
 */
 
static void _killozap_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,sprite->tileid,sprite->xform);
  if (SPRITE->zapping) {
    uint8_t tileid=sprite->tileid+0x10;
    switch (SPRITE->animframe) {
      case 0: break;
      case 1: tileid+=0x10; break;
      case 2: tileid+=0x20; break;
      case 3: tileid+=0x30; break;
      case 4: tileid+=0x20; break;
      case 5: tileid+=0x10; break;
    }
    int dx=0,dy=0;
    switch (SPRITE->dir) {
      case 0x40: dy=-1; break;
      case 0x10: dx=-1; break;
      case 0x08: dx= 1; break;
      case 0x02: dy= 1; break;
      default: return;
    }
    dx*=NS_sys_tilesize;
    dy*=NS_sys_tilesize;
    int beamx=x+dx;
    int beamy=y+dy;
    int i=SPRITE->beamc;
    for (;i-->0;beamx+=dx,beamy+=dy) {
      graf_tile(&g.graf,beamx,beamy,tileid,sprite->xform);
    }
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_killozap={
  .name="killozap",
  .objlen=sizeof(struct sprite_killozap),
  .init=_killozap_init,
  .update=_killozap_update,
  .render=_killozap_render,
};
