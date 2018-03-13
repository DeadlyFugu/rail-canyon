#include "Help.h"
#include "imgui.h"
#include "imguiUtils.h"

Help::Help() { }
Help::~Help() { }

/*
	Assumes there are two columns only.
*/
void addColumn(char* left, char* right) {
	// Left Side
	ImGui::Text(left);
	ImGui::NextColumn();

	// Right Side
	ImGui::Text(right);
	ImGui::NextColumn();
}

void drawKeybindings() {
	drawTitle("Rail Canyon Keybindings", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

	// Setup Columns
	ImGui::Columns(2);

	addColumn("Move Around", "W,A,S,D");
	addColumn("Rotate Screen", "Arrow Keys");
	addColumn("Rotate Screen", "Middle Mouse Button");
	addColumn("Raise/Lower", "Q, E");
	ImGui::Separator();

	addColumn("Increase Speed 2.5x", "Shift (Hold)");
	addColumn("Increase Speed 10x", "Alt (Hold)");
	ImGui::Separator();

	addColumn("Look at (0,0,0)", "Space");
	addColumn("Take Screenshot", "F2");
	addColumn("Toggle HUD", "Tab");

	// Reset Columns
	ImGui::Columns(1);

	ImGui::Spacing();
	ImGui::Spacing();
}

void drawAbout() {
	drawTitle("About Rail Canyon", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

	ImGui::Text("Sonic Heroes level editor, by DeadlyFugu");
	ImGui::Text("Released under the zlib license.");

	ImGui::Spacing();
	ImGui::Spacing();
}

void drawContributors() {

	drawTitle("Contributors (of railcanyon)", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
	
	// Setup Columns
	ImGui::Columns(2);

	addColumn("DeadlyFugu", "railcanyon");
	addColumn("Sewer56", "Misc stuff");

	// Reset Columns
	ImGui::Columns(1);
}

void Help::drawUI() {

	drawAbout();
	drawKeybindings();
	drawContributors();
}