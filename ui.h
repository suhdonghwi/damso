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

void tb_change_cell_style(int x_from, int x_to, int y, uint16_t fg, uint16_t bg)
{
	struct tb_cell *buf = tb_cell_buffer() + (y * tb_width() + x_from);
	for (int x = x_from; x <= x_to; x++)
	{
		tb_change_cell(x, y, buf->ch, fg, bg);
		buf++;
	}
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

void ui_rect(int top, int bottom, int left, int right, uint16_t fg)
{
	tb_change_cell(left, top, u'┌', fg, TB_DEFAULT);
	tb_change_cell(right, top, u'┐', fg, TB_DEFAULT);
	tb_change_cell(right, bottom, u'┘', fg, TB_DEFAULT);
	tb_change_cell(left, bottom, u'└', fg, TB_DEFAULT);

	for (int x = left + 1; x < right; x++)
	{
		tb_change_cell(x, top, u'─', fg, TB_DEFAULT);
		tb_change_cell(x, bottom, u'─', fg, TB_DEFAULT);
	}

	for (int y = top + 1; y < bottom; y++)
	{
		tb_change_cell(left, y, u'│', fg, TB_DEFAULT);
		tb_change_cell(right, y, u'│', fg, TB_DEFAULT);
	}

	for (int x = left + 1; x < right; x++)
	{
		for (int y = top + 1; y < bottom; y++)
		{
			tb_change_cell(x, y, ' ', TB_DEFAULT, TB_DEFAULT);
		}
	}
}