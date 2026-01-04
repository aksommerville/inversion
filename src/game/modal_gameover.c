#include "inversion.h"

/* End.
 */
 
void gameover_end() {
  g.gameover.active=0;
}

/* Calculate score.
 */
 
static void generate_score(char *dst,double playtime,int deathc,int skipc) {
  
  // An early speed run, just after enabling the clock, was 90.086. So I'm sure <90 is possible, but 90 is really good.
  // Playing thru with no mistakes, but a substantial amount of dilly-dallying, 222.952.
  // But there's not much else to drive the score, it's mostly just time. Let's bump it up to five minutes.
  const double playtime_worst=300.0;
  const double playtime_best = 90.0;
  
  // Initial stab at the score is just 100k..1M based on playtime.
  double ntime=(playtime-playtime_worst)/(playtime_best-playtime_worst); // 0..1, 1 being good.
  int score=(int)(100000.0*(1.0-ntime)+1000000.0*ntime);
  if (score<100000) score=100000;
  else if (score>999999) score=999999;
  
  // -20k per death and -100k per skip, and clamp at 100k again.
  // Important that we do this after scaling and clamping the time score -- if you die or skip, you can't have a perfect score.
  score-=deathc*20000;
  score-=skipc*100000;
  if (score<100000) score=100000;
  
  dst[0]='0'+(score/100000)%10;
  dst[1]='0'+(score/ 10000)%10;
  dst[2]='0'+(score/  1000)%10;
  dst[3]='0'+(score/   100)%10;
  dst[4]='0'+(score/    10)%10;
  dst[5]='0'+(score       )%10;
}

/* Begin.
 */
 
void gameover_begin() {
  g.gameover.active=1;
  inv_song(0);
  
  generate_score(g.gameover.score,g.playtime,g.deathc,g.skipc);
  if (memcmp(g.gameover.score,g.hiscore,6)>0) {
    memcpy(g.hiscore,g.gameover.score,6);
    egg_store_set("hiscore",7,g.hiscore,6);
    g.gameover.new_high_score=1;
  } else {
    g.gameover.new_high_score=0;
  }
  
  label_list_clear(&g.gameover.labels);
  struct label *label;
  if (label=label_new(&g.gameover.labels,"You win!",-1,0xffffffff,0)) {
    label->y=FBH/3;
    label->x=(FBW>>1)-(label->srcc*4)+4;
  }
  memcpy(g.gameover.msg,"Score: ",7);
  memcpy(g.gameover.msg+7,g.gameover.score,6);
  if (label=label_new(&g.gameover.labels,g.gameover.msg,13,0xffffffff,0)) {
    label->y=(FBH*2)/3-8;
    label->x=(FBW>>1)-(label->srcc*4)+4;
  }
  g.gameover.blink_label=0;
  if (g.gameover.new_high_score) {
    if (label=label_new(&g.gameover.labels,"New high score!",-1,0xffff00ff,0)) {
      label->y=(FBH*2)/3+8;
      label->x=(FBW>>1)-(label->srcc*4)+4;
      g.gameover.blink_label=label;
    }
  }
}

/* Update.
 */
 
void gameover_update(double elapsed) {
  if (g.gameover.blink_label) {
    if ((g.gameover.blink_clock-=elapsed)<=0.0) {
      g.gameover.blink_clock+=0.333;
      g.gameover.blink_label->rgba^=0x00c00000;
    }
  }
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
    gameover_end();
    hello_begin();
  }
}

/* Render.
 */
 
void gameover_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x004000ff);
  label_list_render(&g.gameover.labels);
}
