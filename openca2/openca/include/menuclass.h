//#################################################################
//my_curses lib:	Menu_Class
//Definde by :      LIP
//Author:			LIP
//Date:				Aug 12, 2002
//Version:			1.0
//Copy right:		NewTone
//#################################################################
#ifndef _MENU_CLASS_
#define _MENU_CLASS_
#include <curses.h>
#include "inputbox.h"
#include "listbox.h"
#include "output_winclass.h"

#define	MAX_MENU_ITEM_NUM	15
#define	H_LINE_MENU			0
#define V_LINE_MENU			1

#define MENU_ITEM_EXEC_CMD			1
#define MENU_ITEM_SUB_INPUT_BOX		2
#define MENU_ITEM_SUB_MENU			3
#define MENU_ITEM_SUB_LIST_BOX		4
#define	MENU_ITEM_SUB_OUTPUT_WIN	5

class	Menu_Class;

typedef struct{
	char	nameStr[32];
	void 	(*pfunc)(void *param);
	void	*param;
	int		itemType;
	Menu_Class *pSubMenu;
	InputBox_Class *pInputBox;
	ListBox_Class  *pListBox;
	OutputWin_Class	*pOutputWin;
}MenuItems;
class	Menu_Class
{
public:
	WINDOW *pWin;
	int		win_height;
	int		win_width;

	int		y0;
	int		x0;
	int		menu_height;
	int		menu_width;
	int		mymenuType;
	MenuItems	items[MAX_MENU_ITEM_NUM];
	int		item_num;
	int		showFirstItem;
	int		maxShowedItem;
	int		curItem;
	int		subCallFlag;
	
	char	*infoStr;
	OutputWin_Class *pOutput;
public:
	Menu_Class(){pOutput=NULL;}
	
	virtual int	init(WINDOW *pwin,int height,int width,int win_y,int win_x,int max_item,int menuType=H_LINE_MENU);
	int setItem(int item,int item_type,char *itemStr);
	
	int setItemFunc(int item,void (*pfunc)(void *param),void *param);
	int setItemInputBox(int item,InputBox_Class *pInput_box);
	int setItemMenu(int item,Menu_Class *pMenu);
	int setItemListBox(int item,ListBox_Class *pList_box);
	int setItemOutputWin(int item,OutputWin_Class *pOutput_win);
	virtual int deal_inputKey(int key);
	virtual int redraw(void);
	int clear(void);
	int clearInfo(void);
};
#endif

