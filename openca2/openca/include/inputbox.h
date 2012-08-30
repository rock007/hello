//#################################################################
//my_curses lib:	InputBox_Class
//Definde by :      LIP
//Author:			LIP
//Date:				Aug 12, 2002
//Version:			1.0
//Copy right:		NewTone
//#################################################################
#ifndef _INPUT_BOX_CLASS_
#define _INPUT_BOX_CLASS_
#include <curses.h>
#include "output_winclass.h"

#define INPUT_BOX_MAX_ITEM_NUM		10
#define INPUT_BOX_ITEM_STR_MAX_LENGTH	128

#define	INPUT_BOX_BUTTON_SELECTED	0
#define INPUT_BOX_ITEM_SELECTED		1

typedef struct{
	char	itemName[32];
	char	itemStr[INPUT_BOX_ITEM_STR_MAX_LENGTH];
	int		flag;
	int		x0;
	int		showFirstChr;
	int		maxShowedChr;
}InputBoxItems;
class InputBox_Class
{
public:
	WINDOW *pWin;
	int		win_height;
	int		win_width;
	
	int		win_y0;
	int		win_x0;
	
	chtype	box_att;
	chtype	border_att;
	
	InputBoxItems	items[INPUT_BOX_MAX_ITEM_NUM];
	int		item_num;
	int		showFirstItem;
	int		maxShowedItem;
	int		curItem;
	int		item_button_flag;
	int		curButton;
	int		cur_x;
	char	showStr[128];
	char	buttonStr[2][8];
	
	char	*infoStr;
	OutputWin_Class *pOutput;
	
public:
	InputBox_Class(){ pOutput = NULL; };
	
	int	setButtonStr(char *buttonStr1,char *buttonStr2);
	
	virtual int	init(WINDOW *pwin,int height,int width,int win_y,int win_x,chtype box, chtype border);
	
	virtual int save(char *returnStr) = 0;
	
	virtual int deal_inputKey(int key);
	virtual int redraw(void);
	int clear(void);
	
	int	addItem(const char *item_name,const char *initStr,int flag);
};
#endif
