#include <stdlib.h>
#include <string.h>
#include "demo.h"
// /home/allaptop/dev/notcurses_github/src/demo/input.c

const int CHUNKS_VERT = 3;		// 6
const int CHUNKS_HORZ = 6;		// 12
//#define MOVES 20

// cursor position
int curs_x=2;
int curs_y=2;

// grid size
int chunk_x=2;
int chunk_y=2;

// Number of letter per grid
#define NUMLETTER 70
#define NUMLETTER2 140
#define MAXMSG 1000
#define MAXGRID 40

void shuffle(char *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          char t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

void rotate(char *array, int n, int d)
{
    if (n > 1) 
    {
		char a[NUMLETTER2];
		
		for (int i = 0; i < n; i++) 
        {
			a[i] = array[i];
		}
        for (int i = 0; i < d; i++) 
        {
			a[i+n] = a[i];
		}
		
		for (int i = 0; i < n; i++) 
        {
			array[i] = a[i+d];
		}
    }
}

struct ncplane** CHUNKS = NULL; 
char vdata[MAXGRID][NUMLETTER];
int  vidx_text[MAXGRID] = {0};
char vtext[MAXGRID][MAXMSG] = {0};

int vidx_msg = 0;
char vmsg[MAXMSG] = {0};

static int fill_chunk(struct ncplane* n, int idx,   int chunkx,  int chunky, bool create_data);

int 	grid_no=0; // 0 to CHUNKS_VERT * CHUNKS_HORZ-1
int 	get_grid_no() {return grid_no;}
void 	set_grid_no(int i) {grid_no = i;}
int 	get_number_of_grid() {return CHUNKS_VERT * CHUNKS_HORZ;}

uint32_t process_inputc(struct notcurses* nc)
{
	ncinput ni;
	uint32_t id = 0;
	id = demo_getc_nblock(nc, &ni);
	
	bool redraw = false;
	if (id == NCKEY_ENTER)
	{
		int cnt=0;
		bool done = false;
		for(int y=0;y<chunk_y-3;y++)
		{
			if (done) break;
			for(int x=0;x<chunk_x-2;x++)
			{
				if (done) break;
				if (curs_x == x && curs_y == y)
				{
					done = true;
					break;
				}
				cnt++;
			}
		}

		for(int i=0;i<CHUNKS_VERT * CHUNKS_HORZ;i++)
		{
			vtext[i][vidx_text[i]] = vdata[i][cnt];
			vidx_text[i]++;
		}
		
		// TODO - receive the grid number where data is for next input
		int gid = get_grid_no();
		vmsg[vidx_msg] = vdata[gid][cnt];
		vidx_msg++;
		
		// TEST show all grid messages 
		FILE* fptr;
		fptr = fopen("tmpinput.txt", "w");
		for(int i=0;i<CHUNKS_VERT * CHUNKS_HORZ;i++)
		{
			fprintf(fptr, "%2d:", i+1);
			for(int j=0;j<vidx_text[i];j++)
				fprintf(fptr, "%c", vtext[i][j]);
			fprintf(fptr, "\n");
		}
		
		fprintf(fptr, "msg:");
		for(int j=0;j<vidx_msg;j++)
			fprintf(fptr, "%c", vmsg[j]);
		fprintf(fptr, "\n");
		
		fclose(fptr);
	}
	else if (id == 'w')
	{
		curs_y--;
		if (curs_y < 0) curs_y = 0;
		redraw = true;
	}
	else if (id == 's')
	{
		curs_y++;
		if (curs_y >= chunk_y-3) curs_y = chunk_y-4;
		redraw = true;
	}	
	else if (id == 'a')
	{
		curs_x--;
		if (curs_x <= 0) curs_x = 0;
		redraw = true;
	}
	else if (id == 'd')
	{
		curs_x++;
		if (curs_x >= chunk_x-2) curs_x = chunk_x-3;
		redraw = true;	
	}
	
	if (redraw)
	{
		for(int z = 0 ; z < CHUNKS_VERT * CHUNKS_HORZ ; ++z)
		{
			if (CHUNKS[z]!=NULL)
			if (fill_chunk(CHUNKS[z], z, chunk_x, chunk_y, false))
			{
			}
		}	
	}
	return id;

}

/*
static int move_square(struct notcurses* nc, struct ncplane* chunk, int* holey, int* holex, uint64_t movens)
{
  int newholex, newholey;
  ncplane_yx(chunk, &newholey, &newholex);
  // we need to move from newhole to hole over the course of movetime
  int deltay, deltax;
  deltay = *holey - newholey;
  deltax = *holex - newholex;
  // divide movetime into abs(max(deltay, deltax)) units, and move delta
  int units = abs(deltay) > abs(deltax) ? abs(deltay) : abs(deltax);
  movens /= units;
  struct timespec movetime;
  ns_to_timespec(movens, &movetime);
  int i;
  int targy = newholey;
  int targx = newholex;
  deltay = deltay < 0 ? -1 : deltay == 0 ? 0 : 1;
  deltax = deltax < 0 ? -1 : deltax == 0 ? 0 : 1;
  
  // FIXME do an adaptive time, like our fades, so we whip along under load
  for(i = 0 ; i < units ; ++i)
  {
    targy += deltay;
    targx += deltax;
    ncplane_move_yx(chunk, targy, targx);
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &movetime);
  }
  
  *holey = newholey;
  *holex = newholex;
  return 0;
}

// we take demodelay * 5 to play MOVES moves
static int play(struct notcurses* nc, struct ncplane** chunks, uint64_t startns)
{
  const uint64_t delayns = timespec_to_ns(&demodelay);
  const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  // we don't want to spend more than demodelay * 5
  const uint64_t totalns = delayns * 5; //5;
  const uint64_t deadline_ns = startns + totalns;
  const uint64_t movens = totalns / MOVES;
  int hole = rand() % chunkcount;
  int holex, holey;
  ncplane_yx(chunks[hole], &holey, &holex);
  ncplane_destroy(chunks[hole]);
  chunks[hole] = NULL;
  int m;
  int lastdir = -1;

  for(m = 0 ; m < MOVES ; ++m)
  {
    uint64_t now = clock_getns(CLOCK_MONOTONIC);
    if(now >= deadline_ns){
      break;
    }
    int mover = chunkcount;
    int direction;
    
    do
    {
      direction = rand() % 4;
      switch(direction){
        case 3: // up
          if(lastdir != 1 && hole >= CHUNKS_HORZ){ mover = hole - CHUNKS_HORZ; } break;
        case 2: // right
          if(lastdir != 0 && hole % CHUNKS_HORZ < CHUNKS_HORZ - 1){ mover = hole + 1; } break;
        case 1: // down
          if(lastdir != 3 && hole < chunkcount - CHUNKS_HORZ){ mover = hole + CHUNKS_HORZ; } break;
        case 0: // left
          if(lastdir != 2 && hole % CHUNKS_HORZ){ mover = hole - 1; } break;
      }
    }while(mover == chunkcount);
    
    lastdir = direction;
    int err = move_square(nc, chunks[mover], &holey, &holex, movens);
    if(err){
      return err;
    }
    chunks[hole] = chunks[mover];
    chunks[mover] = NULL;
    hole = mover;
  }
  return 0;
}
*/

static int fill_chunk(struct ncplane* n, int idx,   int chunkx,  int chunky, bool create_data)
{
  const int hidx = idx % CHUNKS_HORZ;
  const int vidx = idx / CHUNKS_HORZ;
  unsigned maxy, maxx;
  ncplane_dim_yx(n, &maxy, &maxx);
  uint64_t channels = 0;
  int r = 64 + hidx * 10;
  int b = 64 + vidx * 30;
  int g = 225 - ((hidx + vidx) * 12);
  ncchannels_set_fg_rgb8(&channels, r, g, b);
  uint32_t ul = 0, ur = 0, ll = 0, lr = 0;
  ncchannel_set_rgb8(&ul, r, g, b);
  ncchannel_set_rgb8(&lr, r, g, b);
  ncchannel_set_rgb8(&ur, g, b, r);
  ncchannel_set_rgb8(&ll, b, r, g);
  int ret = 0;
  if(notcurses_canutf8(ncplane_notcurses(n)))
  {
    if(ncplane_gradient2x1(n, -1, -1, 0, 0, ul, ur, ll, lr) <= 0){
      ret = -1;
    }
    ret |= ncplane_double_box(n, 0, channels, maxy - 1, maxx - 1, 0);
  }
  else
  {
    if(ncplane_gradient(n, -1, -1, 0, 0, " ", NCSTYLE_NONE, ul, ur, ll, lr) <= 0){
      ret = -1;
    }
    ret |= ncplane_ascii_box(n, 0, channels, maxy - 1, maxx - 1, 0);
  }
  
  if (create_data)
  {
	  for(int i=0;i<NUMLETTER;i++)
	  {
		  vdata[idx][i]='0'+i;
	  }
  }
  
  
  if(maxx >= 4 && maxy >= 3)
  {
    // don't zero-index to viewer
    if (create_data)
		rotate(&vdata[idx][0], NUMLETTER, idx);
		
    int cnt=0;

    ret |= (ncplane_printf_yx(n, 1, (maxx - 1 - (int)(chunkx/2)), "%02d", idx + 1) < 0);
    for(int y=0;y<chunky-3;y++)
    for(int x=0;x<chunkx-2;x++)
    {
		if (curs_x == x && curs_y == y)
		{
			ncplane_on_styles(n, NCSTYLE_BOLD);
		}
	
		if (cnt < NUMLETTER)
		{
			ret |= (ncplane_printf_yx(n, 2+y, x+1, "%c", vdata[idx][cnt]) < 0);
		}
		else
			ret |= (ncplane_printf_yx(n, 2+y, x+1, " ") < 0);
			
		if (curs_x == x && curs_y == y)
		{
			ncplane_off_styles(n, NCSTYLE_BOLD);
		}
		
		cnt++;
	}
  }
  return ret;
}

static int draw_bounding_box(struct ncplane* n, int yoff, int xoff, int chunky, int chunkx)
{
  int ret;
  uint64_t channels = 0;
  ncchannels_set_fg_rgb8(&channels, 180, 80, 180);
  //channels_set_bg_rgb8(&channels, 0, 0, 0);
  ncplane_cursor_move_yx(n, yoff, xoff);
  if(notcurses_canutf8(ncplane_notcurses(n)))
  {
    ret = ncplane_rounded_box(n, 0, channels,
                              CHUNKS_VERT * chunky + yoff + 1,
                              CHUNKS_HORZ * chunkx + xoff + 1, 0);
  }
  else
  {
    ret = ncplane_ascii_box(n, 0, channels,
                            CHUNKS_VERT * chunky + yoff + 1,
                            CHUNKS_HORZ * chunkx + xoff + 1, 0);
  }
  return ret;
}

//--------------------------------------------------------------------------------
// Entry:
// make a bunch of boxes with gradients and use them to play a sliding puzzle.
//--------------------------------------------------------------------------------
int sliders_demo(struct notcurses* nc, uint64_t startns)
{
  int ret = -1, z;
  unsigned maxx, maxy;
  
  //"Get standard plane plus dimensions dimensions."
  struct ncplane* n = notcurses_stddim_yx(nc, &maxy, &maxx);
  int chunky, chunkx;
  
  // want at least 2x2 for each sliding chunk
  if(maxy < CHUNKS_VERT * 2 || maxx < CHUNKS_HORZ * 2)
  {
    return -1;
  } 
  
  // we want an nxm grid of chunks with a border. the leftover space will be unused
  chunky = (maxy - 2) / CHUNKS_VERT;
  chunkx = (maxx - 2) / CHUNKS_HORZ;
  // want an even width so our 2-digit IDs are centered exactly
  chunkx -= (chunkx % 2);
  // don't allow them to be too rectangular, but keep aspect ratio in mind!
  if(chunky > chunkx + 1){
    chunky = chunkx + 1;
  }else if(chunkx > chunky * 2){
    chunkx = chunky * 2;
  }
  
  chunk_x = chunkx;
  chunk_y = chunky;
  
  int wastey = ((maxy - 2) - (CHUNKS_VERT * chunky)) / 2;
  int wastex = ((maxx - 2) - (CHUNKS_HORZ * chunkx)) / 2;
  const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  struct ncplane** chunks = malloc(sizeof(*chunks) * chunkcount);
  if(chunks == NULL)
  {
    return -1;
  }
  
  CHUNKS = chunks;
  memset(chunks, 0, sizeof(*chunks) * chunkcount);
  
  //----------------------------------------------------------
  // create N grid (planes) in a color pattern, in order
  //----------------------------------------------------------
  int cy, cx;
  for(cy = 0 ; cy < CHUNKS_VERT ; ++cy)
  {
    for(cx = 0 ; cx < CHUNKS_HORZ ; ++cx)
    {
      const int idx = cy * CHUNKS_HORZ + cx;
      
      struct ncplane_options nopts = 
      {
        .y = cy * chunky + wastey + 1,
        .x = cx * chunkx + wastex + 1,
        .rows = chunky,
        .cols = chunkx,
      };
      
      //-----------------------------
      // one grid = one plane
      //-----------------------------
      chunks[idx] = ncplane_create(n, &nopts);
      if(chunks[idx] == NULL)
      {
        goto done;
      }
      
      // draw grid[idx]
      if(fill_chunk(chunks[idx], idx, chunkx, chunky, true))
      {
        goto done;
      }
    }
  }
  
  //----------------------------------------------------------
  // draw a box around the playing area in standard plane
  //----------------------------------------------------------
  if(draw_bounding_box(n, wastey, wastex, chunky, chunkx))
  {
    goto done;
  }
  
  DEMO_RENDER(nc);
  
/*
  // shuffle up the chunks
  int i;
  demo_nanosleep(nc, &demodelay);
  for(i = 0 ; i < 200 ; ++i)
  {
    int i0 = rand() % chunkcount;
    int i1 = rand() % chunkcount;
    while(i1 == i0){
      i1 = rand() % chunkcount;
    }
    int targy0, targx0;
    int targy, targx;
    ncplane_yx(chunks[i0], &targy0, &targx0);
    ncplane_yx(chunks[i1], &targy, &targx);
    struct ncplane* t = chunks[i0];
    ncplane_move_yx(t, targy, targx);
    chunks[i0] = chunks[i1];
    ncplane_move_yx(chunks[i0], targy0, targx0);
    chunks[i1] = t;
    DEMO_RENDER(nc);
  }
  // move chunks
  ret = play(nc, chunks, startns);
*/
    
	// LOOP input/render
	uint32_t id=0;
	while (id != 'q')
	{
		id = process_inputc(nc);
		DEMO_RENDER(nc);
	}


done:
  for(z = 0 ; z < chunkcount ; ++z)
  {
    ncplane_destroy(chunks[z]);
  }
  free(chunks);
  return ret;
}
