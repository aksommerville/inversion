#include "inversion.h"

/* End.
 */
 
void gameover_end() {
  g.gameover.active=0;
}

/* Begin.
 */
 
void gameover_begin() {
  g.gameover.active=1;
}

/* Update.
 */
 
void gameover_update(double elapsed) {
  //TODO
}

/* Render.
 */
 
void gameover_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x008000ff);//TODO
}
