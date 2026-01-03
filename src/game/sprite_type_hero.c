#include "inversion.h"

#define WALK_SPEED      6.0 /* m/s */
#define JUMP_POWER_MAX 20.0 /* m/s */
#define JUMP_DECAY     70.0 /* m/s**2 */
#define GRAVITY_LIMIT  16.0 /* m/s */
#define GRAVITY_ACCEL  30.0 /* m/s**2 */
#define DUCK_CONFIRM_TIME  0.500
#define WALK_COOLDOWN_TIME 0.150
#define INVERT_ANIMTIME 0.150

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int ducking;
  int walking; // -1,0,1
  int jumping;
  int pushing; // -1,0,1
  int seated;
  int jump_blackout;
  double jumpdx,jumpdy;
  double gravity;
  double jumpclock; // Counts up during the jump.
  int ducksignx,ducksigny;
  uint8_t ducksigntileid; // + animframe
  uint8_t ducksignxform;
  double duckclock;
  int duckcx,duckcy,duckox,duckoy;
  double walk_cooldown; // Counts down immediately after a rotation, since the duck button is now a walk button.
  int invert_cooldown; // Timing driven by (animclock,animframe)
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
    case 0x10: *l=EGG_BTN_UP; *r=EGG_BTN_DOWN; return;
    case 0x08: *l=EGG_BTN_DOWN; *r=EGG_BTN_UP; return;
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

/* Commit inversion.
 */
 
static void hero_commit_inversion(struct sprite *sprite) {
  double dx=SPRITE->pushing,dy=0.0;
  deltaf_plus_gravity(&dx,&dy);
  sprite->x+=dx;
  sprite->y+=dy;
  sprite->xform^=EGG_XFORM_XREV;
  sprite->pass_physics^=1;
  SPRITE->invert_cooldown=1;
}

/* Check the wall for pushing.
 */
 
static int hero_valid_wall(struct sprite *sprite,int d) {
  if (!SPRITE->seated) return 0; // No inversion without terra firma.
  int dx=d,dy=0;
  deltai_plus_gravity(&dx,&dy);
  int ax=(int)sprite->x+dx;
  int ay=(int)sprite->y+dy;
  if ((ax<0)||(ay<0)||(ax>=NS_sys_mapw)||(ay>=NS_sys_maph)) return 0; // Don't invert across the wraparound. We'll design maps such that it's not possible anyway.
  dx=0;
  dy=-1;
  deltai_plus_gravity(&dx,&dy);
  int bx=ax+dx;
  int by=ay+dy;
  if ((bx<0)||(by<0)||(bx>=NS_sys_mapw)||(by>=NS_sys_maph)) return 0;
  // Both physics must be solid or vacant, whichever is not my passable.
  // No inverting into the goal!
  uint8_t aph=g.map[ay*NS_sys_mapw+ax];
  uint8_t bph=g.map[by*NS_sys_mapw+bx];
  if (sprite->pass_physics==NS_physics_vacant) {
    if (aph!=NS_physics_solid) return 0;
    if (bph!=NS_physics_solid) return 0;
  } else {
    if (aph!=NS_physics_vacant) return 0;
    if (bph!=NS_physics_vacant) return 0;
  }
  // Finally, confirm that no collision exists there against other solid sprites.
  // We should use the hero's hitbox for this, but it's simpler to construct an exact 1x2 hitbox.
  struct aabb hitbox;
  if (ax<bx) { hitbox.l=ax; hitbox.r=bx+1.0; }
  else { hitbox.l=bx; hitbox.r=ax+1.0; }
  if (ay<by) { hitbox.t=ay; hitbox.b=by+1.0; }
  else { hitbox.t=by; hitbox.b=ay+1.0; }
  struct collision coll;
  sprite->pass_physics^=1;
  int collided=sprite_detect_collisions(&coll,1,&hitbox,sprite);
  sprite->pass_physics^=1;
  if (collided) return 0;
  return 1;
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
  if (!sprite_move(sprite,dx,dy)) {
    if (hero_valid_wall(sprite,d)) {
      if (SPRITE->pushing!=d) {
        SPRITE->pushing=d;
        SPRITE->animclock=0.0;
        SPRITE->animframe=0;
      }
      SPRITE->animclock+=elapsed;
      if (SPRITE->animclock>=INVERT_ANIMTIME) {
        SPRITE->animclock-=INVERT_ANIMTIME;
        if (++(SPRITE->animframe)>=4) {
          hero_commit_inversion(sprite);
        }
      }
      return;
    }
  }
  
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.150;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
}

