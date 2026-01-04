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
  if (egg_texture_load_image(g.texid_font=egg_texture_new(),RID_image_font)<0) return -1;
  if (egg_texture_load_image(g.texid_title=egg_texture_new(),RID_image_title)<0) return -1;
  if (egg_texture_load_raw(g.texid_bgbits=egg_texture_new(),FBW,FBH,FBW<<2,0,0)<0) return -1;

  srand_auto();

  //game_reset(1);
  hello_begin();

  return 0;
}

/* Client hooks.
 */

void egg_client_notify(int k,int v) {
}

static char get_active_modal() {
  if (g.hello.active) return 'h';
  if (g.gameover.active) return 'o';
  if (g.pause.active) return 'p';
  return 'g';
}

static void update_modal(char which,double elapsed) {
  switch (which) {
    case 'h': hello_update(elapsed); break;
    case 'o': gameover_update(elapsed); break;
    case 'p': pause_update(elapsed); break;
    case 'g': game_update(elapsed); break;
    default: egg_terminate(1);
  }
}

void egg_client_update(double elapsed) {
  g.pvinput=g.input;
  g.input=egg_input_get_one(0);
  
  // AUX1 summons or dismisses the pause menu, if game in progress.
  if ((g.input&EGG_BTN_AUX1)&&!(g.pvinput&EGG_BTN_AUX1)) {
    if (!g.hello.active&&!g.gameover.active) {
      if (g.pause.active) pause_end();
      else pause_begin();
    }
  }
  
  /* Update the active modal.
   * If that changes which modal is active, then update the new one.
   * We don't want to render anything without it being updated first.
   * But we only do that once, not in a loop. In case modals get broken and summon/dismiss haywire.
   */
  char first_active=get_active_modal();
  update_modal(first_active,elapsed);
  char second_active=get_active_modal();
  if (second_active!=first_active) {
    g.pvinput=g.input;
    update_modal(second_active,elapsed);
  }
}

void egg_client_render() {
  graf_reset(&g.graf);
  if (g.hello.active) {
    hello_render();
  } else if (g.gameover.active) {
    gameover_render();
  } else {
    game_render();
    if (g.pause.active) pause_render();
  }
  graf_flush(&g.graf);
}

/* Resources.
 */

int res_search(int tid,int rid) {
  int lo=0,hi=g.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_entry *res=g.resv+ck;
         if (tid<res->tid) hi=ck;
    else if (tid>res->tid) lo=ck+1;
    else if (rid<res->rid) hi=ck;
    else if (rid>res->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int res_get(void *dstpp,int tid,int rid) {
  int p=res_search(tid,rid);
  if (p<0) return 0;
  const struct rom_entry *res=g.resv+p;
  *(const void**)dstpp=res->v;
  return res->c;
}

/* Sound.
 */
 
void inv_sound(int rid) {
  egg_play_sound(rid,1.0f,0.0f);
}

void inv_song(int rid) {
  if (rid==g.song_playing) return;
  g.song_playing=rid;
  egg_play_song(1,rid,1,0.400f,0.0f);
  if (g.song_playing==RID_song_inversion) {
    if (g.song_lead==NS_physics_vacant) {
      egg_song_set(1,1,EGG_SONG_PROP_TRIM,0.388f);
      egg_song_set(1,2,EGG_SONG_PROP_TRIM,0.0f);
    } else {
      egg_song_set(1,1,EGG_SONG_PROP_TRIM,0.0f);
      egg_song_set(1,2,EGG_SONG_PROP_TRIM,0.286f);
    }
  }
}

void inv_song_lead(uint8_t lead) {
  if (lead==g.song_lead) return;
  g.song_lead=lead;
  if (g.song_playing==RID_song_inversion) {
    if (g.song_lead==NS_physics_vacant) {
      egg_song_set(1,1,EGG_SONG_PROP_TRIM,0.400f);
      egg_song_set(1,2,EGG_SONG_PROP_TRIM,0.0f);
    } else {
      egg_song_set(1,1,EGG_SONG_PROP_TRIM,0.0f);
      egg_song_set(1,2,EGG_SONG_PROP_TRIM,0.400f);
    }
  }
}
