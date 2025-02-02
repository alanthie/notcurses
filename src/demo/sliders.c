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

// dict /usr/share/dict/words
#define MAXDICT 105000
#define MAXWORD 20000
int wcnt[10] = {0};
char word1[MAXWORD][2] = {0};
char word2[MAXWORD][3] = {0};
char word3[MAXWORD][4] = {0};
char word4[MAXWORD][5] = {0};
char word5[MAXWORD][6] = {0};
char word6[MAXWORD][7] = {0};
char word7[MAXWORD][8] = {0};
char word8[MAXWORD][9] = {0};
char word9[MAXWORD][10] = {0};
char word10[MAXWORD][11] = {0};

// /home/allaptop/dev/notcurses/build/world-80-days_djvu.txt
int nbookline=0;
#define MAXLINE 200000
char book[MAXLINE][200] = {0};
int read_book()
{
	int len;
    FILE *file_ptr;
    char str[200];

    file_ptr = fopen("/home/allaptop/dev/notcurses/build/world-80-days_djvu.txt", "r");
    if (NULL == file_ptr) 
    {
        printf("File can't be opened \n");
        return -1;
    }

    while (fgets(str, 200, file_ptr) != NULL) 
    {
		len = strlen(str);
        for(int i=0;i<len;i++)
        {
			if (str[i]=='\r') str[i] = 0;
			if (str[i]=='\n') str[i] = 0;
		} 
		
		len = strlen(str);
		if (len>0)
        {
			if (nbookline < MAXLINE)
			{	
				strcpy(&book[nbookline][0], str); 
				nbookline++;
			}
		}
    }

    fclose(file_ptr);
    return 0;
    
}

