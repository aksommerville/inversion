#include "inversion.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {

  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size mismatch! metadata=%dx%d header=%dx%d\n",fbw,fbh,FBW,FBH);
    return -1;
  }

  g.romc=egg_rom_get(0,0);
  if (!(g.rom=malloc(g.romc))) return -1;
  egg_rom_get(g.rom,g.romc);
  text_set_rom(g.rom,g.romc);
  
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_entry res;
  while (rom_reader_next(&res,&reader)>0) {
    switch (res.tid) {
      case EGG_TID_map:
      case EGG_TID_sprite:
        {
          if (g.resc>=g.resa) {
            int na=g.resa+32;
            if (na>INT_MAX/sizeof(struct rom_entry)) return -1;
            void *nv=realloc(g.resv,sizeof(struct rom_entry)*na);
            if (!nv) return -1;
            g.resv=nv;
            g.resa=na;
          }
          g.resv[g.resc++]=res;
        } break;
    }
  }
  
  if (egg_texture_load_image(g.texid_tiles=egg_texture_new(),RID_image_tiles)<0) return -1;
  if (egg_texture_load_raw(g.texid_bgbits=egg_texture_new(),FBW,FBH,FBW<<2,0,0)<0) return -1;

  srand_auto();

  game_reset(1);

  return 0;
}

void egg_client_notify(int k,int v) {
}

void egg_client_update(double elapsed) {
  g.pvinput=g.input;
  g.input=egg_input_get_one(0);
  game_update(elapsed);
}

void egg_client_render() {
  graf_reset(&g.graf);
  game_render();
  graf_flush(&g.graf);
}

int res_get(void *dstpp,int tid,int rid) {
  int lo=0,hi=g.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_entry *res=g.resv+ck;
         if (tid<res->tid) hi=ck;
    else if (tid>res->tid) lo=ck+1;
    else if (rid<res->rid) hi=ck;
    else if (rid>res->rid) lo=ck+1;
    else {
      *(const void**)dstpp=res->v;
      return res->c;
    }
  }
  return 0;
}
