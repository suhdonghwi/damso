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

int ui_modal(int width, int height, char *content, int start_y)
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

	int selection = 0;
	struct tb_event ev;

	do
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

		if (selection == 0)
		{
			tb_change_cell_style(start - 1, start + strlen(left), selection_line, 0xe8, 0x02);
			tb_change_cell_style(start + strlen(left) + margin - 1,
													 start + strlen(left) + margin + strlen(right),
													 selection_line, 0x07, TB_DEFAULT);
		}
		else
		{
			tb_change_cell_style(start - 1, start + strlen(left), selection_line, 0x07, TB_DEFAULT);
			tb_change_cell_style(start + strlen(left) + margin - 1,
													 start + strlen(left) + margin + strlen(right),
													 selection_line, 0xe8, 0x02);
		}
		tb_present();
	} while (tb_poll_event(&ev));
}