int ndict=0;
int read_dict()
{
	int len;
    FILE *file_ptr;
    char str[50];

    file_ptr = fopen("/usr/share/dict/words", "r");
    if (NULL == file_ptr) 
    {
        printf("File can't be opened \n");
        return -1;
    }

    while (fgets(str, 50, file_ptr) != NULL) 
    {
        //strcpy(word[ndict], str);
        len = strlen(str);
        for(int i=0;i<len;i++)
        {
			if (str[i]=='\r') str[i] = 0;
			if (str[i]=='\n') str[i] = 0;
		}
		len = strlen(str);
        
        if ((len>0) && (len<=10)  && ((str[0] < 'A') || (str[0] > 'Z')) )
        {
			if (wcnt[len-1] < MAXWORD)
			{	
				if (len==1) 	 {strcpy(word1[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==2) {strcpy(word2[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==3) {strcpy(word3[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==4) {strcpy(word4[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==5) {strcpy(word5[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==6) {strcpy(word6[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==7) {strcpy(word7[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==8) {strcpy(word8[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==9) {strcpy(word9[wcnt[len-1]], str); wcnt[len-1]++;}
				else if (len==10) {strcpy(word10[wcnt[len-1]], str); wcnt[len-1]++;}
        
				ndict++;
				//if (ndict>=10000) break;
				if (ndict>=105000) break;
			}
		}
    }

    fclose(file_ptr);
    return 0;
}

char* get_rnd_bookline(int n)
{
	if (nbookline > 0)
	{
		while (true)
		{
			int value = rand() % (nbookline + 0);
			if (strlen(&book[value][0]) >= n)
			{
				// TEST
			FILE* fptr;
			fptr = fopen("tmpw.txt", "a");
			fprintf(fptr, "%d %s\n", n, &book[value][0]);
			fclose(fptr);
			
				return &book[value][0];
			}
		}
	}
	return 0;
}

char* get_rnd_word(int n)
{
	if ((n>=1) && (n<=10) && (wcnt[n-1] > 0))
	{
		int value = rand() % (wcnt[n-1] + 0);
		if (n==1) 	   return &word1[value][0];
		else if (n==2) return &word2[value][0];
		else if (n==3) return &word3[value][0];
		else if (n==4) return &word4[value][0];
		else if (n==5) return &word5[value][0];
		else if (n==6) return &word6[value][0];
		else if (n==7) return &word7[value][0];
		else if (n==8) return &word8[value][0];
		else if (n==9) return &word9[value][0];
		else if (n==10) return &word10[value][0];
	}
	return 0;
}

bool replace_with_rnd_word(char* p)
{
	int n = 0;
	for(int i=0;i<50;i++)
	{
		if (p[i] != 0)
		{
			if (p[i] != ' ')
				n++;
			else
				break;
		}
		else
			break;
	}
	
	if ((n>=1) && (n<=10))
	{
		char* w = get_rnd_word(n);
		if (w != 0)
		{
			
			// TEST
			FILE* fptr;
			fptr = fopen("tmpw.txt", "a");
			fprintf(fptr, "%d %s\n", n, w);
			fclose(fptr);
	
			for(int i=0;i<n;i++)
			{
				p[i] = w[i];
			}
			return true;
		}
	}
	return false;
}


void write_dict()
{
	FILE* fptr;
	fptr = fopen("tmpdict.txt", "w");
	
	fprintf(fptr, "1\n");
	for(int i=0;i<wcnt[0];i++) fprintf(fptr, "%s\n", word1[i]);
	
	fprintf(fptr, "2\n");
	for(int i=0;i<wcnt[1];i++) fprintf(fptr, "%s\n", word2[i]);
	
	fprintf(fptr, "3\n");
	for(int i=0;i<wcnt[2];i++) fprintf(fptr, "%s\n", word3[i]);
	
	for(int i=0;i<wcnt[3];i++) fprintf(fptr, "%s\n", word4[i]);
	for(int i=0;i<wcnt[4];i++) fprintf(fptr, "%s\n", word5[i]);
	for(int i=0;i<wcnt[5];i++) fprintf(fptr, "%s\n", word6[i]);
	for(int i=0;i<wcnt[6];i++) fprintf(fptr, "%s\n", word7[i]);
	for(int i=0;i<wcnt[7];i++) fprintf(fptr, "%s\n", word8[i]);
	for(int i=0;i<wcnt[8];i++) fprintf(fptr, "%s\n", word9[i]);
	for(int i=0;i<wcnt[9];i++) fprintf(fptr, "%s\n", word10[i]);
	fclose(fptr);
}

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
char vshowtext[MAXGRID][MAXMSG] = {0};

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
	int nfill=0;
	for(int j=0;j<vidx_msg;j++)
	{
		if (vshowmsg[j] == vmsg[j]) 
			nfill++;
	}
		
	if (show_cnt > vidx_msg) show_cnt = vidx_msg;
	
	while (nfill < show_cnt)
	{
		value = rand() % vidx_msg;
		vshowmsg[value] = vmsg[value];
		
		nfill = 0;
		for(int j=0;j<vidx_msg;j++)
		{
			if (vshowmsg[j] == vmsg[j]) 
				nfill++;
		}
	}
	
	
	
	for(int idx=0;idx<CHUNKS_VERT * CHUNKS_HORZ;idx++)
	{
		char* line = get_rnd_bookline(vidx_text[idx]);
		if (line != 0)
		{
			strncpy(vtext[idx], line, vidx_text[idx]);
		}
	
		for(int i=0;i<MAXMSG;i++)
			vshowtext[idx][i] = ' ';
		
		//if (vtext[idx][0]!=' ') 
		//{
			//replace_with_rnd_word(&vtext[idx][0]);
		//}
		
		//for(int i=0;i<vidx_text[idx]-1;i++)
		//{
			//if ((vtext[idx][i]==' ') && (vtext[idx][i+1] != ' '))
			//{
				//// start of a word at i+1
				//replace_with_rnd_word(&vtext[idx][i+1]);
			//}
		//}
		
		vshowtext[idx][0] = vtext[idx][0];
		for(int i=1;i<vidx_text[idx];i++)
			if (vtext[idx][i]!=' ')
				vshowtext[idx][i] = '*';
		
		for(int i=0;i<vidx_text[idx]-1;i++)
		{
			if ((vtext[idx][i]==' ') && (vtext[idx][i+1] != ' '))
			{
				// start of a word
				vshowtext[idx][i+1] = vtext[idx][i+1];
			}
		}
		
		for(int i=0;i<show_cnt;i++)
		{
			if (vidx_text[idx] > 0)
			{
				value = rand() % (vidx_text[idx] + 0);
				vshowtext[idx][value] = vtext[idx][value];
			}
		}
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
				ret |= (ncplane_printf_yx(nc_text, i, j+4, "%c", vshowtext[i][j]) < 0);
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

read_dict();
write_dict();

read_book();
//return 0;

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
