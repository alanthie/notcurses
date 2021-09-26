#include <fcntl.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/utsname.h>
#endif
#include "internal.h"
#include "windows.h"
#include "linux.h"

// there does not exist any true standard terminal size. with that said, we
// need assume *something* for the case where we're not actually attached to
// a terminal (mainly unit tests, but also daemon environments). in preference
// to this, we use the geometries defined by (in order of precedence):
//
//  * TIOGWINSZ ioctl(2)
//  * LINES/COLUMNS environment variables
//  * lines/cols terminfo variables
//
// this function sets up ti->default_rows and ti->default_cols
static int
get_default_dimension(const char* envvar, const char* tinfovar, int def){
  const char* env = getenv(envvar);
  int num;
  if(env){
    num = atoi(env);
    if(num > 0){
      return num;
    }
  }
  num = tigetnum(tinfovar);
  if(num > 0){
    return num;
  }
  return def;
}

static void
get_default_geometry(tinfo* ti){
  ti->default_rows = get_default_dimension("LINES", "lines", 24);
  ti->default_cols = get_default_dimension("COLUMNS", "cols", 80);
  loginfo("Default geometry: %d row%s, %d column%s\n",
          ti->default_rows, ti->default_rows != 1 ? "s" : "",
          ti->default_cols, ti->default_cols != 1 ? "s" : "");
}

// we found Sixel support -- set up the API. invert80 refers to whether the
// terminal implements DECSDM correctly (enabling it with \e[80l), or inverts
// the meaning (*disabling* it with \e[80l)j
static inline void
setup_sixel_bitmaps(tinfo* ti, int fd, bool invert80){
  if(invert80){
    ti->pixel_init = sixel_init_inverted;
  }else{
    ti->pixel_init = sixel_init;
  }
  ti->pixel_scrub = sixel_scrub;
  ti->pixel_remove = NULL;
  ti->pixel_draw = sixel_draw;
  ti->pixel_refresh = sixel_refresh;
  ti->pixel_draw_late = NULL;
  ti->pixel_commit = NULL;
  ti->pixel_move = NULL;
  ti->pixel_scroll = NULL;
  ti->pixel_wipe = sixel_wipe;
  ti->pixel_clear_all = NULL;
  ti->pixel_rebuild = sixel_rebuild;
  ti->pixel_trans_auxvec = sixel_trans_auxvec;
  ti->sprixel_scale_height = 6;
  set_pixel_blitter(sixel_blit);
  ti->pixel_implementation = NCPIXEL_SIXEL;
  sprite_init(ti, fd);
}

// kitty 0.19.3 didn't have C=1, and thus needs sixel_maxy_pristine. it also
// lacked animation, and must thus redraw the complete image every time it
// changes. requires the older interface.
static inline void
setup_kitty_bitmaps(tinfo* ti, int fd, ncpixelimpl_e level){
  ti->pixel_scrub = kitty_scrub;
  ti->pixel_remove = kitty_remove;
  ti->pixel_draw = kitty_draw;
  ti->pixel_draw_late = NULL;
  ti->pixel_refresh = NULL;
  ti->pixel_commit = kitty_commit;
  ti->pixel_move = kitty_move;
  ti->pixel_scroll = NULL;
  ti->pixel_clear_all = kitty_clear_all;
  if(level == NCPIXEL_KITTY_STATIC){
    ti->pixel_wipe = kitty_wipe;
    ti->pixel_trans_auxvec = kitty_trans_auxvec;
    ti->pixel_rebuild = kitty_rebuild;
    ti->sixel_maxy_pristine = INT_MAX;
    set_pixel_blitter(kitty_blit);
    ti->pixel_implementation = NCPIXEL_KITTY_STATIC;
  }else{
    if(level == NCPIXEL_KITTY_ANIMATED){
      ti->pixel_wipe = kitty_wipe_animation;
      ti->pixel_rebuild = kitty_rebuild_animation;
      ti->sixel_maxy_pristine = 0;
      set_pixel_blitter(kitty_blit_animated);
      ti->pixel_implementation = NCPIXEL_KITTY_ANIMATED;
    }else{
      ti->pixel_wipe = kitty_wipe_selfref;
      ti->pixel_rebuild = kitty_rebuild_selfref;
      ti->sixel_maxy_pristine = 0;
      set_pixel_blitter(kitty_blit_selfref);
      ti->pixel_implementation = NCPIXEL_KITTY_SELFREF;
    }
  }
  sprite_init(ti, fd);
}

#ifdef __linux__
static inline void
setup_fbcon_bitmaps(tinfo* ti, int fd){
  ti->pixel_scrub = fbcon_scrub;
  ti->pixel_remove = NULL;
  ti->pixel_draw = NULL;
  ti->pixel_draw_late = fbcon_draw;
  ti->pixel_commit = NULL;
  ti->pixel_refresh = NULL;
  ti->pixel_move = NULL;
  ti->pixel_scroll = fbcon_scroll;
  ti->pixel_clear_all = NULL;
  ti->pixel_rebuild = fbcon_rebuild;
  ti->pixel_wipe = fbcon_wipe;
  ti->pixel_trans_auxvec = kitty_trans_auxvec;
  set_pixel_blitter(fbcon_blit);
  ti->pixel_implementation = NCPIXEL_LINUXFB;
  sprite_init(ti, fd);
}
#endif

