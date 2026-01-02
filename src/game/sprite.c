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
  
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,serial,serialc)>=0) {
    struct cmdlist_entry cmd;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case CMD_sprite_tile: sprite->tileid=cmd.arg[0]; sprite->xform=cmd.arg[1]; break;
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

/* Render.
 */
 
void sprites_render() {
  graf_set_input(&g.graf,g.texid_tiles);
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    int x=(int)(sprite->x*NS_sys_tilesize);
    int y=(int)(sprite->y*NS_sys_tilesize);
    if (sprite->type->render) sprite->type->render(sprite,x,y);
    else graf_tile(&g.graf,x,y,sprite->tileid,xform_plus_gravity(sprite->xform));
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
    case 0x10: return xform;//TODO clockwise
    case 0x08: return xform;//TODO deasil
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
