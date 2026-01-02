#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;

struct sprite {
  const struct sprite_type *type;
  int defunct;
  double x,y;
  double hbl,hbr,hbt,hbb; // Hitbox in positive-gravity orientation.
  int solid;
  int rid;
  const uint8_t *arg;
  const void *serial;
  int serialc;
  uint8_t tileid,xform;
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  
  /* Default is a single tile from RID_image_tiles, with (sprite->xform) adjusted per gravity.
   * If you do anything raw or a different image, restore RID_image_tiles before returning.
   */
  void (*render)(struct sprite *sprite,int x,int y);
};

struct sprite *sprite_spawn(
  double x,double y,
  int rid,
  const uint8_t *arg,
  const struct sprite_type *type,
  const void *serial,int serialc
);

void sprites_update(double elapsed);
void sprites_render();

void sprites_kill_all();

const struct sprite_type *sprite_type_by_id(int sprtype);
const struct sprite_type *sprite_type_from_serial(const void *serial,int serialc);

#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_SPRTYPE
#undef _

uint8_t xform_plus_gravity(uint8_t xform);
void deltai_plus_gravity(int *dx,int *dy);
void deltaf_plus_gravity(double *dx,double *dy);

#endif