static bool
query_rgb(void){
  bool rgb = (tigetflag("RGB") > 1 || tigetflag("Tc") > 1);
  if(!rgb){
    // RGB terminfo capability being a new thing (as of ncurses 6.1), it's not
    // commonly found in terminal entries today. COLORTERM, however, is a
    // de-facto (if imperfect/kludgy) standard way of indicating TrueColor
    // support for a terminal. The variable takes one of two case-sensitive
    // values:
    //
    //   truecolor
    //   24bit
    //
    // https://gist.github.com/XVilka/8346728#true-color-detection gives some
    // more information about the topic.
    const char* cterm = getenv("COLORTERM");
    rgb = cterm && (strcmp(cterm, "truecolor") == 0 || strcmp(cterm, "24bit") == 0);
  }
  return rgb;
}

// we couldn't get a terminal from interrogation, so let's see if the TERM
// matches any of our known terminals. this can only be as accurate as the
// TERM setting is (and as up-to-date and complete as we are).
static int
match_termname(const char* termname, queried_terminals_e* qterm){
  // https://github.com/alacritty/alacritty/pull/5274 le sigh
  if(strstr(termname, "alacritty")){
    *qterm = TERMINAL_ALACRITTY;
  }
  return 0;
}

void free_terminfo_cache(tinfo* ti){
  stop_inputlayer(ti);
  free(ti->termversion);
  free(ti->esctable);
#ifdef __linux__
  if(ti->linux_fb_fd >= 0){
    close(ti->linux_fb_fd);
  }
  free(ti->linux_fb_dev);
  if(ti->linux_fbuffer != MAP_FAILED){
    munmap(ti->linux_fbuffer, ti->linux_fb_len);
  }
#endif
  free(ti->tpreserved);
}

// compare one terminal version against another. numerics, separated by
// periods, and comparison ends otherwise (so "20.0 alpha" doesn't compare
// as greater than "20.0", mainly). returns -1 if v1 < v2 (or v1 is NULL),
// 0 if v1 == v2, or 1 if v1 > v2.
static int
compare_versions(const char* restrict v1, const char* restrict v2){
  if(v1 == NULL){
    return -1;
  }
  char* v1e;
  char* v2e;
  while(*v1 && *v2){
    long v1v = strtol(v1, &v1e, 10);
    long v2v = strtol(v2, &v2e, 10);
    if(v1e == v1 && v2e == v2){ // both are done
      return 0;
    }else if(v1e == v1){ // first is done
      return -1;
    }else if(v2e == v2){ // second is done
      return 1;
    }
    if(v1v > v2v){
      return 1;
    }else if(v2v > v1v){
      return -1;
    }
    if(*v1e != '.' && *v2e != '.'){
      break;
    }else if(*v1e != '.' || *v2e != '.'){
      if(*v1e == '.'){
        return 1;
      }else{
        return -1;
      }
    }
    v1 = v1e + 1;
    v2 = v2e + 1;
  }
  // can only get out here if at least one was not a period
  if(*v1e == '.'){
    return 1;
  }
  if(*v2e == '.'){
    return -1;
  }
  return 0;
}

static inline int
terminfostr(char** gseq, const char* name){
  *gseq = tigetstr(name);
  if(*gseq == NULL || *gseq == (char*)-1){
    *gseq = NULL;
    return -1;
  }
  // terminfo syntax allows a number N of milliseconds worth of pause to be
  // specified using $<N> syntax. this is then honored by tputs(). but we don't
  // use tputs(), instead preferring the much faster stdio+tiparm() (at the
  // expense of terminals which do require these delays). to avoid dumping
  // "$<N>" sequences all over stdio, we chop them out. real text can follow
  // them, so we continue on, copying back once out of the delay.
  char* wnext = NULL; // NULL until we hit a delay, then place to write
  bool indelay = false; // true iff we're in a delay section
  // we consider it a delay as soon as we see '$', and the delay ends at '>'
  for(char* cur = *gseq ; *cur ; ++cur){
    if(!indelay){
      // if we're not in a delay section, make sure we're not starting one,
      // and otherwise copy the current character back (if necessary).
      if(*cur == '$'){
        wnext = cur;
        indelay = true;
      }else{
        if(wnext){
          *wnext++ = *cur;
        }
      }
    }else{
      // we are in a delay section. make sure we're not ending one.
      if(*cur == '>'){
        indelay = false;
      }
    }
  }
  if(wnext){
    *wnext = '\0';
  }
  return 0;
}

static inline int
init_terminfo_esc(tinfo* ti, const char* name, escape_e idx,
                  size_t* tablelen, size_t* tableused){
  char* tstr;
  if(ti->escindices[idx]){
    return 0;
  }
  if(terminfostr(&tstr, name) == 0){
    if(grow_esc_table(ti, tstr, idx, tablelen, tableused)){
      return -1;
    }
  }else{
    ti->escindices[idx] = 0;
  }
  return 0;
}

// Tertiary Device Attributes, necessary to identify VTE.
// https://vt100.net/docs/vt510-rm/DA3.html
// Replies with DCS ! | ... ST
#define TRIDEVATTR "\x1b[=c"

// Primary Device Attributes, necessary to elicit a response from terminals
// which don't respond to other queries. All known terminals respond to DA1.
// https://vt100.net/docs/vt510-rm/DA1.html
// Device Attributes; replies with (depending on decTerminalID resource):
//   ⇒  CSI ? 1 ; 2 c  ("VT100 with Advanced Video Option")
//   ⇒  CSI ? 1 ; 0 c  ("VT101 with No Options")
//   ⇒  CSI ? 4 ; 6 c  ("VT132 with Advanced Video and Graphics")
//   ⇒  CSI ? 6 c  ("VT102")
//   ⇒  CSI ? 7 c  ("VT131")
//   ⇒  CSI ? 1 2 ; Ps c  ("VT125")
//   ⇒  CSI ? 6 2 ; Ps c  ("VT220")
//   ⇒  CSI ? 6 3 ; Ps c  ("VT320")
//   ⇒  CSI ? 6 4 ; Ps c  ("VT420")
#define PRIDEVATTR "\x1b[c"

