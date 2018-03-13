#pragma once
#include "imguiUtils.h"

/* 
	Sets the current cursor position such that the next
	line of text will be drawn in the middle of the parent
	window.
*/
void setCenterText(char* textToCenter) {

	ImVec2 textSize = ImGui::CalcTextSize(textToCenter);
	ImVec2 windowSize = ImGui::GetWindowSize();

	float remainingSpace = windowSize.x - textSize.x;
	
	// Left border to center text.
	float leftBorder = remainingSpace / 2.0;

	ImGui::SetCursorPosX(leftBorder);
}

/*
	Draws a page title in the center of the screen
	with the specified target color.
*/
void drawTitle(char* text, ImVec4 color) {
	setCenterText(text);
	ImGui::TextColored(color, text);
}