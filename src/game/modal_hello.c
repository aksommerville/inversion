#include "inversion.h"

/* End.
 */
 
void hello_end() {
  g.hello.active=0;
}

/* Begin.
 */
 
void hello_begin() {
  g.hello.active=1;
  inv_song(RID_song_around_the_corner);
  g.pause_jump_blackout=1;
}

/* Dismiss (begin game).
 */
 
static void hello_dismiss() {
  hello_end();
  game_reset(1);
}

/* Update.
 */
 
void hello_update(double elapsed) {
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) hello_dismiss();
  else if ((g.input&EGG_BTN_AUX1)&&!(g.pvinput&EGG_BTN_AUX1)) hello_dismiss();
}

/* Render.
 */
 
void hello_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x0080ffff);//TODO
}
