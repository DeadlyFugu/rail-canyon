#pragma once
#include "imgui.h"

/*
	Sets the current cursor position such that the next
	line of text will be drawn in the middle of the parent
	window.
*/
void setCenterText(char* textToCenter);

/*
	Draws a page title in the center of the screen
	with the specified target color.
*/
void drawTitle(char* text, ImVec4 color);