static void hero_walk_end(struct sprite *sprite) {
  SPRITE->walking=0;
  SPRITE->animclock=0.0;
  SPRITE->animframe=0;
  SPRITE->pushing=0;
}

/* Flip. Call this as we reverse gravity, with (g.gravity) still in the original orientation.
 * The hero sprite is vertically asymmetric.
 * So when we bonk head on the ceiling, if we leave (sprite->y) where it is, we'd be a head away.
 * (Mind that it's not always (y)).
 * Another thing here: Swapping horizontal gravities also correctly swaps the horizontal xform, but we actually want Dot to face the same way after.
 */
 
static void hero_flip(struct sprite *sprite) {
  double d=sprite->hbb+sprite->hbt;
  switch (g.gravity) {
    case 0x40: sprite->y-=d; break;
    case 0x10: sprite->x-=d; sprite->xform^=EGG_XFORM_XREV; break;
    case 0x08: sprite->x+=d; sprite->xform^=EGG_XFORM_XREV; break;
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
  SPRITE->pushing=0;
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

/* (candx,candy) is a solid cell and (otherx,othery) is passable adjacent to it.
 * These are presumably directly under the hero's feet.
 * Returns nonzero if the rotation would be clockwise, zero if deasil.
 * Also optionall (nx,ny), the new sprite position.
 */
 
static int hero_hitbox_for_rotation(
  struct aabb *hitbox,
  const struct sprite *sprite,
  int candx,int candy,int otherx,int othery,
  double *nx,double *ny
) {
  // Poison (hitbox) such that it definitely will collide with something, if we don't replace it.
  hitbox->l=hitbox->t=0.0;
  hitbox->r=NS_sys_mapw;
  hitbox->b=NS_sys_maph;
  switch (g.gravity) {
    case 0x40: case 0x02: if (otherx<candx) { // top or bottom to left
        hitbox->l=candx-sprite->hbb+sprite->hbt;
        hitbox->r=candx;
        hitbox->t=candy+0.5-sprite->hbr;
        hitbox->b=candy+0.5-sprite->hbl;
        if (nx) *nx=candx-sprite->hbb;
        if (ny) *ny=candy+0.5;
      } else if (otherx>candx) { // top or bottom to right
        hitbox->l=candx+1.0;
        hitbox->r=candx+1.0-sprite->hbb-sprite->hbt;
        hitbox->t=candy+0.5+sprite->hbl;
        hitbox->b=candy+0.5+sprite->hbr;
        if (nx) *nx=candx+1.0+sprite->hbb;
        if (ny) *ny=candy+0.5;
      } break;
    case 0x10: case 0x08: if (othery<candy) { // left or right to top
        hitbox->l=candx+0.5+sprite->hbl;
        hitbox->r=candx+0.5+sprite->hbr;
        hitbox->t=candy-sprite->hbb+sprite->hbt;
        hitbox->b=candy;
        if (nx) *nx=candx+0.5;
        if (ny) *ny=candy-sprite->hbb;
      } else if (othery>candy) { // left or right to bottom
        hitbox->l=candx+0.5+sprite->hbl;
        hitbox->r=candx+0.5+sprite->hbr;
        hitbox->t=candy+1.0;
        hitbox->b=candy+1.0+sprite->hbb-sprite->hbt;
        if (nx) *nx=candx+0.5;
        if (ny) *ny=candy+1.0+sprite->hbb;
      } break;
  }
  switch (g.gravity) {
    case 0x40: return (otherx<candx)?1:0;
    case 0x10: return (othery>candy)?1:0;
    case 0x08: return (othery<candy)?1:0;
    case 0x02: return (otherx>candx)?1:0;
  }
  return 0;
}

/* Ducking.
 */
 
static void hero_duck_commit(struct sprite *sprite) {
  
  /* Confirm once more that the target position is kosher.
   * If not, reset the clock and get out.
   * I don't expect this ever to happen, but maybe there are moving platforms or something.
   */
  struct aabb hitbox;
  double nx=sprite->x,ny=sprite->y;
  int clockwise=hero_hitbox_for_rotation(&hitbox,sprite,SPRITE->duckcx,SPRITE->duckcy,SPRITE->duckox,SPRITE->duckoy,&nx,&ny);
  struct collision coll;
  if (sprite_detect_collisions(&coll,1,&hitbox,sprite)) {
    SPRITE->duckclock=0.0;
    return;
  }
  
  SPRITE->ducking=0; // You'd think we need a blackout but nope! Since we're changing gravity's direction, the duck button changes.
  SPRITE->walk_cooldown=WALK_COOLDOWN_TIME;
  sprite->x=nx;
  sprite->y=ny;
  if (g.gravity==0x40) sprite->xform^=EGG_XFORM_XREV;
  gravity_rotate(clockwise?1:-1);
  if (g.gravity==0x40) sprite->xform^=EGG_XFORM_XREV;
}
 
static void hero_duck_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->ducksigntileid==0x91) {
    if ((SPRITE->duckclock+=elapsed)>=DUCK_CONFIRM_TIME) {
      hero_duck_commit(sprite);
      return;
    }
  }
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.250;
    if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
  }
}

