#include <string.h>
#include <stdio.h>

#include "termbox.h"

void ui_print(int x, int y, char *str)
{
	while (*str)
	{
		tb_change_cell(x++, y, *str, TB_WHITE, TB_DEFAULT);
		str++;
	}
}