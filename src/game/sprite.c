#include "inversion.h"

static const uint8_t arg_default[4]={0};

/* Delete.
 */
 
static void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* Spawn.
 */

struct sprite *sprite_spawn(
  double x,double y,
  int rid,
  const uint8_t *arg,
  const struct sprite_type *type,
  const void *serial,int serialc
) {
  if (!arg) arg=arg_default;
  if (!serial) serialc=res_get(&serial,EGG_TID_sprite,rid);
  if (serialc<0) return 0;
  if (!type) {
    if (!(type=sprite_type_from_serial(serial,serialc))) return 0;
  }
  
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  sprite->type=type;
  sprite->x=x;
  sprite->y=y;
  sprite->rid=rid;
  sprite->arg=arg;
  sprite->serial=serial;
  sprite->serialc=serialc;
  sprite->hbl=-0.5;
  sprite->hbr=0.5;
  sprite->hbt=-0.5;
  sprite->hbb=0.5;
  sprite->pass_physics=NS_physics_vacant;
  
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,serial,serialc)>=0) {
    struct cmdlist_entry cmd;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case CMD_sprite_solid: sprite->solid=1; break;
        case CMD_sprite_tile: sprite->tileid=cmd.arg[0]; sprite->xform=cmd.arg[1]; break;
        case CMD_sprite_hitbox: {
            sprite->hbl=(double)((int8_t)cmd.arg[0])/(double)NS_sys_tilesize;
            sprite->hbr=(double)((int8_t)cmd.arg[1])/(double)NS_sys_tilesize;
            sprite->hbt=(double)((int8_t)cmd.arg[2])/(double)NS_sys_tilesize;
            sprite->hbb=(double)((int8_t)cmd.arg[3])/(double)NS_sys_tilesize;
          } break;
      }
    }
  }
  
  if (type->init&&((type->init(sprite)<0)||sprite->defunct)) {
    sprite_del(sprite);
    return 0;
  }
  
  if (g.spritec>=g.spritea) {
    int na=g.spritea+32;
    if (na>INT_MAX/sizeof(void*)) { sprite_del(sprite); return 0; }
    void *nv=realloc(g.spritev,sizeof(void*)*na);
    if (!nv) { sprite_del(sprite); return 0; }
    g.spritev=nv;
    g.spritea=na;
  }
  g.spritev[g.spritec++]=sprite;
  
  return sprite;
}

/* Update all.
 */

void sprites_update(double elapsed) {
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->defunct||!sprite->type->update) continue;
    sprite->type->update(sprite,elapsed);
  }
  struct sprite **p=g.spritev+g.spritec-1;
  for (i=g.spritec;i-->0;p--) {
    struct sprite *sprite=*p;
    if (!sprite->defunct) continue;
    g.spritec--;
    memmove(p,p+1,sizeof(void*)*(g.spritec-i));
    sprite_del(sprite);
  }
}

/* Partial cocktail sort for render order.
 */
 
static int sprites_rendercmp(const struct sprite *a,const struct sprite *b) {
  // Hero last. And I think that's all we care about.
  if (a->type==&sprite_type_hero) return 1;
  if (b->type==&sprite_type_hero) return -1;
  return 0;
}

static void sprites_sort_partial() {
  if (g.spritec<2) return;
  int first=1,last=g.spritec-1,i=1,done=1;
  for (;i<=last;i++) {
    struct sprite *a=g.spritev[i-1];
    struct sprite *b=g.spritev[i];
    if (sprites_rendercmp(a,b)>0) {
      done=0;
      g.spritev[i-1]=b;
      g.spritev[i]=a;
    }
  }
  if (done) return;
  for (i=g.spritec-1;i-->1;) {
    struct sprite *a=g.spritev[i-1];
    struct sprite *b=g.spritev[i];
    if (sprites_rendercmp(a,b)>0) {
      g.spritev[i-1]=b;
      g.spritev[i]=a;
    }
  }
}

/* Render.
 */
 
