/* modals.h
 * We're being pretty stupid about modals.
 * There are three, and they are all implemented bespoke, with main checking them all in turn.
 * Modals read their state off (g), but it's a struct that we declare here.
 */
 
#ifndef MODALS_H
#define MODALS_H

/* Tile-based text, shared among modals.
 * This is a jam game and will never be translated, so the text is all hard-coded.
 ********************************************************************/

struct label {
  const char *src;
  int srcc;
  int x,y; // Center of first glyph. Glyphs are 8x8 with the right and bottom edge always empty.
  uint32_t rgba;
  int id; // For owner's use.
};

struct label_list {
  struct label *v;
  int c,a;
};

/* Adding a label invalidates addresses of previous ones.
 * (src) is BORROW: It must be constant and immortal. I expect only to use inlines.
 * Position is initially (0,0). You'll want to set positions after all labels are created.
 */
struct label *label_new(struct label_list *list,const char *src,int srcc,uint32_t rgba,int id);

void label_list_clear(struct label_list *list);

void label_list_center_all(struct label_list *list);

void label_list_render(struct label_list *list);

/* Hello.
 *****************************************************************/
 
struct hello {
  int active;
  double transitionclock;
  struct label_list labels;
  char hs[32];
  int hsc;
};

void hello_end();
void hello_begin();
void hello_update(double elapsed);
void hello_render();

/* Gameover.
 *****************************************************************/
 
struct gameover {
  int active;
  char score[6];
  int new_high_score;
  char msg[32];
  int msgc;
  struct label_list labels;
  double blink_clock;
  struct label *blink_label;
};

void gameover_end();
void gameover_begin();
void gameover_update(double elapsed);
void gameover_render();

/* Pause.
 ******************************************************************/
 
struct pause {
  int active;
  struct label_list labels;
  int optp; // Index in (labels.v).
};

void pause_end();
void pause_begin();
void pause_update(double elapsed); // Since main summons us, it will dismiss us too.
void pause_render(); // Main should render the game first.

#endif
