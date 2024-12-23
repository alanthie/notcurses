#include <memory>
#include <unistd.h>
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <notcurses/notcurses.h>
#include <sstream>
#include <algorithm>
#include <thread>
#include <mutex>

static void tabcbfn(struct nctab* t, struct ncplane* p, void* curry);



struct NCKey
{
	static constexpr char32_t Invalid   = NCKEY_INVALID;
	static constexpr char32_t Resize    = NCKEY_RESIZE;
	static constexpr char32_t Up        = NCKEY_UP;
	static constexpr char32_t Right     = NCKEY_RIGHT;
	static constexpr char32_t Down      = NCKEY_DOWN;
	static constexpr char32_t Left      = NCKEY_LEFT;
	static constexpr char32_t Ins       = NCKEY_INS;
	static constexpr char32_t Del       = NCKEY_DEL;
	static constexpr char32_t Backspace = NCKEY_BACKSPACE;
	static constexpr char32_t PgDown    = NCKEY_PGDOWN;
	static constexpr char32_t PgUp      = NCKEY_PGUP;
	static constexpr char32_t Home      = NCKEY_HOME;
	static constexpr char32_t End       = NCKEY_END;
	static constexpr char32_t F00       = NCKEY_F00;
	static constexpr char32_t F01       = NCKEY_F01;
	static constexpr char32_t F02       = NCKEY_F02;
	static constexpr char32_t F03       = NCKEY_F03;
	static constexpr char32_t F04       = NCKEY_F04;
	static constexpr char32_t F05       = NCKEY_F05;
	static constexpr char32_t F06       = NCKEY_F06;
	static constexpr char32_t F07       = NCKEY_F07;
	static constexpr char32_t F08       = NCKEY_F08;
	static constexpr char32_t F09       = NCKEY_F09;
	static constexpr char32_t F10       = NCKEY_F10;
	static constexpr char32_t F11       = NCKEY_F11;
	static constexpr char32_t F12       = NCKEY_F12;
	static constexpr char32_t F13       = NCKEY_F13;
	static constexpr char32_t F14       = NCKEY_F14;
	static constexpr char32_t F15       = NCKEY_F15;
	static constexpr char32_t F16       = NCKEY_F16;
	static constexpr char32_t F17       = NCKEY_F17;
	static constexpr char32_t F18       = NCKEY_F18;
	static constexpr char32_t F19       = NCKEY_F19;
	static constexpr char32_t F20       = NCKEY_F20;
	static constexpr char32_t F21       = NCKEY_F21;
	static constexpr char32_t F22       = NCKEY_F22;
	static constexpr char32_t F23       = NCKEY_F23;
	static constexpr char32_t F24       = NCKEY_F24;
	static constexpr char32_t F25       = NCKEY_F25;
	static constexpr char32_t F26       = NCKEY_F26;
	static constexpr char32_t F27       = NCKEY_F27;
	static constexpr char32_t F28       = NCKEY_F28;
	static constexpr char32_t F29       = NCKEY_F29;
	static constexpr char32_t F30       = NCKEY_F30;
	static constexpr char32_t F31       = NCKEY_F31;
	static constexpr char32_t F32       = NCKEY_F32;
	static constexpr char32_t F33       = NCKEY_F33;
	static constexpr char32_t F34       = NCKEY_F34;
	static constexpr char32_t F35       = NCKEY_F35;
	static constexpr char32_t F36       = NCKEY_F36;
	static constexpr char32_t F37       = NCKEY_F37;
	static constexpr char32_t F38       = NCKEY_F38;
	static constexpr char32_t F39       = NCKEY_F39;
	static constexpr char32_t F40       = NCKEY_F40;
	static constexpr char32_t F41       = NCKEY_F41;
	static constexpr char32_t F42       = NCKEY_F42;
	static constexpr char32_t F43       = NCKEY_F43;
	static constexpr char32_t F44       = NCKEY_F44;
	static constexpr char32_t F45       = NCKEY_F45;
	static constexpr char32_t F46       = NCKEY_F46;
	static constexpr char32_t F47       = NCKEY_F47;
	static constexpr char32_t F48       = NCKEY_F48;
	static constexpr char32_t F49       = NCKEY_F49;
	static constexpr char32_t F50       = NCKEY_F50;
	static constexpr char32_t F51       = NCKEY_F51;
	static constexpr char32_t F52       = NCKEY_F52;
	static constexpr char32_t F53       = NCKEY_F53;
	static constexpr char32_t F54       = NCKEY_F54;
	static constexpr char32_t F55       = NCKEY_F55;
	static constexpr char32_t F56       = NCKEY_F56;
	static constexpr char32_t F57       = NCKEY_F57;
	static constexpr char32_t F58       = NCKEY_F58;
	static constexpr char32_t F59       = NCKEY_F59;
	static constexpr char32_t F60       = NCKEY_F60;
	static constexpr char32_t Enter     = NCKEY_ENTER;
	static constexpr char32_t CLS       = NCKEY_CLS;
	static constexpr char32_t DLeft     = NCKEY_DLEFT;
	static constexpr char32_t DRight    = NCKEY_DRIGHT;
	static constexpr char32_t ULeft     = NCKEY_ULEFT;
	static constexpr char32_t URight    = NCKEY_URIGHT;
	static constexpr char32_t Center    = NCKEY_CENTER;
	static constexpr char32_t Begin     = NCKEY_BEGIN;
	static constexpr char32_t Cancel    = NCKEY_CANCEL;
	static constexpr char32_t Close     = NCKEY_CLOSE;
	static constexpr char32_t Command   = NCKEY_COMMAND;
	static constexpr char32_t Copy      = NCKEY_COPY;
	static constexpr char32_t Exit      = NCKEY_EXIT;
	static constexpr char32_t Print     = NCKEY_PRINT;
	static constexpr char32_t CapsLock  = NCKEY_CAPS_LOCK;
	static constexpr char32_t ScrollLock= NCKEY_SCROLL_LOCK;
	static constexpr char32_t NumLock   = NCKEY_NUM_LOCK;
	static constexpr char32_t PrintScreen= NCKEY_PRINT_SCREEN;
	static constexpr char32_t Pause     = NCKEY_PAUSE;
	static constexpr char32_t Menu      = NCKEY_MENU;
	static constexpr char32_t Refresh   = NCKEY_REFRESH;
	static constexpr char32_t Button1   = NCKEY_BUTTON1;
	static constexpr char32_t Button2   = NCKEY_BUTTON2;
	static constexpr char32_t Button3   = NCKEY_BUTTON3;
	static constexpr char32_t Button4   = NCKEY_BUTTON4;
	static constexpr char32_t Button5   = NCKEY_BUTTON5;
	static constexpr char32_t Button6   = NCKEY_BUTTON6;
	static constexpr char32_t Button7   = NCKEY_BUTTON7;
	static constexpr char32_t Button8   = NCKEY_BUTTON8;
	static constexpr char32_t Button9   = NCKEY_BUTTON9;
	static constexpr char32_t Button10  = NCKEY_BUTTON10;
	static constexpr char32_t Button11  = NCKEY_BUTTON11;
	static constexpr char32_t ScrollUp  = NCKEY_SCROLL_UP;
	static constexpr char32_t ScrollDown = NCKEY_SCROLL_DOWN;
	static constexpr char32_t Return    = NCKEY_RETURN;

