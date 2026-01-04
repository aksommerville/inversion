#include "inversion.h"

/* New label.
 */
 
struct label *label_new(struct label_list *list,const char *src,int srcc,uint32_t rgba,int id) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (list->c>=list->a) {
    int na=list->a+8;
    if (na>INT_MAX/sizeof(struct label)) return 0;
    void *nv=realloc(list->v,sizeof(struct label)*na);
    if (!nv) return 0;
    list->v=nv;
    list->a=na;
  }
  struct label *label=list->v+list->c++;
  memset(label,0,sizeof(struct label));
  label->src=src;
  label->srcc=srcc;
  label->rgba=rgba;
  label->id=id;
  return label;
}

/* Clear list.
 */

void label_list_clear(struct label_list *list) {
  list->c=0;
}

/* Center all labels.
 */

void label_list_center_all(struct label_list *list) {
  if (list->c<1) return;
  int h=list->c*8;
  int y=(FBH>>1)-(h>>1);
  struct label *label=list->v;
  int i=list->c;
  for (;i-->0;label++,y+=8) {
    label->y=y;
    label->x=(FBW>>1)-(label->srcc*4)+4;
  }
}

/* Render list.
 */

void label_list_render(struct label_list *list) {
  graf_set_input(&g.graf,g.texid_font);
  struct label *label=list->v;
  int i=list->c;
  for (;i-->0;label++) {
    graf_set_tint(&g.graf,label->rgba|0xff);
    graf_set_alpha(&g.graf,label->rgba&0xff);
    const char *src=label->src;
    int li=label->srcc;
    int x=label->x;
    int y=label->y;
    for (;li-->0;src++,x+=8) graf_tile(&g.graf,x,y,*src,0);
  }
  graf_set_tint(&g.graf,0);
  graf_set_alpha(&g.graf,0xff);
}