// XTVERSION. Replies with DCS > | ... ST
#define XTVERSION "\x1b[>0q"

// XTGETTCAP['TN'] (Terminal Name)
#define XTGETTCAPTN "\x1bP+q544e\x1b\\"

// Secondary Device Attributes, necessary to get Alacritty's version. Since
// this doesn't uniquely identify a terminal, we ask it last, so that if any
// queries which *do* unambiguously identify a terminal have succeeded, this
// needn't be paid attention to.
// https://vt100.net/docs/vtk510-rm/DA2.html
// (note that tmux uses 84 rather than common 60/61)
// Replies with CSI > \d \d ; Pv ; [01] c
#define SECDEVATTR "\x1b[>c"

// query for kitty graphics. if they are supported, we'll get a response to
// this using the kitty response syntax. otherwise, we'll get nothing. we
// send this with the other identification queries, since APCs tend not to
// be consumed by certain terminal emulators (looking at you, Linux console)
// which can be identified directly, sans queries.
#define KITTYQUERY "\x1b_Gi=1,a=q;\x1b\\"

// request kitty keyboard protocol through level 1, and push current.
// see https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
#define KBDSUPPORT "\x1b[>u\x1b[=1u"

// the kitty keyboard protocol allows unambiguous, complete identification of
// input events. this queries for the level of support.
#define KBDQUERY "\x1b[?u"

// these queries (terminated with a Primary Device Attributes, to which
// all known terminals reply) hopefully can uniquely and unquestionably
// identify the terminal to which we are talking.
#define IDQUERIES KITTYQUERY \
                  KBDSUPPORT \
                  KBDQUERY \
                  TRIDEVATTR \
                  XTVERSION \
                  XTGETTCAPTN \
                  SECDEVATTR

// query background, replies in X color https://www.x.org/releases/X11R7.7/doc/man/man7/X.7.xhtml#heading11
#define CSI_BGQ "\x1b]11;?\e\\"

// FIXME ought be using the u7 terminfo string here, if it exists. the great
// thing is, if we get a response to this, we know we can use it for u7!
// we send this first because terminals which don't consume the entire escape
// sequences following will bleed the excess into the terminal, and we want
// to blow any such output away (or at least return to the cell where such
// output started).
#define DSRCPR "\x1b[6n"

// check for Synchronized Update Mode support. the p is necessary, but at
// least Konsole and Terminal.app fail to consume it =[.
#define SUMQUERY "\x1b[?2026$p"

// XTSMGRAPHICS query for the number of color registers.
#define CREGSXTSM "\x1b[?2;1;0S"

// XTSMGRAPHICS query for the maximum supported geometry.
#define GEOMXTSM "\x1b[?1;1;0S"

// non-standard CSI for total pixel geometry
#define GEOMPIXEL "\x1b[14t"

// request the cell geometry of the textual area
#define GEOMCELL "\x1b[18t"

#define DIRECTIVES CSI_BGQ \
                   SUMQUERY \
                   "\x1b[?1;3;256S" /* try to set 256 cregs */ \
                   CREGSXTSM \
                   GEOMXTSM \
                   GEOMPIXEL \
                   GEOMCELL \
                   PRIDEVATTR

// enter the alternate screen (smcup). we could technically get this from
// terminfo, but everyone who supports it supports it the same way, and we
// need to send it before our other directives if we're going to use it.
#define SMCUP "\x1b[?1049h"

// we send an XTSMGRAPHICS to set up 256 color registers (the most we can
// currently take advantage of; we need at least 64 to use sixel at all).
// maybe that works, maybe it doesn't. then query both color registers
// and geometry. send XTGETTCAP for terminal name. if 'minimal' is set, don't
// send any identification queries (we've already identified the terminal).
static int
send_initial_queries(int fd, bool minimal, bool noaltscreen){
  const char *queries;
  if(noaltscreen){
    if(minimal){
      queries = DSRCPR DIRECTIVES;
    }else{
      queries = DSRCPR IDQUERIES DIRECTIVES;
    }
  }else{
    if(minimal){
      queries = SMCUP DSRCPR DIRECTIVES;
    }else{
      queries = SMCUP DSRCPR IDQUERIES DIRECTIVES;
    }
  }
  size_t len = strlen(queries);
  loginfo("sending %lluB queries\n", (unsigned long long)len);
  if(blocking_write(fd, queries, len)){
    return -1;
  }
  return 0;
}

// if we get a response to the standard cursor locator escape, we know this
// terminal supports it, hah.
static int
add_u7_escape(tinfo* ti, size_t* tablelen, size_t* tableused){
  const char* u7 = get_escape(ti, ESCAPE_U7);
  if(u7){
    return 0; // already present
  }
  if(grow_esc_table(ti, DSRCPR, ESCAPE_U7, tablelen, tableused)){
    return -1;
  }
  return 0;
}

static int
add_smulx_escapes(tinfo* ti, size_t* tablelen, size_t* tableused){
  if(get_escape(ti, ESCAPE_SMULX)){
    return 0;
  }
  if(grow_esc_table(ti, "\x1b[4:3m", ESCAPE_SMULX, tablelen, tableused) ||
     grow_esc_table(ti, "\x1b[4:0m", ESCAPE_SMULNOX, tablelen, tableused)){
    return -1;
  }
  return 0;
}

static inline void
kill_escape(tinfo* ti, escape_e e){
  ti->escindices[e] = 0;
}