	static bool IsMouse (char32_t ch) noexcept
	{
		return nckey_mouse_p (ch);
	}

	static bool IsSupPUAa (char32_t ch) noexcept
	{
		return nckey_supppuaa_p (ch);
	}

	static bool IsSupPUAb (char32_t ch) noexcept
	{
		return nckey_supppuab_p (ch);
	}
};

struct EvType
{
	static constexpr ncintype_e Unknown = NCTYPE_UNKNOWN;
	static constexpr ncintype_e Press = NCTYPE_PRESS;
	static constexpr ncintype_e Repeat = NCTYPE_REPEAT;
	static constexpr ncintype_e Release = NCTYPE_RELEASE;
};

std::mutex mtx2;
class nc_terminal
{
public:
	std::string sinput;
	std::mutex mtx;
	static unsigned dimy, dimx;
	bool nomice = false;

	struct notcurses* nc  = nullptr;
	bool bottom = false;

	unsigned rows, cols;
	struct ncplane* stdp = nullptr;	// standard main plane
	struct ncplane_options popts;

	struct ncplane* ncp = nullptr;	// tabbed plane
	struct nctabbed_options topts;
	struct nctabbed* nct = nullptr;	// tabs info linked list

	struct ncplane* input_plane = nullptr;// input plane
	struct ncplane* input_plane_inner = nullptr;// input plane inner
	struct ncplane_options iopts;

