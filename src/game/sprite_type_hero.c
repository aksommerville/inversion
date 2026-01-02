#include "inversion.h"

#define WALK_SPEED      6.0 /* m/s */
#define JUMP_POWER_MAX 20.0 /* m/s */
#define JUMP_DECAY     70.0 /* m/s**2 */
#define GRAVITY_LIMIT  10.0 /* m/s */
#define GRAVITY_ACCEL  20.0 /* m/s**2 */

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int ducking;
  int walking; // -1,0,1
  int jumping;
  int seated;
  int jump_blackout;
  double jumpdx,jumpdy;
  double gravity;
  double jumpclock; // Counts up during the jump.
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  SPRITE->seated=1;
  return 0;
}

/* Buttons that can change per gravity.
 */
 
static void hero_walk_buttons(int *l,int *r) {
  switch (g.gravity) {
    case 0x10: *l=EGG_BTN_UP; *r=EGG_BTN_DOWN; break;
    case 0x08: *l=EGG_BTN_DOWN; *r=EGG_BTN_UP; break;
  }
  *l=EGG_BTN_LEFT;
  *r=EGG_BTN_RIGHT;
}

static int hero_duck_button() {
  switch (g.gravity) {
    case 0x40: return EGG_BTN_UP;
    case 0x10: return EGG_BTN_LEFT;
    case 0x08: return EGG_BTN_RIGHT;
    case 0x02: return EGG_BTN_DOWN;
  }
  return 0;
}

/* Walking.
 */
 
static void hero_walk(struct sprite *sprite,int d,double elapsed) {

  if (!SPRITE->walking) {
    SPRITE->walking=1;
    SPRITE->animclock=0.0;
    SPRITE->animframe=0;
  }
  if (d<0) sprite->xform=EGG_XFORM_XREV;
  else sprite->xform=0;
  
  double dx=(d<0)?-1.0:1.0,dy=0.0;
  dx*=elapsed*WALK_SPEED;
  deltaf_plus_gravity(&dx,&dy);
  sprite_move(sprite,dx,dy);
  
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.200;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
}

static void hero_walk_end(struct sprite *sprite) {
  SPRITE->walking=0;
  SPRITE->animclock=0.0;
  SPRITE->animframe=0;
}

/* Flip. Call this as we reverse gravity, with (g.gravity) still in the original orientation.
 * The hero sprite is vertically asymmetric.
 * So when we bonk head on the ceiling, if we leave (sprite->y) where it is, we'd be a head away.
 * (Mind that it's not always (y)).
 */
 
static void hero_flip(struct sprite *sprite) {
  double d=sprite->hbb+sprite->hbt;
  switch (g.gravity) {
    case 0x40: sprite->y-=d; break;
    case 0x10: sprite->x-=d; break;
    case 0x08: sprite->x+=d; break;
    case 0x02: sprite->y+=d; break;
  }
}

/* Gravity and jumping.
 */
 
static void hero_jump_crest(struct sprite *sprite) {
  SPRITE->jumping=0;
  SPRITE->seated=0;
  SPRITE->gravity=0.0;
}

static void hero_jump_update(struct sprite *sprite,double elapsed) {
  if (!(g.input&EGG_BTN_SOUTH)) {
    hero_jump_crest(sprite);
    return;
  }
  SPRITE->jumpclock+=elapsed;
  double dx=SPRITE->jumpdx*elapsed;
  double dy=SPRITE->jumpdy*elapsed;
  if (!sprite_move(sprite,dx,dy)) {
    // If we are within say 2 frames of the jump's start, don't flip. Presumably there is something resting on my head.
    // Hitting the ceiling in a 2-meter corridor takes about 0.050, and that should bonk.
    if (SPRITE->jumpclock<0.035) {
      inv_sound(RID_sound_bonk_reject);
    } else {
      inv_sound(RID_sound_bonk);
      hero_flip(sprite);
      gravity_reverse();
    }
    hero_jump_crest(sprite);
    return;
  }
  if (SPRITE->jumpdx<0.0) {
    if ((SPRITE->jumpdx+=elapsed*JUMP_DECAY)>=0.0) hero_jump_crest(sprite);
  } else if (SPRITE->jumpdx>0.0) {
    if ((SPRITE->jumpdx-=elapsed*JUMP_DECAY)<=0.0) hero_jump_crest(sprite);
  } else if (SPRITE->jumpdy<0.0) {
    if ((SPRITE->jumpdy+=elapsed*JUMP_DECAY)>=0.0) hero_jump_crest(sprite);
  } else if (SPRITE->jumpdy>0.0) {
    if ((SPRITE->jumpdy-=elapsed*JUMP_DECAY)<=0.0) hero_jump_crest(sprite);
  }
}
 
static void hero_jump_begin(struct sprite *sprite,double elapsed) {
  inv_sound(RID_sound_jump);
  SPRITE->jumping=1;
  SPRITE->jump_blackout=1;
  SPRITE->seated=0;
  SPRITE->gravity=0.0;
  SPRITE->jumpdx=0.0;
  SPRITE->jumpdy=-JUMP_POWER_MAX;
  deltaf_plus_gravity(&SPRITE->jumpdx,&SPRITE->jumpdy);
  SPRITE->jumpclock=0.0;
  hero_jump_update(sprite,elapsed);
}

static void hero_gravity_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->gravity+=GRAVITY_ACCEL*elapsed)>GRAVITY_LIMIT) SPRITE->gravity=GRAVITY_LIMIT;
  double dx=0.0,dy=SPRITE->gravity*elapsed;
  deltaf_plus_gravity(&dx,&dy);
  if (!sprite_move(sprite,dx,dy)) {
    SPRITE->gravity=0.0;
    if (!SPRITE->seated) {
      SPRITE->seated=1;
    }
  } else {
    if (SPRITE->seated) {
      SPRITE->seated=0;
    }
  }
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  // Walking.
  if (!SPRITE->ducking) {
    int lbtn,rbtn;
    hero_walk_buttons(&lbtn,&rbtn);
    int btn=g.input&(lbtn|rbtn);
    if (btn==lbtn) hero_walk(sprite,-1,elapsed);
    else if (btn==rbtn) hero_walk(sprite,1,elapsed);
    else if (SPRITE->walking) hero_walk_end(sprite);
  }
  
  // Ducking.
  if (SPRITE->ducking) {
    if (!(g.input&hero_duck_button())) {
      SPRITE->ducking=0;
    }
  } else if (SPRITE->seated) {
    if (g.input&hero_duck_button()) {
      if (SPRITE->walking) hero_walk_end(sprite);
      SPRITE->ducking=1;
    }
  }
  
  // Jump or gravity.
  if (SPRITE->seated&&(g.input&EGG_BTN_SOUTH)&&!SPRITE->jump_blackout) {
    hero_jump_begin(sprite,elapsed);
  } else if (SPRITE->jumping) {
    hero_jump_update(sprite,elapsed);
  } else {
    if (!(g.input&EGG_BTN_SOUTH)) SPRITE->jump_blackout=0;
    hero_gravity_update(sprite,elapsed);
  }
  
  // The world wraps around. If we've gone offscreen, wrap it.
  if (sprite->x<0.0) sprite->x+=NS_sys_mapw;
  else if (sprite->x>NS_sys_mapw) sprite->x-=NS_sys_mapw;
  if (sprite->y<0.0) sprite->y+=NS_sys_maph;
  else if (sprite->y>NS_sys_maph) sprite->y-=NS_sys_maph;
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {
  uint8_t tileid=sprite->tileid;
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