static void
kill_appsync_escapes(tinfo* ti){
  kill_escape(ti, ESCAPE_BSUM);
  kill_escape(ti, ESCAPE_ESUM);
}

static int
add_appsync_escapes_sm(tinfo* ti, size_t* tablelen, size_t* tableused){
  if(get_escape(ti, ESCAPE_BSUM)){
    return 0;
  }
  if(grow_esc_table(ti, "\x1b[?2026h", ESCAPE_BSUM, tablelen, tableused) ||
     grow_esc_table(ti, "\x1b[?2026l", ESCAPE_ESUM, tablelen, tableused)){
    return -1;
  }
  return 0;
}

static int
add_appsync_escapes_dcs(tinfo* ti, size_t* tablelen, size_t* tableused){
  if(get_escape(ti, ESCAPE_BSUM)){
    return 0;
  }
  if(grow_esc_table(ti, "\x1bP=1s\x1b\\", ESCAPE_BSUM, tablelen, tableused) ||
     grow_esc_table(ti, "\x1bP=2s\x1b\\", ESCAPE_ESUM, tablelen, tableused)){
    return -1;
  }
  return 0;
}

static int
add_pushcolors_escapes(tinfo* ti, size_t* tablelen, size_t* tableused){
  if(get_escape(ti, ESCAPE_SAVECOLORS)){
    return 0;
  }
  if(grow_esc_table(ti, "\x1b[#P", ESCAPE_SAVECOLORS, tablelen, tableused) ||
     grow_esc_table(ti, "\x1b[#Q", ESCAPE_RESTORECOLORS, tablelen, tableused)){
    return -1;
  }
  return 0;
}

// qui si convien lasciare ogne sospetto; ogne viltà convien che qui sia morta.
// in a more perfect world, this function would not exist, but this is a
// regrettably imperfect world, and thus all manner of things are not maintained
// in terminfo, and old terminfos abound, and users don't understand terminfo,
// so we override and/or supply various properties based on terminal
// identification performed earlier. we still get most things from terminfo,
// though, so it's something of a worst-of-all-worlds deal where TERM still
// needs be correct, even though we identify the terminal. le sigh.
static int
apply_term_heuristics(tinfo* ti, const char* termname, queried_terminals_e qterm,
                      size_t* tablelen, size_t* tableused, bool* invertsixel,
                      unsigned nonewfonts){
  if(!termname){
    // setupterm interprets a missing/empty TERM variable as the special value “unknown”.
    termname = ti->termname ? ti->termname : "unknown";
  }
  if(qterm == TERMINAL_UNKNOWN){
    match_termname(termname, &qterm);
    // we pick up alacritty's version via a weird hack involving Secondary
    // Device Attributes. if we're not alacritty, don't trust that version.
    if(qterm != TERMINAL_ALACRITTY){
      free(ti->termversion);
      ti->termversion = NULL;
    }
  }
  // st had neither caps.sextants nor caps.quadrants last i checked (0.8.4)
  ti->caps.braille = true; // most everyone has working caps.braille, even from fonts
  ti->caps.halfblocks = true; // most everyone has working halfblocks
  // FIXME clean this shit up; use a table for chrissakes
  if(qterm == TERMINAL_KITTY){ // kitty (https://sw.kovidgoyal.net/kitty/)
    termname = "Kitty";
    // see https://sw.kovidgoyal.net/kitty/protocol-extensions.html
    ti->bg_collides_default |= 0x1000000;
    ti->caps.sextants = true; // work since bugfix in 0.19.3
    ti->caps.quadrants = true;
    ti->caps.rgb = true;
    if(add_smulx_escapes(ti, tablelen, tableused)){
      return -1;
    }
    /*if(compare_versions(ti->termversion, "0.22.1") >= 0){
      setup_kitty_bitmaps(ti, ti->ttyfd, NCPIXEL_KITTY_SELFREF);
    }else*/ if(compare_versions(ti->termversion, "0.20.0") >= 0){
      setup_kitty_bitmaps(ti, ti->ttyfd, NCPIXEL_KITTY_ANIMATED);
    }else{
      setup_kitty_bitmaps(ti, ti->ttyfd, NCPIXEL_KITTY_STATIC);
    }
    if(add_pushcolors_escapes(ti, tablelen, tableused)){
      return -1;
    }
    // kitty SUM doesn't want long sequences, which is exactly where we use
    // it. remove support (we pick it up from queries).
    kill_appsync_escapes(ti);
  }else if(qterm == TERMINAL_ALACRITTY){
    termname = "Alacritty";
    ti->caps.quadrants = true;
    // ti->caps.sextants = true; // alacritty https://github.com/alacritty/alacritty/issues/4409
    ti->caps.rgb = true;
    // Alacritty implements DCS ASU, but no detection for it
    if(add_appsync_escapes_dcs(ti, tablelen, tableused)){
      return -1;
    }
    if(compare_versions(ti->termversion, "0.15.1") < 0){
      *invertsixel = true;
    }
  }else if(qterm == TERMINAL_VTE){
    termname = "VTE";
    ti->caps.quadrants = true;
    ti->caps.sextants = true; // VTE has long enjoyed good sextant support
    if(add_smulx_escapes(ti, tablelen, tableused)){
      return -1;
    }
    // VTE understands DSC ACU, but doesn't do anything with it; don't use it
  }else if(qterm == TERMINAL_FOOT){
    termname = "foot";
    ti->caps.sextants = true;
    ti->caps.quadrants = true;
    ti->caps.rgb = true;
    if(compare_versions(ti->termversion, "1.8.2") < 0){
      *invertsixel = true;
    }
  }else if(qterm == TERMINAL_TMUX){
    termname = "tmux";
    // FIXME what, oh what to do with tmux?
  }else if(qterm == TERMINAL_MLTERM){
    termname = "MLterm";
    ti->caps.quadrants = true; // good caps.quadrants, no caps.sextants as of 3.9.0
  }else if(qterm == TERMINAL_WEZTERM){
    termname = "WezTerm";
    ti->caps.rgb = true;
    ti->caps.quadrants = true;
    if(ti->termversion && strcmp(ti->termversion, "20210610") >= 0){
      ti->caps.sextants = true; // good caps.sextants as of 2021-06-10
      if(add_smulx_escapes(ti, tablelen, tableused)){
        return -1;
      }
    }
  }else if(qterm == TERMINAL_XTERM){
    termname = "XTerm";
    // xterm 357 added color palette escapes XT{PUSH,POP,REPORT}COLORS
    if(compare_versions(ti->termversion, "357") >= 0){
      if(add_pushcolors_escapes(ti, tablelen, tableused)){
        return -1;
      }
      if(compare_versions(ti->termversion, "359") < 0){
        *invertsixel = true;
      }
    }
  }else if(qterm == TERMINAL_MINTTY){
    termname = "MinTTY";
    if(add_smulx_escapes(ti, tablelen, tableused)){
      return -1;
    }
    if(compare_versions(ti->termversion, "3.5.2") < 0){
      *invertsixel = true;
    }
    ti->bce = true;
  }else if(qterm == TERMINAL_CONTOUR){
    termname = "Contour";
    ti->caps.quadrants = true;
    ti->caps.rgb = true;
  }else if(qterm == TERMINAL_ITERM){
    termname = "iTerm2";
    // iTerm implements DCS ASU, but has no detection for it
    if(add_appsync_escapes_dcs(ti, tablelen, tableused)){
      return -1;
    }
    ti->caps.quadrants = true;
    ti->caps.rgb = true;
  }else if(qterm == TERMINAL_APPLE){
    termname = "Terminal.app";
    // no quadrants, no sextants, no rgb, but it does have braille
#ifdef __linux__
  }else if(qterm == TERMINAL_LINUX){
    struct utsname un;
    if(uname(&un) == 0){
      ti->termversion = strdup(un.release);
    }
    if(is_linux_framebuffer(ti)){
      termname = "Linux framebuffer";
      setup_fbcon_bitmaps(ti, ti->linux_fb_fd);
    }else{
      termname = "Linux console";
    }
    ti->caps.halfblocks = false;
    ti->caps.braille = false; // no caps.braille, no caps.sextants in linux console
    if(ti->ttyfd >= 0){
      reprogram_console_font(ti, nonewfonts, &ti->caps.halfblocks,
                             &ti->caps.quadrants);
    }
    // assume no useful unicode drawing unless we're positively sure
#else
(void)nonewfonts;
#endif
  }else if(qterm == TERMINAL_TERMINOLOGY){
    termname = "Terminology";
    ti->caps.rgb = false; // as of at least 1.9.0
    ti->caps.quadrants = true;
  }
  // run a wcwidth(⣿) to guarantee libc Unicode 3 support, independent of term
  if(wcwidth(L'⣿') < 0){
    ti->caps.braille = false;
  }
  // run a wcwidth(🬸) to guarantee libc Unicode 13 support, independent of term
  if(wcwidth(L'🬸') < 0){
    ti->caps.sextants = false;
  }
  ti->termname = termname;
  return 0;
}