	struct ncplane* reader_plane = nullptr;
	struct ncreader* nc_reader = nullptr;

	struct ncplane* status_plane = nullptr; // status plane
	struct ncplane_options sopts;
	std::string sstatus = "status";

	bool is_notcurses_stopped ()
	{
		return nc == nullptr;
	}

	nc_terminal() {}
	~nc_terminal()
	{
		if (is_notcurses_stopped ())
			return;

		destroy();
	}

	void destroy()
	{
		if (is_notcurses_stopped ()) return;

		if (input_plane!=nullptr)
		{
			ncplane_destroy(input_plane);
			input_plane = nullptr;
		}
		if (input_plane_inner!=nullptr)
		{
			ncplane_destroy(input_plane_inner);
			input_plane_inner = nullptr;
		}
		if (status_plane != nullptr)
		{
			ncplane_destroy(status_plane);
			status_plane = nullptr;
		}
		if (nct!=nullptr)
		{
			 nctabbed_destroy(nct);
			 nct = nullptr;
		}
		if (nc_reader!=nullptr)
		{
			 ncreader_destroy(nc_reader, nullptr);
			 nc_reader = nullptr;
		}
	}

	int try_read_key(bool& key_read) const
    {
		uint32_t c;
		ncinput ni;

		key_read = false;
		c = notcurses_get_nblock(nc, &ni);
		if (c != 0) key_read = true;
		return (int)c;
	}

    inline void write(const std::string& s) const
    {
		//............
		// split lines
		// "\r\n"
		// write lines
        std::cout << s << std::flush;
    }

    void get_term_size(unsigned* r, unsigned* c)
    {
		ncplane_dim_yx(stdp, r, c);
	}
	void get_term_size(int& r, int& c)
    {
		unsigned rrows, rcols;
		ncplane_dim_yx(stdp, &rrows, &rcols);
		r = (int)rrows;
		c = (int)rcols;
	}

	void tabcb(struct nctab* t, struct ncplane* p)
	{
		ncplane_erase(p);

		const char* tname = nctab_name(t);
		if (strcmp(tname, "Chat") == 0)
		{
		}
		else if (strcmp(tname, "File") == 0)
		{
		}
		else if (strcmp(tname, "User") == 0)
		{
		}
		else if (strcmp(tname, "Log") == 0)
		{
		}

		ncplane_puttext(p, 0, NCALIGN_CENTER, nctab_name(t), NULL);
		for (int r=1;r<10;r++)
		{
			std::stringstream ss;
			ss << "tab:" << nctab_name(t) << " row:" << r;
			ncplane_cursor_move_yx(p, r, 0);
			ncplane_puttext(p, r, NCALIGN_CENTER, ss.str().c_str(), NULL);
		}
	}

	void status_redraw()
	{
		if (status_plane == nullptr) return;
		ncplane_erase(status_plane);

		ncplane_puttext(status_plane, 0, NCALIGN_CENTER, sstatus.c_str(), NULL);
		sstatus.clear();
	}

	void input_redraw()
	{
		if (input_plane_inner == nullptr) return;
		ncplane_erase(input_plane_inner);

		ncplane_puttext(input_plane_inner, 0, NCALIGN_CENTER, "message to send", NULL);
		for (int r=1;r<4;r++)
		{
			ncplane_cursor_move_yx(input_plane_inner, r, 0);
			//if (r==1)
			//{
				//ncplane_puttext(input_plane_inner, r, NCALIGN_LEFT, sinput.c_str(), NULL);
				////sinput.clear();
			//}
		}
	}

	int reset_main_plane()
	{
		ncplane_erase(stdp);
		ncplane_puttext(stdp, 0, NCALIGN_CENTER,"Crypto Chat by Alain Lanthier (version 0.001)", NULL);
		return 0;
	}

