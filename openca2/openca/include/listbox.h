//#################################################################
//my_curses lib:	ListBox_Class
//Definde by :      LIP
//Author:			LIP
//Date:				Aug 12, 2002
//Version:			1.0
//Copy right:		NewTone
//#################################################################
#ifndef _LIST_BOX_CLASS_
#define _LIST_BOX_CLASS_
#include <curses.h>
#include "output_winclass.h"

#define LIST_BOX_MAX_ROW_NUM		100
#define LIST_BOX_MAX_COLUME_NUM		10
#define LIST_BOX_MAX_COLUME_WIDTH	32
typedef struct{
	char row[LIST_BOX_MAX_COLUME_NUM][LIST_BOX_MAX_COLUME_WIDTH];
}ListBoxItem;
class ListBox_Class
{
public:
	WINDOW *pWin;
	int		win_height;
	int		win_width;
	
	int		win_y0;
	int		win_x0;
	
	chtype	box_att;
	chtype	border_att;
	
	char	colume_name[LIST_BOX_MAX_COLUME_NUM][32];
	int		colume_width[LIST_BOX_MAX_COLUME_NUM];
	int		colume_num;
	int		showFirstCol;
	int		maxShowedCol;
	
	ListBoxItem items[LIST_BOX_MAX_ROW_NUM];
	int		item_num;
	int		showFirstItem;
	int		maxShowedItem;
	int		curItem;
	
	char	*infoStr;
	OutputWin_Class *pOutput;
public:
	ListBox_Class(){pOutput=NULL;};
	
	virtual int	init(WINDOW *pwin,int height,int width,int win_y,int win_x,chtype box, chtype border);
	
	virtual int deal_inputKey(int key);
	virtual int redraw(void);
	virtual int clear(void);
	int	removeItem(int item);
	
	int setColumns(int col_num,...);
	int	addRowItem(int item,...);
};
#endif