// some terminals cannot combine certain styles with colors, as expressed in
// the "ncv" terminfo capability (using ncurses-style constants). don't
// advertise support for the style in that case. otherwise, if the style is
// supported, OR it into supported_styles (using Notcurses-style constants).
static void
build_supported_styles(tinfo* ti){
  const struct style {
    unsigned s;        // NCSTYLE_* value
    int esc;           // ESCAPE_* value for enable
    const char* tinfo; // terminfo capability for conditional permit
    unsigned ncvbit;   // bit in "ncv" mask for unconditional deny
  } styles[] = {
    { NCSTYLE_BOLD, ESCAPE_BOLD, "bold", A_BOLD },
    { NCSTYLE_UNDERLINE, ESCAPE_SMUL, "smul", A_UNDERLINE },
    { NCSTYLE_ITALIC, ESCAPE_SITM, "sitm", A_ITALIC },
    { NCSTYLE_STRUCK, ESCAPE_SMXX, "smxx", 0 },
    { NCSTYLE_UNDERCURL, ESCAPE_SMULX, "Smulx", 0 },
    { 0, 0, NULL, 0 }
  };
  int nocolor_stylemask = tigetnum("ncv");
  for(typeof(*styles)* s = styles ; s->s ; ++s){
    if(get_escape(ti, s->esc)){
      if(nocolor_stylemask > 0){
        if(nocolor_stylemask & s->ncvbit){
          ti->escindices[s->esc] = 0;
          continue;
        }
      }
      ti->supported_styles |= s->s;
    }
  }
}

#ifdef __APPLE__
// Terminal.App is a wretched piece of shit that can't handle even the most
// basic of queries, instead bleeding them through to stdout like a great
// wounded hippopotamus. it does export "TERM_PROGRAM=Apple_Terminal", becuase
// it is a committee on sewage and drainage where all the members have
// tourette's. on mac os, if TERM_PROGRAM=Apple_Terminal, accept this hideous
// existence, circumvent all queries, and may god have mercy on our souls.
// of course that means if a terminal launched from Terminal.App doesn't clear
// or reset this environment variable, they're cursed to live as Terminal.App.
// i'm likewise unsure what we're supposed to do should you ssh anywhere =[.
static queried_terminals_e
macos_early_matches(void){
  const char* tp = getenv("TERM_PROGRAM");
  if(tp == NULL){
    return TERMINAL_UNKNOWN;
  }
  if(strcmp(tp, "Apple_Terminal")){
    return TERMINAL_UNKNOWN;
  }
  return TERMINAL_APPLE;
}
#endif