	int reset_input_plane()
	{
		if (input_plane!=nullptr)
		{
			ncplane_destroy(input_plane);
			input_plane = nullptr;
		}
		if (input_plane_inner!=nullptr)
		{
			ncplane_destroy(input_plane_inner);
			input_plane_inner = nullptr;
		}
		if (nc_reader!=nullptr)
		{
			 ncreader_destroy(nc_reader, nullptr);
			 nc_reader = nullptr;
		}

		iopts =
		{
			.y = (rows>=14) ? 1 + 0 + rows - 8 : (unsigned int)7,
			.x = 0,
			.rows = 6,
			.cols = (cols>=40) ? cols : (unsigned int)40
		};
		input_plane = ncplane_create(stdp, &iopts);

		struct ncplane_options sopts_inner =
	    {
			.y = (rows>=14) ? 2 + 0 + rows - 8 : (unsigned int)8,
			.x = 1,
			.rows = 4,
			.cols = (cols>=41) ? cols - 2 : (unsigned int)38
		};
		input_plane_inner = ncplane_create(stdp, &sopts_inner);

		{
		  unsigned irows, icols;
		  ncplane_dim_yx(input_plane, &irows, &icols);

		  nccell c = NCCELL_TRIVIAL_INITIALIZER;
		  nccell_set_bg_rgb8(&c, 0x20, 0x20, 0x20);
		  ncplane_set_base_cell(input_plane, &c);
		  nccell ul = NCCELL_TRIVIAL_INITIALIZER, ur = NCCELL_TRIVIAL_INITIALIZER;
		  nccell ll = NCCELL_TRIVIAL_INITIALIZER, lr = NCCELL_TRIVIAL_INITIALIZER;
		  nccell hl = NCCELL_TRIVIAL_INITIALIZER, vl = NCCELL_TRIVIAL_INITIALIZER;
		  if(nccells_rounded_box(input_plane, NCSTYLE_BOLD, 0, &ul, &ur, &ll, &lr, &hl, &vl))
		  {
			std::cerr << "reset_input_plane nccells_rounded_box failed" << std::endl;
			return -1;
		  }

		  nccell_set_fg_rgb(&ul, 0xff0000); nccell_set_bg_rgb(&ul, 0x002000);
		  nccell_set_fg_rgb(&ur, 0x00ff00); nccell_set_bg_rgb(&ur, 0x002000);
		  nccell_set_fg_rgb(&ll, 0x0000ff); nccell_set_bg_rgb(&ll, 0x002000);
		  nccell_set_fg_rgb(&lr, 0xffffff); nccell_set_bg_rgb(&lr, 0x002000);
		  if(ncplane_box_sized(input_plane, &ul, &ur, &ll, &lr, &hl, &vl, irows - 0, icols,
							   NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM | NCBOXGRAD_RIGHT | NCBOXGRAD_LEFT))
		  {
			std::cerr << "reset_input_planencplane_box_sized failed" << std::endl;
			return -1;
		  }
		}

		ncreader_options reader_opts{};
		bool horscroll = false;
		reader_opts.flags = NCREADER_OPTION_CURSOR | (horscroll ? NCREADER_OPTION_HORSCROLL : 0);

		// can't use Plane until we have move constructor for Reader
		struct ncplane_options nopts = {
			.y = 0,
			.x = 0,
			.rows = 4,
			.cols = (cols>=41) ? cols - 2 : (unsigned int)38,
			.userptr = nullptr,
			.name = "read",
			.resizecb = nullptr,
			.flags = 0,
			.margin_b = 0, .margin_r = 0,
		};
		reader_plane = ncplane_create(input_plane_inner, &nopts);
		ncplane_set_base(reader_plane, "░", 0, 0);
		nc_reader = ncreader_create(reader_plane, &reader_opts);
		if(nc_reader == nullptr)
		{
			//return EXIT_FAILURE;
		}

		return 0;
	}

	int reset_status_plane()
	{
		if (status_plane!=nullptr)
		{
			ncplane_destroy(status_plane);
			status_plane = nullptr;
		}

		sopts =
		{
			.y = (rows>=15) ? 1 + 6 + rows - 8 : (unsigned int)14,
			.x = 0,
			.rows = 1,
			.cols = (cols>=41) ? cols: (unsigned int)40
		};
		status_plane = ncplane_create(stdp, &sopts);
		return 0;
	}

