//#################################################################
//my_curses lib:	OutputWin_Class
//Definde by :      LIP
//Author:			LIP
//Date:				Aug 12, 2002
//Version:			1.0
//Copy right:		NewTone
//#################################################################
#ifndef _OUTPUT_WIN_CLASS_
#define _OUTPUT_WIN_CLASS_
#include <curses.h>

#define OUTPUT_WIN_MAX_BUF_LINES_NUM		128
#define OUTPUT_WIN_MAX_LINE_LENGTH	256
typedef struct{
	char str[OUTPUT_WIN_MAX_LINE_LENGTH];
}OutputWinLine;
class OutputWin_Class
{
public:
	WINDOW *pWin;
	int		win_height;
	int		win_width;
	
	int		win_y0;
	int		win_x0;
	
	chtype	box_att;
	chtype	border_att;
	
	int		show_Line_x0;
	int		showed_LineLength;
	
	OutputWinLine lines[OUTPUT_WIN_MAX_BUF_LINES_NUM];
	int		line_num;
	int		showFirstLine;
	int		maxShowedLine;
	int		curFirstLine;
	int		curLastLine;
	
	char	*infoStr;
public:
	OutputWin_Class(){};
	
	int	init(WINDOW *pwin,int win_y,int win_x,int height,int width,chtype box, chtype border);
	
	virtual int deal_inputKey(int key);
	virtual int redraw(void);
	virtual int clear(void);
	int	clean_buf(void);
	
	int	out_printf(char *fmt,...);
};
#endif
