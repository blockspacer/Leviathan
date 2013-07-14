#include "Include.h"
// ------------------------------------ //
#ifndef LEVIATHAN_GUIBASICTEXT
#include "GuiBasicText.h"
#endif
#include "GuiManager.h"
using namespace Leviathan;
using namespace Leviathan::Gui;
// ------------------------------------ //
DLLEXPORT Leviathan::Gui::GuiBasicText::GuiBasicText(int slot, int zorder) : BaseGraphicalComponent(slot, zorder), Text(L""), Font(L"Arial"){
	// set every created flag to false //
	IsExpensiveText = false;
	NeedsAdjusting = true;

	// set type //
	CType = GUI_GRAPHICALCOMPONENT_TYPE_SIMPLETEXT;

	TextModifier = 1;

	OldExpensiveState = false;

	// flag default values //
	TextAdjustMode = GUI_BASICTEXT_MODE_JUSTRENDER;

	// ensure that length is negative so that it gets calculated when needed //
	TextLength = -1;
}

DLLEXPORT Leviathan::Gui::GuiBasicText::~GuiBasicText(){
	// nothing to delete //
}
// ------------------------------------ //
DLLEXPORT bool Leviathan::Gui::GuiBasicText::Init(const Float4 &colour, const wstring &text, const wstring &font, float textmodifier, 
	bool expensivetext, int adjustmode)
{
	Text = text;
	IsExpensiveText = expensivetext;
	OldExpensiveState = IsExpensiveText;

	TextAdjustMode = adjustmode;

	PrimaryTextColour = colour;

	Font = font;

	TextModifier = textmodifier;

	// ensure that updated is set //
	RUpdated = true;
	NeedsAdjusting = true;

	return true;
}
DLLEXPORT void Leviathan::Gui::GuiBasicText::Release(RenderBridge* bridge){
	// remove from render bridge //
	size_t i = bridge->GetSlotIndex(Slot);

	SAFE_DELETE(bridge->DrawActions[i]);

	bridge->DrawActions.erase(bridge->DrawActions.begin());
}
// ------------------------------------ //
DLLEXPORT void Leviathan::Gui::GuiBasicText::Render(RenderBridge* bridge, Graphics* graph){
	if(NeedsAdjusting){
		// needs to adjust size //
		_CheckTextAdjustment();
		NeedsAdjusting = false;
	}

	bool releaseold = false;

	if(IsExpensiveText != OldExpensiveState){
		// needs to change text type //
		RUpdated = true;
		releaseold = true;
	}

	if(!RUpdated){
		// nothing to update //
		return;
	}

	// get index from bridge //
	size_t i = bridge->GetSlotIndex(Slot);

	if(releaseold){
		// needs to delete old rendering blob //
		SAFE_DELETE(bridge->DrawActions[i]);
	}


	// ensure that right thing and parameters are in the render bridge //


	if(bridge->DrawActions[i] == NULL){
		// create new object //

		if(!IsExpensiveText){

			bridge->DrawActions[i] = new BasicTextRendBlob(ZOrder, Slot, Position, PrimaryTextColour, TextModifier, Text, Font, CoordType);


		} else {
			// create expensive text block //
			bridge->DrawActions[i] = new ExpensiveTextRendBlob(ZOrder, Slot, Position, PrimaryTextColour, TextModifier, Text, Font, CoordType, 
				TextAdjustMode == GUI_BASICTEXT_MODE_TRYTOAUTOFIT, Size);
		}



	} else {
		// update existing object //
		if(!IsExpensiveText){

			BasicTextRendBlob* tmpptr = (BasicTextRendBlob*)bridge->DrawActions[i];

			tmpptr->Update(ZOrder, Position, PrimaryTextColour, TextModifier, Text, Font, CoordType);

		} else {
			// update expensive text object //

			ExpensiveTextRendBlob* tmpptr = (ExpensiveTextRendBlob*)bridge->DrawActions[i];

			tmpptr->Update(ZOrder, Position, PrimaryTextColour, TextModifier, Text, Font, CoordType, TextAdjustMode == GUI_BASICTEXT_MODE_TRYTOAUTOFIT,
				Size);

		}
	}

	// set as non updated //
	RUpdated = false;
}
// ------------------------------------ //
DLLEXPORT bool Leviathan::Gui::GuiBasicText::UpdateText(const wstring &texttoset, bool expensivetext /*= false*/, int adjustmode /*= GUI_BASICTEXT_MODE_JUSTRENDER*/){
	// set new data //
	Text = texttoset;
	IsExpensiveText = expensivetext;
	TextAdjustMode = adjustmode;


	// set as updated //
	RUpdated = true;
	NeedsAdjusting = true;

	// set length to uncalculated //
	TextLength = -1;

	return true;
}
// ------------------------------------ //
void Leviathan::Gui::GuiBasicText::_OnLocationOrSizeChange(){
	// set as updated //
	RUpdated = true;
}

void Leviathan::Gui::GuiBasicText::_CheckTextAdjustment(){
	// return if adjustment isn't wanted //
	if(TextAdjustMode == GUI_BASICTEXT_MODE_JUSTRENDER)
		return;
	// the text renderer will automatically adjust it, just set parameters to the 
}

DLLEXPORT bool Leviathan::Gui::GuiBasicText::GetTextLength(float &lengthreceiver, float &heightreceiver){
	if(TextLength < 0){
		// calculate length and height //
		TextLength = GuiManager::Get()->GetGraph()->CountTextRenderLength(Text, Font, TextModifier, CoordType);

		TextHeigth = GuiManager::Get()->GetGraph()->GetTextRenderHeight(Font, TextModifier, CoordType);
	}

	lengthreceiver = TextLength;
	heightreceiver = TextHeigth;
	return true;
}