	int reset_tabbed()
	{
		if (nct!=nullptr)
		{
			 nctabbed_destroy(nct);
			 nct = nullptr;
		}

		popts =
		{
			.y = 1,
			.x = 0,
			.rows = (rows>=14) ? rows - 8 : (unsigned int)6,
			.cols = (cols>=40) ? cols  : (unsigned int)40
		};
		ncp = ncplane_create(stdp, &popts);

		topts = {
			.selchan = NCCHANNELS_INITIALIZER(0, 255, 0, 0, 0, 0),
			.hdrchan = NCCHANNELS_INITIALIZER(255, 0, 0, 60, 60, 60),
			.sepchan = NCCHANNELS_INITIALIZER(255, 255, 255, 100, 100, 100),
			.separator = "][",
			.flags = bottom ? NCTABBED_OPTION_BOTTOM : 0
		};
		nct = nctabbed_create(ncp, &topts);

		//nctab* nctabbed_add(nctabbed* nt, nctab* after, nctab* before, tabcb cb, const char* name, void* opaque){
		ncplane_set_base(nctabbed_content_plane(nct), " ", 0, NCCHANNELS_INITIALIZER(255, 255, 255, 15, 60, 15));
		if(     nctabbed_add(nct, NULL, NULL, tabcbfn, "Chat", 	(void*)this) == NULL
			 || nctabbed_add(nct, NULL, NULL, tabcbfn, "Keys", 	(void*)this) == NULL
			 || nctabbed_add(nct, NULL, NULL, tabcbfn, "Log", 	(void*)this) == NULL
			 || nctabbed_add(nct, NULL, NULL, tabcbfn, "Config",(void*)this) == NULL
			 || nctabbed_add(nct, NULL, NULL, tabcbfn, "User", 	(void*)this) == NULL
			 || nctabbed_add(nct, NULL, NULL, tabcbfn, "File", 	(void*)this) == NULL)
		{
			std::cerr << "nctabbed_add failed" << std::endl;
			return -1;
		}

		return 0;
	}

	void create_resize()
	{
		reset_main_plane();
		reset_tabbed();
		reset_input_plane();
		reset_status_plane();
	}

	int init()
	{
		notcurses_options nopts{};
		nopts.flags = NCOPTION_INHIBIT_SETLOCALE;

		//API ALLOC struct notcurses* notcurses_core_init(const notcurses_options* opts, FILE* fp);
		nc = notcurses_core_init(&nopts, NULL);
		if(!nc)
		{
			std::cerr << "notcurses_core_init failed" << std::endl;
			return -1;
		}
		if(!nomice)
		{
			notcurses_mice_enable (nc, NCMICE_ALL_EVENTS);
		}

		stdp = notcurses_stddim_yx(nc, &rows, &cols);

		create_resize();

		nctabbed_redraw(nct);
		input_redraw();
		status_redraw();

		if(notcurses_render(nc) < 0)
		{
			std::cerr << "notcurses_render failed" << std::endl;
			return -1;
		}

		return 0;
	}

	int printf(char *buf)
	{
		sinput.append(buf);
		return 0;
	}

	void putc(char c)
	{
		sinput.append(&c, 1);
	}

	char evtype_to_char(ncinput* ni)
	{
	  switch(ni->evtype){
		case EvType::Unknown:
		  return 'u';
		case EvType::Press:
		  return 'P';
		case EvType::Repeat:
		  return 'R';
		case EvType::Release:
		  return 'L';
	  }
	  return 'X';
	}

	// Print the utf8 Control Pictures for otherwise unprintable ASCII
	char32_t printutf8(char32_t kp)
	{
	  if(kp <= NCKEY_ESC){
		return 0x2400 + kp;
	  }
	  return kp;
	}