// if |termtype| is not NULL, it is used to look up the terminfo database entry
// via setupterm(). the value of the TERM environment variable is otherwise
// (implicitly) used. some details are not exposed via terminfo, and we must
// make heuristic decisions based on the detected terminal type, yuck :/.
// the first thing we do is fire off any queries we have (XTSMGRAPHICS, etc.)
// with a trailing Device Attributes. all known terminals will reply to a
// Device Attributes, allowing us to get a negative response if our queries
// aren't supported by the terminal. we fire it off early because we have a
// full round trip before getting the reply, which is likely to pace init.
int interrogate_terminfo(tinfo* ti, const char* termtype, FILE* out, unsigned utf8,
                         unsigned noaltscreen, unsigned nocbreak, unsigned nonewfonts,
                         int* cursor_y, int* cursor_x, ncsharedstats* stats,
                         int lmargin, int tmargin, unsigned draininput){
  int foolcursor_x, foolcursor_y;
  if(!cursor_x){
    cursor_x = &foolcursor_x;
  }
  if(!cursor_y){
    cursor_y = &foolcursor_y;
  }
  *cursor_x = *cursor_y = -1;
  memset(ti, 0, sizeof(*ti));
  ti->qterm = TERMINAL_UNKNOWN;
  // we don't need a controlling tty for everything we do; allow a failure here
  ti->ttyfd = get_tty_fd(out);
  ti->gpmfd = -1;
  size_t tablelen = 0;
  size_t tableused = 0;
  const char* tname = NULL;
#ifdef __APPLE__
  ti->qterm = macos_early_matches();
#elif defined(__MINGW64__)
  if(termtype){
    logwarn("termtype (%s) ignored on windows\n", termtype);
  }
  if(prepare_windows_terminal(ti, &tablelen, &tableused) == 0){
    ti->qterm = TERMINAL_MSTERMINAL;
    if(cursor_y && cursor_x){
      locate_cursor(ti, cursor_y, cursor_x);
    }
  }
#elif defined(__linux__)
  ti->linux_fb_fd = -1;
  ti->linux_fbuffer = MAP_FAILED;
  // we might or might not program quadrants into the console font
  if(is_linux_console(ti->ttyfd)){
    ti->qterm = TERMINAL_LINUX;
  }
#endif
  if(ti->ttyfd >= 0){
    if((ti->tpreserved = calloc(1, sizeof(*ti->tpreserved))) == NULL){
      return -1;
    }
    if(tcgetattr(ti->ttyfd, ti->tpreserved)){
      logpanic("Couldn't preserve terminal state for %d (%s)\n", ti->ttyfd, strerror(errno));
      free(ti->tpreserved);
      return -1;
    }
    // enter cbreak mode regardless of user preference until we've performed
    // terminal interrogation. at that point, we might restore original mode.
    if(cbreak_mode(ti)){
      free(ti->tpreserved);
      return -1;
    }
    // if we already know our terminal (e.g. on the linux console), there's no
    // need to send the identification queries. the controls are sufficient.
    bool minimal = (ti->qterm != TERMINAL_UNKNOWN);
    if(send_initial_queries(ti->ttyfd, minimal, noaltscreen)){
      goto err;
    }
  }
#ifndef __MINGW64__
  // windows doesn't really have a concept of terminfo. you might ssh into other
  // machines, but they'll use the terminfo installed thereon (putty, etc.).
  int termerr;
  if(setupterm(termtype, ti->ttyfd, &termerr)){
    logpanic("Terminfo error %d for [%s] (see terminfo(3ncurses))\n",
             termerr, termtype ? termtype : "");
    goto err;
  }
  tname = termname(); // longname() is also available
#endif
  int linesigs_enabled = 1;
  if(ti->tpreserved){
    if(!(ti->tpreserved->c_lflag & ISIG)){
      linesigs_enabled = 0;
    }
  }
  if(init_inputlayer(ti, stdin, lmargin, tmargin, stats, draininput,
                     linesigs_enabled)){
    goto err;
  }
  ti->sprixel_scale_height = 1;
  get_default_geometry(ti);
  ti->caps.utf8 = utf8;
  // allow the "rgb" boolean terminfo capability, a COLORTERM environment
  // variable of either "truecolor" or "24bit", or unconditionally enable it
  // for several terminals known to always support 8bpc rgb setaf/setab.
  if(ti->caps.colors == 0){
    int colors = tigetnum("colors");
    if(colors <= 0){
      ti->caps.colors = 1;
    }else{
      ti->caps.colors = colors;
    }
    ti->caps.rgb = query_rgb(); // independent of colors
  }
  const struct strtdesc {
    escape_e esc;
    const char* tinfo;
  } strtdescs[] = {
    { ESCAPE_CUP, "cup", },
    { ESCAPE_HPA, "hpa", },
    { ESCAPE_VPA, "vpa", },
    // Not all terminals support setting the fore/background independently
    { ESCAPE_SETAF, "setaf", },
    { ESCAPE_SETAB, "setab", },
    { ESCAPE_OP, "op", },
    { ESCAPE_CNORM, "cnorm", },
    { ESCAPE_CIVIS, "civis", },
    { ESCAPE_SGR0, "sgr0", },
    { ESCAPE_SITM, "sitm", },
    { ESCAPE_RITM, "ritm", },
    { ESCAPE_BOLD, "bold", },
    { ESCAPE_CUD, "cud", },
    { ESCAPE_CUU, "cuu", },
    { ESCAPE_CUF, "cuf", },
    { ESCAPE_CUB, "cub", },
    { ESCAPE_U7, "u7", },
    { ESCAPE_SMKX, "smkx", },
    { ESCAPE_SMXX, "smxx", },
    { ESCAPE_EL, "el", },
    { ESCAPE_RMXX, "rmxx", },
    { ESCAPE_SMUL, "smul", },
    { ESCAPE_RMUL, "rmul", },
    { ESCAPE_SC, "sc", },
    { ESCAPE_RC, "rc", },
    { ESCAPE_IND, "ind", },
    { ESCAPE_INDN, "indn", },
    { ESCAPE_CLEAR, "clear", },
    { ESCAPE_OC, "oc", },
    { ESCAPE_RMKX, "rmkx", },
    { ESCAPE_INITC, "initc", },
    { ESCAPE_MAX, NULL, },
  };
  for(typeof(*strtdescs)* strtdesc = strtdescs ; strtdesc->esc < ESCAPE_MAX ; ++strtdesc){
    if(init_terminfo_esc(ti, strtdesc->tinfo, strtdesc->esc, &tablelen, &tableused)){
      goto err;
    }
  }
  // verify that the terminal provides cursor addressing (absolute movement)
  if(ti->escindices[ESCAPE_CUP] == 0){
    logpanic("Required terminfo capability 'cup' not defined\n");
    goto err;
  }
  if(ti->ttyfd >= 0){
    // if the keypad neen't be explicitly enabled, smkx is not present
    const char* smkx = get_escape(ti, ESCAPE_SMKX);
    if(smkx){
      if(tty_emit(tiparm(smkx), ti->ttyfd) < 0){
        logpanic("Error enabling keypad transmit mode\n");
        goto err;
      }
    }
  }
  if(tigetflag("bce") > 0){
    ti->bce = true;
  }
  if(ti->caps.colors > 1){
    const char* initc = get_escape(ti, ESCAPE_INITC);
    if(initc){
      ti->caps.can_change_colors = tigetflag("ccc") == 1;
    }
  }else{ // disable initc if there's no color support
    ti->escindices[ESCAPE_INITC] = 0;
  }
  // neither of these is supported on e.g. the "linux" virtual console.
  if(!noaltscreen){
    if(init_terminfo_esc(ti, "smcup", ESCAPE_SMCUP, &tablelen, &tableused) ||
       init_terminfo_esc(ti, "rmcup", ESCAPE_RMCUP, &tablelen, &tableused)){
      goto err;
    }
    const char* smcup = get_escape(ti, ESCAPE_SMCUP);
    if(smcup){
      ti->in_alt_screen = 1;
      // if we're not using the standard smcup, our initial hardcoded use of it
      // presumably had no effect; warn the user.
      if(strcmp(smcup, SMCUP)){
        logwarn("warning: non-standard smcup!\n");
      }
    }
  }else{
    ti->escindices[ESCAPE_SMCUP] = 0;
    ti->escindices[ESCAPE_RMCUP] = 0;
  }
  if(get_escape(ti, ESCAPE_CIVIS) == NULL){
    char* chts;
    if(terminfostr(&chts, "chts") == 0){
      if(grow_esc_table(ti, chts, ESCAPE_CIVIS, &tablelen, &tableused)){
        goto err;
      }
    }
  }
  if(get_escape(ti, ESCAPE_BOLD)){
    if(grow_esc_table(ti, "\e[22m", ESCAPE_NOBOLD, &tablelen, &tableused)){
      goto err;
    }
  }
  // if op is defined as ansi 39 + ansi 49, make the split definitions
  // available. this ought be asserted by extension capability "ax", but
  // no terminal i've found seems to do so. =[
  const char* op = get_escape(ti, ESCAPE_OP);
  if(op && strcmp(op, "\x1b[39;49m") == 0){
    if(grow_esc_table(ti, "\x1b[39m", ESCAPE_FGOP, &tablelen, &tableused) ||
       grow_esc_table(ti, "\x1b[49m", ESCAPE_BGOP, &tablelen, &tableused)){
      goto err;
    }
  }
  unsigned kitty_graphics = 0;
  if(ti->ttyfd >= 0){
    struct initial_responses* iresp;
    if((iresp = inputlayer_get_responses(ti->ictx)) == NULL){
      goto err;
    }
    if(iresp->appsync_supported){
      if(add_appsync_escapes_sm(ti, &tablelen, &tableused)){
        free(iresp->version);
        free(iresp);
        goto err;
      }
    }
    if(iresp->qterm != TERMINAL_UNKNOWN){
      ti->qterm = iresp->qterm;
    }
    *cursor_y = iresp->cursory;
    *cursor_x = iresp->cursorx;
    ti->termversion = iresp->version;
    if(iresp->dimy && iresp->dimx){
      // FIXME probably oughtn't be setting the defaults, as this is just some
      // random transient measurement?
      ti->default_rows = iresp->dimy;
      ti->default_cols = iresp->dimx;
    }
    if(iresp->pixy && iresp->pixx){
      ti->pixy = iresp->pixy;
      ti->pixx = iresp->pixx;
    }
    if(ti->default_rows && ti->default_cols){
      ti->cellpixy = ti->pixy / ti->default_rows;
      ti->cellpixx = ti->pixx / ti->default_cols;
    }
    ti->bg_collides_default = iresp->bg;
    // kitty trumps sixel, when both are available
    if((kitty_graphics = iresp->kitty_graphics) == 0){
      ti->color_registers = iresp->color_registers;
      ti->sixel_maxy = iresp->sixely;
      ti->sixel_maxx = iresp->sixelx;
    }
    free(iresp);
  }
  if(nocbreak){
    if(ti->ttyfd >= 0){
      // FIXME do this in input later, upon signaling completion?
      if(tcsetattr(ti->ttyfd, TCSANOW, ti->tpreserved)){
        goto err;
      }
    }
  }
  if(*cursor_x >= 0 && *cursor_y >= 0){
    if(add_u7_escape(ti, &tablelen, &tableused)){
      goto err;
    }
  }
  bool invertsixel = false;
  if(apply_term_heuristics(ti, tname, ti->qterm, &tablelen, &tableused,
                           &invertsixel, nonewfonts)){
    goto err;
  }
  build_supported_styles(ti);
  if(ti->pixel_draw == NULL && ti->pixel_draw_late == NULL){
    if(kitty_graphics){
      setup_kitty_bitmaps(ti, ti->ttyfd, NCPIXEL_KITTY_ANIMATED);
    }
    // our current sixel quantization algorithm requires at least 64 color
    // registers. we make use of no more than 256. this needs to happen
    // after heuristics, since the choice of sixel_init() depends on it.
    if(ti->color_registers >= 64){
      setup_sixel_bitmaps(ti, ti->ttyfd, invertsixel);
    }
  }
  return 0;

err:
  // FIXME need to leave alternate screen if we entered it
  if(ti->tpreserved){
    (void)tcsetattr(ti->ttyfd, TCSANOW, ti->tpreserved);
    free(ti->tpreserved);
    ti->tpreserved = NULL;
  }
  stop_inputlayer(ti);
  free(ti->esctable);
  free(ti->termversion);
  del_curterm(cur_term);
  close(ti->ttyfd);
  ti->ttyfd = -1;
  return -1;
}

