#include "inversion.h"

#define BGCOLOR 0x7ea1d3ff
#define TRANSITION_TIME 0.750

/* End.
 */
 
void hello_end() {
  g.hello.active=0;
}

/* Generate high score text.
 * Always returns in 0..dsta.
 */
 
static int hello_generate_high_score(char *dst,int dsta) {
  //TODO
  return 0;
}

/* Begin.
 */
 
void hello_begin() {
  g.hello.active=1;
  inv_song(RID_song_around_the_corner);
  g.pause_jump_blackout=1;
  g.hello.transitionclock=0.0;
  g.hello.hsc=hello_generate_high_score(g.hello.hs,sizeof(g.hello.hs));
  
  if (!g.hello.labels.c) {
    struct label *label;
    if (label=label_new(&g.hello.labels,"By AK Sommerville",-1,0x202040ff,0)) {
      label->y=47;
      label->x=(FBW>>1)-(label->srcc*4);
    }
    if (label=label_new(&g.hello.labels,"January 2026",-1,0x202040ff,0)) {
      label->y=57;
      label->x=(FBW>>1)-(label->srcc*4);
    }
    if (label=label_new(&g.hello.labels,"Uplifting Jam #6",-1,0x202040ff,0)) {
      label->y=67;
      label->x=(FBW>>1)-(label->srcc*4);
    }
    if (label=label_new(&g.hello.labels,g.hello.hs,g.hello.hsc,0x000000ff,0)) {
      label->y=85;
      label->x=(FBW>>1)-(label->srcc*4);
    }
  }
}

/* Dismiss (begin game).
 */
 
static void hello_dismiss() {
  g.hello.transitionclock=TRANSITION_TIME;
  game_reset(1);
  game_no_fade_in();
}

/* Update.
 */
 
void hello_update(double elapsed) {
  if (g.hello.transitionclock>0.0) {
    if ((g.hello.transitionclock-=elapsed)<=0.0) {
      hello_end();
    }
  } else {
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) hello_dismiss();
    else if ((g.input&EGG_BTN_AUX1)&&!(g.pvinput&EGG_BTN_AUX1)) hello_dismiss();
  }
}

/* Render.
 */

#define LETTERX 8
#define LETTERY 8
#define LETTERW 15
#define LETTERH 23
const double fallh[9]={100.0,110.0,90.0,100.0,105.0,95.0,108.0,100.0,92.0};
 
void hello_render() {

  /* During the outward transition, the game has been reset already.
   * Render game, then draw our background fading out.
   * Draw the title as 9 sprites tumbling down.
   * Labels disappear immediately.
   */
  if (g.hello.transitionclock>0.0) {
    game_render();
    int alpha=(int)((g.hello.transitionclock*255.0)/TRANSITION_TIME);
    if (alpha>0) {
      if (alpha>0xff) alpha=0xff;
      graf_fill_rect(&g.graf,0,0,FBW,FBH,(BGCOLOR&0xffffff00)|alpha);
    }
    graf_set_input(&g.graf,g.texid_title);
    double trp=1.0-g.hello.transitionclock/TRANSITION_TIME;
    int x=LETTERX;
    int i=9; for (;i-->0;x+=LETTERW+1) {
      int dsty=LETTERY+(int)(fallh[i]*trp);
      graf_decal(&g.graf,x,dsty,x,LETTERY,LETTERW,LETTERH);
    }
    
  /* Before the transition, it's considerably simpler.
   */
  } else {
    graf_fill_rect(&g.graf,0,0,FBW,FBH,BGCOLOR);
    graf_set_input(&g.graf,g.texid_title);
    graf_decal(&g.graf,0,0,0,0,FBW,FBH);
    label_list_render(&g.hello.labels);
  }
}