	// return the string version of a special composed key
	const char* nckeystr(char32_t spkey)
	{
	  switch(spkey)
	  { // FIXME
		case NCKEY_RESIZE:
		  mtx2.lock();
		  notcurses_refresh (nc, &rows, &cols);
		  mtx2.unlock();

		  // resize
		  create_resize();

		  return "resize event";

		case NCKEY_INVALID: return "invalid";
		case NCKEY_LEFT:    return "left";
		case NCKEY_UP:      return "up";
		case NCKEY_RIGHT:   return "right";
		case NCKEY_DOWN:    return "down";
		case NCKEY_INS:     return "insert";
		case NCKEY_DEL:     return "delete";
		case NCKEY_PGDOWN:  return "pgdown";
		case NCKEY_PGUP:    return "pgup";
		case NCKEY_HOME:    return "home";
		case NCKEY_END:     return "end";
		case NCKEY_F00:     return "F0";
		case NCKEY_F01:     return "F1";
		case NCKEY_F02:     return "F2";
		case NCKEY_F03:     return "F3";
		case NCKEY_F04:     return "F4";
		case NCKEY_F05:     return "F5";
		case NCKEY_F06:     return "F6";
		case NCKEY_F07:     return "F7";
		case NCKEY_F08:     return "F8";
		case NCKEY_F09:     return "F9";
		case NCKEY_F10:     return "F10";
		case NCKEY_F11:     return "F11";
		case NCKEY_F12:     return "F12";
		case NCKEY_F13:     return "F13";
		case NCKEY_F14:     return "F14";
		case NCKEY_F15:     return "F15";
		case NCKEY_F16:     return "F16";
		case NCKEY_F17:     return "F17";
		case NCKEY_F18:     return "F18";
		case NCKEY_F19:     return "F19";
		case NCKEY_F20:     return "F20";
		case NCKEY_F21:     return "F21";
		case NCKEY_F22:     return "F22";
		case NCKEY_F23:     return "F23";
		case NCKEY_F24:     return "F24";
		case NCKEY_F25:     return "F25";
		case NCKEY_F26:     return "F26";
		case NCKEY_F27:     return "F27";
		case NCKEY_F28:     return "F28";
		case NCKEY_F29:     return "F29";
		case NCKEY_F30:     return "F30";
		case NCKEY_F31:     return "F31";
		case NCKEY_F32:     return "F32";
		case NCKEY_F33:     return "F33";
		case NCKEY_F34:     return "F34";
		case NCKEY_F35:     return "F35";
		case NCKEY_F36:     return "F36";
		case NCKEY_F37:     return "F37";
		case NCKEY_F38:     return "F38";
		case NCKEY_F39:     return "F39";
		case NCKEY_F40:     return "F40";
		case NCKEY_F41:     return "F41";
		case NCKEY_F42:     return "F42";
		case NCKEY_F43:     return "F43";
		case NCKEY_F44:     return "F44";
		case NCKEY_F45:     return "F45";
		case NCKEY_F46:     return "F46";
		case NCKEY_F47:     return "F47";
		case NCKEY_F48:     return "F48";
		case NCKEY_F49:     return "F49";
		case NCKEY_F50:     return "F50";
		case NCKEY_F51:     return "F51";
		case NCKEY_F52:     return "F52";
		case NCKEY_F53:     return "F53";
		case NCKEY_F54:     return "F54";
		case NCKEY_F55:     return "F55";
		case NCKEY_F56:     return "F56";
		case NCKEY_F57:     return "F57";
		case NCKEY_F58:     return "F58";
		case NCKEY_F59:     return "F59";
		case NCKEY_BACKSPACE: return "backspace";
		case NCKEY_CENTER:  return "center";
		case NCKEY_ENTER:   return "enter";
		case NCKEY_CLS:     return "clear";
		case NCKEY_DLEFT:   return "down+left";
		case NCKEY_DRIGHT:  return "down+right";
		case NCKEY_ULEFT:   return "up+left";
		case NCKEY_URIGHT:  return "up+right";
		case NCKEY_BEGIN:   return "begin";
		case NCKEY_CANCEL:  return "cancel";
		case NCKEY_CLOSE:   return "close";
		case NCKEY_COMMAND: return "command";
		case NCKEY_COPY:    return "copy";
		case NCKEY_EXIT:    return "exit";
		case NCKEY_PRINT:   return "print";
		case NCKEY_REFRESH: return "refresh";
		case NCKEY_SEPARATOR: return "separator";
		case NCKEY_CAPS_LOCK: return "caps lock";
		case NCKEY_SCROLL_LOCK: return "scroll lock";
		case NCKEY_NUM_LOCK: return "num lock";
		case NCKEY_PRINT_SCREEN: return "print screen";
		case NCKEY_PAUSE: return "pause";
		case NCKEY_MENU: return "menu";
		// media keys, similarly only available through kitty's protocol
		case NCKEY_MEDIA_PLAY: return "play";
		case NCKEY_MEDIA_PAUSE: return "pause";
		case NCKEY_MEDIA_PPAUSE: return "play-pause";
		case NCKEY_MEDIA_REV: return "reverse";
		case NCKEY_MEDIA_STOP: return "stop";
		case NCKEY_MEDIA_FF: return "fast-forward";
		case NCKEY_MEDIA_REWIND: return "rewind";
		case NCKEY_MEDIA_NEXT: return "next track";
		case NCKEY_MEDIA_PREV: return "previous track";
		case NCKEY_MEDIA_RECORD: return "record";
		case NCKEY_MEDIA_LVOL: return "lower volume";
		case NCKEY_MEDIA_RVOL: return "raise volume";
		case NCKEY_MEDIA_MUTE: return "mute";
		case NCKEY_LSHIFT: return "left shift";
		case NCKEY_LCTRL: return "left ctrl";
		case NCKEY_LALT: return "left alt";
		case NCKEY_LSUPER: return "left super";
		case NCKEY_LHYPER: return "left hyper";
		case NCKEY_LMETA: return "left meta";
		case NCKEY_RSHIFT: return "right shift";
		case NCKEY_RCTRL: return "right ctrl";
		case NCKEY_RALT: return "right alt";
		case NCKEY_RSUPER: return "right super";
		case NCKEY_RHYPER: return "right hyper";
		case NCKEY_RMETA: return "right meta";
		case NCKEY_L3SHIFT: return "level 3 shift";
		case NCKEY_L5SHIFT: return "level 5 shift";
		case NCKEY_MOTION: return "mouse (no buttons pressed)";
		case NCKEY_BUTTON1: return "mouse (button 1)";
		case NCKEY_BUTTON2: return "mouse (button 2)";
		case NCKEY_BUTTON3: return "mouse (button 3)";
		case NCKEY_BUTTON4: return "mouse (button 4)";
		case NCKEY_BUTTON5: return "mouse (button 5)";
		case NCKEY_BUTTON6: return "mouse (button 6)";
		case NCKEY_BUTTON7: return "mouse (button 7)";
		case NCKEY_BUTTON8: return "mouse (button 8)";
		case NCKEY_BUTTON9: return "mouse (button 9)";
		case NCKEY_BUTTON10: return "mouse (button 10)";
		case NCKEY_BUTTON11: return "mouse (button 11)";
		default:            return "unknown";
	  }
	}