char* termdesc_longterm(const tinfo* ti){
  size_t tlen = strlen(ti->termname) + 1;
  size_t slen = tlen;
  if(ti->termversion){
    slen += strlen(ti->termversion) + 1;
  }
  char* ret = malloc(slen);
  if(ret){
    memcpy(ret, ti->termname, tlen);
    if(ti->termversion){
      ret[tlen - 1] = ' ';
      strcpy(ret + tlen, ti->termversion);
    }
  }
  return ret;
}

// send a u7 request, and wait until we have a cursor report. if input's ttyfd
// is valid, we can just camp there. otherwise, we need dance with potential
// user input looking at infd.
int locate_cursor(tinfo* ti, int* cursor_y, int* cursor_x){
#ifdef __MINGW64__
  if(ti->qterm == TERMINAL_MSTERMINAL){
    if(ti->outhandle){
      CONSOLE_SCREEN_BUFFER_INFO conbuf;
      if(!GetConsoleScreenBufferInfo(ti->outhandle, &conbuf)){
        logerror("couldn't get cursor info\n");
        return -1;
      }
      *cursor_y = conbuf.dwCursorPosition.Y;
      *cursor_x = conbuf.dwCursorPosition.X;
      loginfo("got a report from y=%d x=%d\n", *cursor_y, *cursor_x);
      return 0;
    }
  }
#endif
  const char* u7 = get_escape(ti, ESCAPE_U7);
  if(u7 == NULL){
    logwarn("No support in terminfo\n");
    return -1;
  }
  if(ti->ttyfd < 0){
    logwarn("No valid path for cursor report\n");
    return -1;
  }
  int fd = ti->ttyfd;
  if(tty_emit(u7, fd)){
    return -1;
  }
  get_cursor_location(ti->ictx, cursor_y, cursor_x);
  loginfo("got a report from %d %d/%d\n", fd, *cursor_y, *cursor_x);
  return 0;
}

