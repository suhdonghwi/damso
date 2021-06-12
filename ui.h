#include <string.h>
#include <stdio.h>

#include "termbox.h"

void tb_clear_line(int y)
{
	for (int x = 0; x < tb_width(); x++)
	{
		tb_change_cell(x, y, ' ', TB_DEFAULT, TB_DEFAULT);
	}
}

void tb_change_cell_style(int x, int y, uint16_t fg, uint16_t bg)
{
	tb_change_cell(x, y, tb_cell_buffer()[y * tb_width() + x].ch, fg, bg);
}

void ui_print(int x, int y, char *str, uint16_t fg, uint16_t bg)
{
	for (int i = 0; i < strlen(str); i++)
	{
		tb_change_cell(x++, y, str[i], fg, bg);
	}
}

int ui_print_center(int y, char *str, uint16_t fg, uint16_t bg)
{
	int start = (tb_width() - strlen(str)) / 2;
	ui_print(start, y, str, fg, bg);

	return start;
}