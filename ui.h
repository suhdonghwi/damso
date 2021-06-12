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

void tb_change_cell_style(int x_from, int count, int y, uint16_t fg, uint16_t bg)
{
	struct tb_cell *buf = tb_cell_buffer() + (y * tb_width() + x_from);
	for (int i = 0; i < count; i++)
	{
		tb_change_cell(x_from + i, y, buf->ch, fg, bg);
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

void ui_modal(int width, int height, char *content, int start_y)
{
	int rect_top = (tb_height() - height) / 2, rect_left = (tb_width() - width) / 2;
	ui_rect(rect_top,
					rect_top + height,
					rect_left,
					rect_left + width,
					0x07);

	char buf[BUF_SIZE], *tmp = content;
	for (int i = 0;; i++)
	{
		int to_copy = width - 4;

		memset(buf, '\0', BUF_SIZE);
		strncpy(buf, content, to_copy);
		ui_print_center(rect_top + start_y + i, buf, 0x07, TB_DEFAULT);

		content += to_copy;
		if (content - tmp >= strlen(tmp))
		{
			break;
		}
	}

	tb_present();
}

int ui_dialog(int width, int height, char *content, char *left, char *right)
{
	ui_modal(width, height, content, 2);

	int rect_top = (tb_height() - height) / 2,
			margin = (width - strlen(left) - strlen(right)) / 3,
			selection_line = rect_top + height - 2;
	char selections[BUF_SIZE] = "";
	sprintf(selections, "%s%*c%s", left, margin, ' ', right);
	int start = ui_print_center(selection_line, selections, 0x07, TB_DEFAULT);
	tb_present();

	int selection = 0;
	struct tb_event ev;

	while (1)
	{
		uint16_t left_fg = selection == 0 ? 0xe8 : 0x07;
		uint16_t left_bg = selection == 0 ? 0x02 : TB_DEFAULT;
		uint16_t right_fg = selection == 0 ? 0x07 : 0xe8;
		uint16_t right_bg = selection == 0 ? TB_DEFAULT : 0x02;

		tb_change_cell_style(start - 1, strlen(left) + 2, selection_line, left_fg, left_bg);
		tb_change_cell_style(start + strlen(left) + margin - 1,
												 strlen(right) + 2,
												 selection_line, right_fg, right_bg);

		tb_present();

		if (tb_poll_event(&ev))
		{
			switch (ev.type)
			{
			case TB_EVENT_KEY:
				if (ev.key == TB_KEY_ARROW_RIGHT && selection == 0)
				{
					selection = 1;
				}
				else if (ev.key == TB_KEY_ARROW_LEFT && selection == 1)
				{
					selection = 0;
				}
				else if (ev.key == TB_KEY_ENTER)
				{
					return selection;
				}
				break;
			}
		}
	}

	return -1;
}