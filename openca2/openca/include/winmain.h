//#################################################################
//my_curses lib:	WinMain_Class
//Definde by :      LIP
//Author:			LIP
//Date:				Aug 12, 2002
//Version:			1.0
//Copy right:		NewTone
//#################################################################
#ifndef _WINMAIN_CLASS_
#define _WINMAIN_CLASS_
#include <curses.h>
#include "output_winclass.h"
#include "menuclass.h"

class WinMain_Class
{
public:
	WINDOW *pWin;
	int		win_height;
	int		win_width;
	
	int		win_y0;
	int		win_x0;
	int		client_height;
	int		client_width;
	int		cli_y0;
	int		cli_x0;
	
	chtype	box_att;
	chtype	border_att;
	
	Menu_Class	*winmain_menu;
	int		havemenuFlag;
	int		menuShowFlag;
	
	char	infoStr[128];
	OutputWin_Class *pOutput;
public:
	WinMain_Class(){pOutput=NULL;};
	
	int	init(WINDOW *pwin,int height,int width,int win_y,int win_x,chtype box, chtype border,int menuFlag);
	int setMenu(Menu_Class *main_menu);
	
	int setClient(int y0,int x0,int height,int width);
	int setOutputWin(OutputWin_Class *ptr);
	int showMenu(void);
	virtual int deal_inputKey(void);
	int redraw(void);
	int clear(void);
	int clear_client(void);
	int writeClient(char *fmt,...);
};
#endif
