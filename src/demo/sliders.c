#include <stdlib.h>
#include <string.h>
#include "demo.h"

int MODE=0; // input, text
const int CHUNKS_VERT = 3;		// 6
const int CHUNKS_HORZ = 8;		// 12
const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
struct ncplane** chunks = NULL;
int BoxH;
int BoxW;
struct ncplane* nc_text = NULL;

// cursor position
int curs_x=2;
int curs_y=2;

// grid size
int chunk_x=2;
int chunk_y=2;

// Number of letter per grid
// 32-96
#define NUMLETTER 65
#define NUMLETTER2 130
#define MAXMSG 1000
#define MAXGRID 40

int draw_text(struct notcurses* nc);
int draw_grid(struct notcurses* nc, bool create_grid);

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

// TODO dont save as raw - some memory encryption
int vidx_msg = 0;
char vmsg[MAXMSG] = {0};
char vshowmsg[MAXMSG] = {0};
int show_cnt = 0;

static int fill_chunk(struct ncplane* n, int idx,   int chunkx,  int chunky, bool create_data);

// Grid with the good input
int grid_no=0; // 0 to CHUNKS_VERT * CHUNKS_HORZ-1

// INTERFACE
int 	get_grid_no() {return grid_no;}
void 	set_grid_no(int i) {grid_no = i;}
int 	get_number_of_grid() {return CHUNKS_VERT * CHUNKS_HORZ;}