	void process_enter()
	{
		char* ct = ncreader_contents(nc_reader);
		ncreader_clear(nc_reader);
		//ncreader_redraw(nc_reader);
	}

	void process_char(char32_t r, ncinput& ni)
	{
		char buffer[100] = {0};
		char buffer2[100] = {0};
		char buffer_in[100] = {0};

		if (r == (char32_t)-1)
		{
			int e = errno;
			if(e)
			{
				std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
			}
			return;
		}

		//if (r == 0) return; // interrupted by signal

		if (r == NCKEY_ENTER)
		{
			process_enter();
		}

		sprintf(buffer, "%c%c%c%c%c%c%c%c%c ",
				  ncinput_shift_p(&ni) ? 'S' : 's',
				  ncinput_alt_p(&ni) ? 'A' : 'a',
				  ncinput_ctrl_p(&ni) ? 'C' : 'c',
				  ncinput_super_p(&ni) ? 'U' : 'u',
				  ncinput_hyper_p(&ni) ? 'H' : 'h',
				  ncinput_meta_p(&ni) ? 'M' : 'm',
				  ncinput_capslock_p(&ni) ? 'X' : 'x',
				  ncinput_numlock_p(&ni) ? '#' : '.',
				  evtype_to_char(&ni));

		if(r < 0x80)
		{
		  sprintf(buffer2, "ASCII: [0x%02x (%03d)] '%lc'", r, r, (wint_t)(iswprint(r) ? r : printutf8(r))) ;
		  strcat(buffer, buffer2);

		  // INPUT TEXT
		  //sprintf(buffer_in, "%lc", (wint_t)(iswprint(r) ? r : printutf8(r)));
		  //sinput.append(buffer_in);
		}
		else
		{
		  if (nckey_synthesized_p(r))
		  {
			sprintf(buffer2, "Special: [0x%02x (%02d)] '%s'", r, r, nckeystr(r));
			strcat(buffer, buffer2);

			if(NCKey::IsMouse(r))
			{
			  sprintf(buffer2, "IsMouse %d/%d", ni.x, ni.y);
			  strcat(buffer, buffer2);
			}
		  }
		  else
		  {
			sprintf(buffer2, "Unicode: [0x%08x] '%s'", r, ni.utf8);
			strcat(buffer, buffer2);
		  }
		}

		if(ni.eff_text[0] != ni.id || ni.eff_text[1] != 0)
		{
		  sprintf(buffer2, " effective text '");
		  strcat(buffer, buffer2);

		  for (int c=0; ni.eff_text[c]!=0; c++)
		  {
			unsigned char egc[5]={0};
			if (notcurses_ucs32_to_utf8(&ni.eff_text[c], 1, egc, 4)>=0)
			{
			  sprintf(buffer2, "%s", egc);
			  strcat(buffer, buffer2);

			  // INPUT TEXT
			  //sprintf(buffer_in, "%s", egc);
			  //sinput.append(buffer_in);
			}
		  }
		  sprintf(buffer2, "'");
		  strcat(buffer, buffer2);
		}

		sstatus.append(buffer);
	}

