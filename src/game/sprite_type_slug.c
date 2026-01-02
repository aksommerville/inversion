/* sprite_type_slug.c
 * We're basically just "dummy", but we react to gravity, and presumably are solid.
 */

#include "game/inversion.h"

#define GRAVITY_LIMIT  16.0 /* m/s */
#define GRAVITY_ACCEL  30.0 /* m/s**2 */

struct sprite_slug {
  struct sprite hdr;
  double gravity;
  int seated;
};

#define SPRITE ((struct sprite_slug*)sprite)

static int _slug_init(struct sprite *sprite) {
  SPRITE->seated=0;
  return 0;
}

static void _slug_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->gravity+=GRAVITY_ACCEL*elapsed)>GRAVITY_LIMIT) SPRITE->gravity=GRAVITY_LIMIT;
  double dx=0.0,dy=SPRITE->gravity*elapsed;
  deltaf_plus_gravity(&dx,&dy);
  if (!sprite_move(sprite,dx,dy)) {
    if (!SPRITE->seated) {
      SPRITE->seated=1;
      SPRITE->gravity=0.0;
    }
  } else {
    if (SPRITE->seated) {
      SPRITE->seated=0;
    }
  }
}

const struct sprite_type sprite_type_slug={
  .name="slug",
  .objlen=sizeof(struct sprite_slug),
  .init=_slug_init,
  .update=_slug_update,
};