void dumb_grid()
{
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

void make_showmsg()
{
	for(int i=0;i<MAXMSG;i++)
		vshowmsg[i] = ' ';
		
	vshowmsg[0] = vmsg[0];
	for(int i=1;i<vidx_msg;i++)
		if (vmsg[i]!=' ')
			vshowmsg[i] = '*';
	
	for(int i=0;i<vidx_msg-1;i++)
	{
		if ((vmsg[i]==' ') && (vmsg[i+1] != ' '))
		{
			vshowmsg[i+1] = vmsg[i+1];
		}
	}
	
	int value;
	for(int i=0;i<show_cnt;i++)
	{
		value = rand() % (vidx_msg + 1);
		vshowmsg[value] = vmsg[value];
	}
}

uint32_t process_inputc(struct notcurses* nc)
{
	ncinput ni;
	uint32_t id = 0;
	id = demo_getc_nblock(nc, &ni);
	
	bool redraw = false;
	
	if (id == '0')
	{
		MODE = 1 - MODE;
		if (MODE==1) draw_text(nc);
		else draw_grid(nc, false);
	}
	else if (MODE == 0)
	{
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
			
			int gid = get_grid_no();
			vmsg[vidx_msg] = vdata[gid][cnt];
			vidx_msg++;
			
			dumb_grid();
		}
		else if (id == NCKEY_UP)
		{
			curs_y--;
			if (curs_y < 0) curs_y = 0;
			redraw = true;
		}
		else if (id == NCKEY_DOWN)
		{
			curs_y++;
			if (curs_y >= chunk_y-3) curs_y = chunk_y-4;
			redraw = true;
		}	
		else if (id == NCKEY_LEFT)
		{
			curs_x--;
			if (curs_x <= 0) curs_x = 0;
			redraw = true;
		}
		else if (id == NCKEY_RIGHT)
		{
			curs_x++;
			if (curs_x >= chunk_x-2) curs_x = chunk_x-3;
			redraw = true;	
		}
		else if ((id >= 32) && (id <= 126))
		{
			vmsg[vidx_msg] = id;
			vidx_msg++;
			
			for(int i=0;i<CHUNKS_VERT * CHUNKS_HORZ;i++)
			{
				int value = rand() % (NUMLETTER + 1);
				if (id == 32) vtext[i][vidx_text[i]] = ' ' ;
				else vtext[i][vidx_text[i]] = ' ' + value; //random
				vidx_text[i]++;
			}
			
			dumb_grid();
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
	}
	else if (MODE == 1)
	{
		if (id == NCKEY_UP)
		{
			// show next line
			redraw = true;
		}
		else if (id == NCKEY_DOWN)
		{
			// show prev line
			redraw = true;
		}	
		else if (id == NCKEY_LEFT)
		{
			// show prev world
			redraw = true;
		}
		else if (id == NCKEY_RIGHT)
		{
			// show next world
			redraw = true;	
		}
		else if (id == '+')
		{
			// show more letters
			show_cnt++;
			if (show_cnt>MAXMSG) show_cnt = MAXMSG;
			redraw = true;
		}
		else if (id == '-')
		{
			// show less letters
			show_cnt--;
			if (show_cnt<0) show_cnt = 0;
			redraw = true;	
		}
		
		if (redraw)
		{
			draw_text(nc);
		}
	}
	
	return id;
}

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
		  vdata[idx][i]=' '+i;
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

int draw_text(struct notcurses* nc)
{
	int ret = -1;
	unsigned maxx, maxy;

	struct ncplane* n = notcurses_stddim_yx(nc, &maxy, &maxx);
      
    if (nc_text != NULL)
    {
		ncplane_destroy(nc_text);
		nc_text  = NULL;
	}
	
	struct ncplane_options nopts = 
	{
		.y = 2,
		.x = 3,
		.rows = BoxH-2,
		.cols = BoxW-3
	};
	
	nc_text = ncplane_create(n, &nopts);
	if (nc_text == NULL)
	{
		return -1;
	}
 
	make_showmsg();
	
	for(int i=0;i<CHUNKS_VERT * CHUNKS_HORZ;i++)
    {
		if (i==0)
		{
			ret |= (ncplane_printf_yx(nc_text, i, 0, "**: ") < 0);
			for(int j=0;j<vidx_msg;j++)
			{
				ret |= (ncplane_printf_yx(nc_text, i, j+4, "%c", vshowmsg[j]) < 0);
			}
			for(int j=vidx_msg;j<maxx-4;j++)
			{
				ret |= (ncplane_printf_yx(nc_text, i, j+4, " ") < 0);
			}
		}
		else
		{
			ret |= (ncplane_printf_yx(nc_text, i, 0, "%2d: ", i+1) < 0);
			for(int j=0;j<vidx_text[i];j++)
			{
				ret |= (ncplane_printf_yx(nc_text, i, j+4, "%c", vtext[i][j]) < 0);
			}
			for(int j=vidx_text[i];j<maxx-4;j++)
			{
				ret |= (ncplane_printf_yx(nc_text, i, j+4, " ") < 0);
			}
		}
	}
	
	for(int i=CHUNKS_VERT * CHUNKS_HORZ;i<BoxH-2;i++)
    {
		for(int j=0;j<BoxW-3;j++)
		{
			ret |= (ncplane_printf_yx(nc_text, i, j, " ") < 0);
		}
	}
	
	return 0;
}

int draw_grid(struct notcurses* nc, bool create_grid)
{
  int ret = -1;
  unsigned maxx, maxy;
  struct ncplane* n = notcurses_stddim_yx(nc, &maxy, &maxx);
  int chunky, chunkx;
  
  // want at least 2x2 for each sliding chunk
  if(maxy < CHUNKS_VERT * 2 || maxx < CHUNKS_HORZ * 2)
  {
    return -1;
  } 
  
  chunky = (maxy - 2) / CHUNKS_VERT;
  chunkx = (maxx - 2) / CHUNKS_HORZ;
  chunkx -= (chunkx % 2);
  if(chunky > chunkx + 1)
  {
    chunky = chunkx + 1;
  }

  chunk_x = chunkx;
  chunk_y = chunky;
  
  int wastey = ((maxy - 2) - (CHUNKS_VERT * chunky)) / 2;
  int wastex = ((maxx - 2) - (CHUNKS_HORZ * chunkx)) / 2;
  //const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  
  if (chunks != NULL)
  {
	for(int z = 0 ; z < chunkcount ; ++z)
	{
		if (chunks[z] != NULL)
		{
			ncplane_destroy(chunks[z]);
			chunks[z] = NULL;
		}
	}

	free(chunks);
	chunks = NULL;
  }
	
  if(chunks == NULL)
	chunks = malloc(sizeof(*chunks) * chunkcount);
	
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
      if(fill_chunk(chunks[idx], idx, chunkx, chunky, create_grid))
      {
        goto done;
      }
    }
  }
  
  //----------------------------------------------------------
  // draw a box around the playing area in standard plane
  //----------------------------------------------------------
  BoxH = CHUNKS_VERT * chunky + wastey + 1;
  BoxW = CHUNKS_HORZ * chunkx + wastex + 1;
                              
  if(draw_bounding_box(n, wastey, wastex, chunky, chunkx))
  {
    goto done;
  }
  
  DEMO_RENDER(nc);

done:;
  return ret;
}


//--------------------------------------------------------------------------------
// Entry
//--------------------------------------------------------------------------------
int sliders_demo(struct notcurses* nc, uint64_t startns)
{
	int ret = -1;
	ret = draw_grid(nc, true);

	// LOOP input/render
	uint32_t id=0;
	while (id != 'q')
	{
		id = process_inputc(nc);
		DEMO_RENDER(nc);
	}

	if (chunks != NULL)
	{
		for(int z = 0 ; z < chunkcount ; ++z)
		{
			if (chunks[z] != NULL)
			{
				ncplane_destroy(chunks[z]);
				chunks[z] = NULL;
			}
		}
		free(chunks);
	}
	return ret;
}