int cbreak_mode(tinfo* ti){
#ifndef __MINGW64__
  int ttyfd = ti->ttyfd;
  if(ttyfd < 0){
    return 0;
  }
  // assume it's not a true terminal (e.g. we might be redirected to a file)
  struct termios modtermios;
  memcpy(&modtermios, ti->tpreserved, sizeof(modtermios));
  // see termios(3). disabling ECHO and ICANON means input will not be echoed
  // to the screen, input is made available without enter-based buffering, and
  // line editing is disabled. since we have not gone into raw mode, ctrl+c
  // etc. still have their typical effects. ICRNL maps return to 13 (Ctrl+M)
  // instead of 10 (Ctrl+J).
  modtermios.c_lflag &= (~ECHO & ~ICANON);
  modtermios.c_iflag &= ~ICRNL;
  if(tcsetattr(ttyfd, TCSANOW, &modtermios)){
    logerror("Error disabling echo / canonical on %d (%s)\n", ttyfd, strerror(errno));
    return -1;
  }
#else
  // we don't yet have a way to take Cygwin/MSYS2 out of canonical mode. we'll
  // hit this stanza in MSYS2; allow the GetConsoleMode() to fail for now. this
  // means we'll need enter pressed after the query response, obviously an
  // unacceptable state of affairs...FIXME
  DWORD mode;
  if(!GetConsoleMode(ti->inhandle, &mode)){
    logerror("error acquiring input mode\n");
    return 0; // FIXME is it safe?
  }
  mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
  if(!SetConsoleMode(ti->inhandle, mode)){
    logerror("error setting input mode\n");
    return -1;
  }
#endif
  return 0;
}