static void hero_duck_begin(struct sprite *sprite) {

  SPRITE->ducking=1;
  SPRITE->animclock=0.0;
  SPRITE->animframe=0;
  SPRITE->duckclock=0.0;
  
  /* Find the corner candidate cell.
   * Rotation only happens on map corners. Sprites don't count.
   * I'm going to design maps such that the edges match each other, so we shouldn't need to worry about OOB.
   * Just call it invalid if OOB.
   * Locate the two cells below my feet. If one is passable and the other solid, the solid one is our corner candidate.
   */
  double afx=-0.5,bfx=0.5,afy=1.0,bfy=1.0;
  deltaf_plus_gravity(&afx,&afy);
  deltaf_plus_gravity(&bfx,&bfy);
  afx+=sprite->x;
  afy+=sprite->y;
  bfx+=sprite->x;
  bfy+=sprite->y;
  int ax=(int)afx,ay=(int)afy,bx=(int)bfx,by=(int)bfy;
  int candx=-1,candy,otherx,othery; // (candx<0) means there is no corner.
  if ((ax==bx)&&(ay==by)) {
    // Exactly centered. I guess we could fudge one of them out, but meh.
  } else if (
    (ax<0)||(ay<0)||(ax>=NS_sys_mapw)||(ay>=NS_sys_maph)||
    (bx<0)||(by<0)||(bx>=NS_sys_mapw)||(by>=NS_sys_maph)
  ) {
    // One cell OOB. This shouldn't come up.
  } else {
    uint8_t aph=g.map[ay*NS_sys_mapw+ax];
    uint8_t bph=g.map[by*NS_sys_mapw+bx];
    if ((aph==sprite->pass_physics)&&(bph!=sprite->pass_physics)) {
      candx=bx;
      candy=by;
      otherx=ax;
      othery=ay;
    } else if ((aph!=sprite->pass_physics)&&(bph==sprite->pass_physics)) {
      candx=ax;
      candy=ay;
      otherx=bx;
      othery=by;
    } else {
      // Both passable or both vacant, ie the typical "not a corner" case.
    }
  }
  
  /* If we have a corner candidate, calculate our rotated position and confirm it is passable.
   */
  int clockwise=0;
  if (candx>=0) {
    struct aabb hitbox;
    clockwise=hero_hitbox_for_rotation(&hitbox,sprite,candx,candy,otherx,othery,0,0);
    struct collision coll;
    if (sprite_detect_collisions(&coll,1,&hitbox,sprite)) {
      candx=-1;
    }
  }
  
  /* Commit with valid rotation?
   */
  if (candx>=0) {
    SPRITE->ducksignx=candx*NS_sys_tilesize+(NS_sys_tilesize>>1);
    SPRITE->ducksigny=candy*NS_sys_tilesize+(NS_sys_tilesize>>1);
    SPRITE->ducksigntileid=0x91;
    // Transform is of course a huge drama. Based on gravity, and also the "clockwise" response from hero_hitbox_for_rotation().
    // Natural orientation is deasil from down-gravity.
    switch (g.gravity) {
      case 0x40: SPRITE->ducksignxform=clockwise?(EGG_XFORM_YREV):(EGG_XFORM_XREV|EGG_XFORM_YREV); break;
      case 0x10: SPRITE->ducksignxform=clockwise?(EGG_XFORM_SWAP|EGG_XFORM_XREV|EGG_XFORM_YREV):(EGG_XFORM_SWAP|EGG_XFORM_YREV); break;
      case 0x08: SPRITE->ducksignxform=clockwise?(EGG_XFORM_SWAP):(EGG_XFORM_SWAP|EGG_XFORM_XREV); break;
      case 0x02: SPRITE->ducksignxform=clockwise?(EGG_XFORM_XREV):(0); break;
    }
    SPRITE->duckcx=candx;
    SPRITE->duckcy=candy;
    SPRITE->duckox=otherx;
    SPRITE->duckoy=othery;
    
  /* Commit with invalid rotation.
   * Put the indicator at the quantized foot cell.
   */
  } else {
    int cdx=0,cdy=1;
    deltai_plus_gravity(&cdx,&cdy);
    int x=(int)sprite->x+cdx;
    int y=(int)sprite->y+cdy;
    SPRITE->ducksignx=x*NS_sys_tilesize+(NS_sys_tilesize>>1);
    SPRITE->ducksigny=y*NS_sys_tilesize+(NS_sys_tilesize>>1);
    SPRITE->ducksigntileid=0x93;
    SPRITE->ducksignxform=0;
  }
}