	int loop()
	{
		uint32_t c;
		ncinput ni;
		bool done = false;

		while(done == false)
		//while((c = notcurses_get_nblock(nc, &ni)) != 'q')
		//while((c = notcurses_get_blocking(nc, &ni)) != 'q')
		{
			c = notcurses_get_blocking(nc, &ni);
			if (c == 'L' && ncinput_ctrl_p(&ni))
			{
				done = true;
				break;
			}

			if (c != 0)
			{
				if(ni.evtype == NCTYPE_RELEASE)
				{
				  continue;
				}

				if (c == NCKEY_TAB && !ncinput_shift_p(&ni) && !ncinput_ctrl_p(&ni) && !ncinput_alt_p(&ni) )
                    nctabbed_next(nct);
                else if (c == NCKEY_TAB && ncinput_shift_p(&ni) )
                    nctabbed_prev(nct);
				else if (c == NCKEY_F02 )
					nctabbed_next(nct);
				else if (c == NCKEY_F01)
					nctabbed_prev(nct);
				else
				{
					// OTHER
					process_char(c, ni);

					if (ncreader_offer_input(nc_reader, &ni))
					{
					}
				}
			}

			if (c != 0)
			{
				nctabbed_ensure_selected_header_visible(nct);
				nctabbed_redraw(nct);
				input_redraw();
				status_redraw();

				if(notcurses_render(nc) < 0)
				{
					std::cerr << "notcurses_render failed" << std::endl;
					notcurses_stop(nc);
					return -1;
				}
			}
			else
			{
				// sleep...
			}
		}

		destroy();
		if(notcurses_stop(nc) < 0)
		{
			std::cerr << "notcurses_stop failed" << std::endl;
			nc = nullptr;
			return -1;
		}
		nc = nullptr;
		return 0;
	}
};

int main(int , char** )
{
	if(setlocale(LC_ALL, "") == nullptr)
	{
		std::cerr << "error setlocale(LC_ALL)" << std::endl;
		return EXIT_FAILURE;
	}

  nc_terminal nc_term;

  int r = nc_term.init();
  if (r < 0)
  {
	  std::cerr << "error init " << std::endl;
	  return r;
  }
  return nc_term.loop();
}

// the callback draws the tab contents
void tabcbfn(struct nctab* t, struct ncplane* p, void* curry)
{
	nc_terminal* term = reinterpret_cast<nc_terminal*>(curry);
	if (term == nullptr) return;
	term->tabcb(t, p);
}