void sprites_render() {
  sprites_sort_partial();
  graf_set_input(&g.graf,g.texid_tiles);
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    
    uint8_t xform=0;
    //if (!sprite->type->render) xform=xform_plus_gravity(sprite->xform); // Should sprites render per gravity by default? Having second thoughts.
    int x=(int)(sprite->x*NS_sys_tilesize);
    int y=(int)(sprite->y*NS_sys_tilesize);
    #define DRAWME(xx,yy) { \
      if (sprite->type->render) sprite->type->render(sprite,xx,yy); \
      else graf_tile(&g.graf,xx,yy,sprite->tileid,xform); \
    }
    DRAWME(x,y)
    
    /* If we're within one meter of the edge, draw on the opposite side too, in case it overlaps the screen.
     * Two meters if a custom renderer is in play.
     */
    int edge=NS_sys_tilesize;
    if (sprite->type->render) edge<<=1;
    if (x<edge) {
      DRAWME(x+FBW,y)
      if (y<edge) DRAWME(x+FBW,y+FBH)
      else if (y>FBH-edge) DRAWME(x+FBW,y-FBH)
    } else if (x>FBW-edge) {
      DRAWME(x-FBW,y)
      if (y<edge) DRAWME(x-FBW,y+FBH)
      else if (y>FBH-edge) DRAWME(x-FBW,y-FBH)
    }
    if (y<edge) DRAWME(x,y+FBH)
    else if (y>FBH-edge) DRAWME(x,y-FBH)
    #undef DRAWME
  }
}

/* Kill all.
 */
 
void sprites_kill_all() {
  while (g.spritec>0) {
    g.spritec--;
    sprite_del(g.spritev[g.spritec]);
  }
}

/* Type by id.
 */

const struct sprite_type *sprite_type_by_id(int sprtype) {
  switch (sprtype) {
    #define _(tag) case NS_sprtype_##tag: return &sprite_type_##tag;
    FOR_EACH_SPRTYPE
    #undef _
  }
  return 0;
}

const struct sprite_type *sprite_type_from_serial(const void *serial,int serialc) {
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,serial,serialc)<0) return 0;
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_sprite_type: return sprite_type_by_id((cmd.arg[0]<<8)|cmd.arg[1]); break;
    }
  }
  return 0;
}

/* Modify xform with respect to global gravity.
 */
 
uint8_t xform_plus_gravity(uint8_t xform) {
  switch (g.gravity) {
    case 0x40: {
        if (xform&EGG_XFORM_SWAP) return xform^EGG_XFORM_XREV;
        return xform^EGG_XFORM_YREV;
      }
    case 0x10: return xform^(EGG_XFORM_SWAP|EGG_XFORM_YREV);
    case 0x08: return xform^(EGG_XFORM_SWAP|EGG_XFORM_XREV);
  }
  return xform;
}

void deltai_plus_gravity(int *dx,int *dy) {
  switch (g.gravity) {
    case 0x40: *dy=-*dy; break;
    case 0x10: { int xx=-*dy,yy=*dx; *dx=xx; *dy=yy; } break;
    case 0x08: { int xx=*dy,yy=-*dx; *dx=xx; *dy=yy; } break;
    case 0x02: break;
  }
}

void deltaf_plus_gravity(double *dx,double *dy) {
  switch (g.gravity) {
    case 0x40: *dy=-*dy; break;
    case 0x10: { double xx=-*dy,yy=*dx; *dx=xx; *dy=yy; } break;
    case 0x08: { double xx=*dy,yy=-*dx; *dx=xx; *dy=yy; } break;
    case 0x02: break;
  }
}

/* Force in bounds.
 */
 
void sprite_force_ib(struct sprite *sprite) {
  if (sprite->x<0.0) sprite->x+=NS_sys_mapw;
  else if (sprite->x>NS_sys_mapw) sprite->x-=NS_sys_mapw;
  if (sprite->y<0.0) sprite->y+=NS_sys_maph;
  else if (sprite->y>NS_sys_maph) sprite->y-=NS_sys_maph;
}
