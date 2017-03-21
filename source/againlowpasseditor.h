#pragma once

#include "public.sdk/source/vst/vstguieditor.h"
#include "pluginterfaces/vst/ivstplugview.h"
#include "pluginterfaces/vst/ivstcontextmenu.h"

namespace Steinberg {
namespace Vst {

class AGainLowPassEditorView :
	public VSTGUIEditor,
	public CControlListener,
	public IParameterFinder {
public:
	AGainLowPassEditorView(void* controller);

	//---from VSTGUIEditor---------------
	bool PLUGIN_API open(void* parent);
	void PLUGIN_API close();
	CMessageResult notify(CBaseObject* sender, const char* message);
	void beginEdit(long /*index*/) {}
	void endEdit(long /*index*/) {}

	//---from CControlListener---------
	void valueChanged(CControl* pControl);
	//long controlModifierClicked(CControl* pControl, long button); to je pro kontext nabidku, to delat nebudu
	void controlBeginEdit(CControl* pControl); 
	void controlEndEdit(CControl* pControl);

	//---from EditorView---------------
	tresult PLUGIN_API onSize(ViewRect* newSize);
	tresult PLUGIN_API canResize() { return kResultFalse; }
	tresult PLUGIN_API checkSizeConstraint(ViewRect* rect);

	//---from IParameterFinder---------------
	tresult PLUGIN_API findParameter(int32 xPos, int32 yPos, ParamID& resultTag);

	//---from IContextMenuTarget---------------
	// tresult PLUGIN_API executeMenuItem(int32 tag); to je pro kontext nabidku, to delat nebudu

	DELEGATE_REFCOUNT(VSTGUIEditor)
	tresult PLUGIN_API queryInterface(const char* iid, void** obj);

	//---Internal Function------------------
	void update(ParamID tag, ParamValue value);
	//void messageTextChanged(); // tohle patri k tomu textovymu poli to delat nebudu

protected:
	CHorizontalSlider* cutoffSlider;
	CTextEdit* cutoffTextEdit;
	CHorizontalSlider* gainSlider;
	CTextEdit* gainTextEdit;
	CVuMeter* vuMeter;
	CBitmap* background;

	float lastVuMeterValue;
};


} // namespace Steinberg
} // namespace Vst