/* Set (g.on_goal) if I'm standing on the goal.
 * Clearing it is not my problem.
 */
 
static void hero_check_goal(const struct sprite *sprite) {
  if (!SPRITE->seated) return;
  
  // Determine affected cells. Don't worry about wrapping -- goals won't touch the edges.
  double dya=1.0,dyz=1.0,dxa=sprite->hbl,dxz=sprite->hbr;
  deltaf_plus_gravity(&dxa,&dya);
  deltaf_plus_gravity(&dxz,&dyz);
  int xa=(int)(sprite->x+dxa); if (xa<0) xa=0; else if (xa>=NS_sys_mapw) xa=NS_sys_mapw-1;
  int xz=(int)(sprite->x+dxz); if (xz<0) xz=0; else if (xz>=NS_sys_mapw) xz=NS_sys_mapw-1;
  int ya=(int)(sprite->y+dya); if (ya<0) ya=0; else if (ya>=NS_sys_maph) ya=NS_sys_maph-1;
  int yz=(int)(sprite->y+dyz); if (yz<0) yz=0; else if (ya>=NS_sys_maph) ya=NS_sys_maph-1;
  
  // (xa,ya)..(xz,yz) should be a straight line. Advance whichever axis doesn't match. Any goal tile means the answer is Yes.
  for (;;) {
    uint8_t physics=g.map[ya*NS_sys_mapw+xa];
    if (physics==NS_physics_goal) { g.on_goal=1; return; }
    int xok=0,yok=0;
    if (xa<xz) xa++; else if (xa>xz) xa--; else xok=1;
    if (ya<yz) ya++; else if (ya>yz) ya--; else yok=1;
    if (xok&&yok) return;
  }
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  // Inversion cooldown suppresses all else, even (especially!) gravity.
  if (SPRITE->invert_cooldown) {
    if ((SPRITE->animclock+=elapsed)>=INVERT_ANIMTIME) {
      SPRITE->animclock-=INVERT_ANIMTIME;
      if (--(SPRITE->animframe)<0) {
        SPRITE->invert_cooldown=0;
        SPRITE->pushing=0;
      }
    }
    return;
  }

  // Walking.
  if (!SPRITE->ducking) {
    if (SPRITE->walk_cooldown>0.0) {
      if (SPRITE->walking) hero_walk_end(sprite);
      if (!(g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT|EGG_BTN_UP|EGG_BTN_DOWN))) {
        SPRITE->walk_cooldown=0.0; // instant freeze
      } else {
        SPRITE->walk_cooldown-=elapsed;
      }
    } else {
      int lbtn,rbtn;
      hero_walk_buttons(&lbtn,&rbtn);
      int btn=g.input&(lbtn|rbtn);
      if (btn==lbtn) hero_walk(sprite,-1,elapsed);
      else if (btn==rbtn) hero_walk(sprite,1,elapsed);
      else if (SPRITE->walking) hero_walk_end(sprite);
    }
  }
  
  // Ducking.
  if (SPRITE->ducking) {
    if (!(g.input&hero_duck_button())) {
      SPRITE->ducking=0;
    } else {
      hero_duck_update(sprite,elapsed);
    }
  } else if (SPRITE->seated) {
    if (g.input&hero_duck_button()) {
      if (SPRITE->walking) hero_walk_end(sprite);
      hero_duck_begin(sprite);
      hero_duck_update(sprite,elapsed);
    }
  }
  
  // Jump or gravity.
  if (SPRITE->seated&&(g.input&EGG_BTN_SOUTH)&&!SPRITE->jump_blackout&&!SPRITE->ducking) {
    hero_jump_begin(sprite,elapsed);
  } else if (SPRITE->jumping) {
    hero_jump_update(sprite,elapsed);
  } else {
    if (!(g.input&EGG_BTN_SOUTH)) SPRITE->jump_blackout=0;
    hero_gravity_update(sprite,elapsed);
  }
  
  sprite_force_ib(sprite);
  hero_check_goal(sprite);
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {

  uint8_t tileid=sprite->tileid;
  uint8_t xform=xform_plus_gravity(sprite->xform);
  //TODO dead hat animation
  if (SPRITE->pushing) {
    if (SPRITE->animframe>=4) return; // Briefly invisible.
    tileid+=5+SPRITE->animframe;
  } else if (SPRITE->ducking) {
    tileid+=9;
  } else if (SPRITE->jumping) {
    tileid+=3;
  } else if (!SPRITE->seated) {
    tileid+=4;
  } else if (SPRITE->walking) {
    switch (SPRITE->animframe) {
      case 1: tileid+=1; break;
      case 3: tileid+=2; break;
    }
  }
  
  graf_tile(&g.graf,x,y,tileid,xform);
  int headdx=0,headdy=-NS_sys_tilesize;
  deltai_plus_gravity(&headdx,&headdy);
  graf_tile(&g.graf,x+headdx,y+headdy,tileid-0x10,xform);
  
  if (SPRITE->ducking) {
    graf_tile(&g.graf,SPRITE->ducksignx,SPRITE->ducksigny,SPRITE->ducksigntileid+SPRITE->animframe,SPRITE->ducksignxform);
  }
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
