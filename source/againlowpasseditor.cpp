#include "againlowpasseditor.h"
#include "againlowpasscontroller.h"
#include "againparamids.h"

#include "base/source/fstring.h"

#include <stdio.h>
#include <math.h>

//#include <stdio.h>
//#include <math.h>


namespace Steinberg {
namespace Vst {

enum {
	// UI size
	kEditorWidth = 350,
	kEditorHeight = 120
};


AGainLowPassEditorView::AGainLowPassEditorView(void * controller) : 
	VSTGUIEditor(controller),
	cutoffSlider(0),
	cutoffTextEdit(0),
	gainSlider(0),
	gainTextEdit(0),
	vuMeter(0),
	lastVuMeterValue(0.f),
	background(0)
	{
	setIdleRate(50);  // 1000ms/50ms = 20Hz
	ViewRect viewRect(0, 0, kEditorWidth, kEditorHeight);
	setRect(viewRect);
}

//
bool AGainLowPassEditorView::open(void* parent)
{
	if (frame) // already attached!                         
	{
		return false;
	}

	CRect editorSize(0, 0, kEditorWidth, kEditorHeight);

	frame = new CFrame(editorSize, parent, this);
	frame->setBackgroundColor(kGreyCColor);

	background = new CBitmap("background.png");
	frame->setBackground(background);

	CRect size(0, 0, 50, 18);

	//---Cutoff Label--------

	size.offset(10, 10);
	CTextLabel* labelCutoff = new CTextLabel(size, "Cutoff", 0, kShadowText);
	frame->addView(labelCutoff);

	//---Cutoff slider-------
	CBitmap* handleCutoff = new CBitmap("slider_handle.png");
	CBitmap* backgroundSliderCutoff = new CBitmap("slider_background.bmp");

	size(0, 0, 130, 18);
	size.offset(60, 10);
	CPoint offsetCutoff;
	CPoint offsetHandleCutoff(0, 2);
	cutoffSlider = new CHorizontalSlider(size, this, 'CutO', offsetHandleCutoff, size.getWidth(), handleCutoff, backgroundSliderCutoff, offsetCutoff, kLeft);
	frame->addView(cutoffSlider);
	handleCutoff->forget();
	backgroundSliderCutoff->forget();

	//---Cutoff Textedit--------
	size(0, 0, 60, 18);
	size.offset(70 + cutoffSlider->getWidth(), 10);
	cutoffTextEdit = new CTextEdit(size, this, 'CutT', "", 0, k3DIn);
	cutoffTextEdit->setFont(kNormalFontSmall);
	frame->addView(cutoffTextEdit);

	//---Gain--------------------

	//---Gain Label--------
	
	size(0, 0, 50, 18);
	size.offset(10, 40);
	CTextLabel* label = new CTextLabel(size, "Gain", 0, kShadowText);
	frame->addView(label);

	//---Gain slider-------
	CBitmap* handle = new CBitmap("slider_handle.png");
	CBitmap* backgroundSlider = new CBitmap("slider_background.bmp");

	size(0, 0, 130, 18);
	size.offset(60, 40);
	CPoint offset;
	CPoint offsetHandle(0, 2);
	gainSlider = new CHorizontalSlider(size, this, 'Gain', offsetHandle, size.getWidth(), handle, backgroundSlider, offset, kLeft);
	frame->addView(gainSlider);
	handle->forget();
	backgroundSlider->forget();

	//---Gain Textedit--------
	size(0, 0, 60, 18);
	size.offset(70 + gainSlider->getWidth(), 40);
	gainTextEdit = new CTextEdit(size, this, 'GaiT', "", 0, k3DIn);
	gainTextEdit->setFont(kNormalFontSmall);
	frame->addView(gainTextEdit);


	//---VuMeter--------------------
	CBitmap* onBitmap = new CBitmap("vu_on.bmp");
	CBitmap* offBitmap = new CBitmap("vu_off.bmp");

	size(0, 0, 12, 105);
	size.offset(290, 10);
	vuMeter = new CVuMeter(size, onBitmap, offBitmap, 26, kVertical);
	frame->addView(vuMeter);
	onBitmap->forget();
	offBitmap->forget();

	// sync UI controls with controller parameter values
	ParamValue value = getController()->getParamNormalized(kGainId);
	update(kGainId, value);

	ParamValue valueCutoff = getController()->getParamNormalized(kCutOffId);
	update(kCutOffId, valueCutoff);


	return true;
}

void PLUGIN_API AGainLowPassEditorView::close()
{
	if (frame)
	{
		delete frame;
		frame = 0;
	}

	if (background)
	{
		background->forget();
		background = 0;
	}

	gainSlider = 0;
	gainTextEdit = 0;
	cutoffSlider = 0;
	cutoffTextEdit = 0;
	vuMeter = 0;
}

void AGainLowPassEditorView::valueChanged(CControl* pControl)
{
	switch (pControl->getTag())
	{
		//------------------
		case 'Gain':
		{
			controller->setParamNormalized(kGainId, pControl->getValue());
			controller->performEdit(kGainId, pControl->getValue());
		}	break;

		//------------------
		case 'GaiT':
		{
			char text[128];
			gainTextEdit->getText(text);

			String128 string;
			String tmp(text);
			tmp.copyTo16(string, 0, 127);

			ParamValue valueNormalized;
			controller->getParamValueByString(kGainId, string, valueNormalized);

			gainSlider->setValue((float)valueNormalized);
			valueChanged(gainSlider);
			gainSlider->invalid();
		}	break;

		//------------------
		case 'CutO':
		{
			controller->setParamNormalized(kCutOffId, pControl->getValue());
			controller->performEdit(kCutOffId, pControl->getValue());
		}	break;

		//------------------
		case 'CutT':
		{
			char text[128];
			cutoffTextEdit->getText(text);

			String128 string;
			String tmp(text);
			tmp.copyTo16(string, 0, 127);

			ParamValue valueNormalized;
			controller->getParamValueByString(kCutOffId, string, valueNormalized);

			cutoffSlider->setValue((float)valueNormalized);
			valueChanged(cutoffSlider);
			cutoffSlider->invalid();
		}	break;

	}
}

void AGainLowPassEditorView::controlBeginEdit(CControl* pControl)
{
	switch (pControl->getTag())
	{
		//------------------
	case 'Gain':
	{
		controller->beginEdit(kGainId);
	}	break;

	case 'CutO':
	{
		controller->beginEdit(kCutOffId);
	}	break;
	}
}

void AGainLowPassEditorView::controlEndEdit(CControl* pControl)
{
	switch (pControl->getTag())
	{
		//------------------
	case 'Gain':
	{
		controller->endEdit(kGainId);
	}	break;

	case 'CutO':
	{
		controller->endEdit(kCutOffId);
	}	break;

	}
}

tresult PLUGIN_API AGainLowPassEditorView::onSize(ViewRect* newSize)
{
	tresult res = VSTGUIEditor::onSize(newSize);
	return res;
}

tresult PLUGIN_API AGainLowPassEditorView::checkSizeConstraint(ViewRect* rect)
{
	if (rect->right - rect->left < kEditorWidth)
	{
		rect->right = rect->left + kEditorWidth;
	}
	else if (rect->right - rect->left > kEditorWidth + 50)
	{
		rect->right = rect->left + kEditorWidth + 50;
	}
	if (rect->bottom - rect->top < kEditorHeight)
	{
		rect->bottom = rect->top + kEditorHeight;
	}
	else if (rect->bottom - rect->top > kEditorHeight + 50)
	{
		rect->bottom = rect->top + kEditorHeight + 50;
	}
	return kResultTrue;
}

tresult PLUGIN_API AGainLowPassEditorView::findParameter(int32 xPos, int32 yPos, ParamID& resultTag) {
	CPoint where(xPos, yPos);
	if (gainSlider->hitTest(where, kGainId))
	{
		resultTag = kGainId;
		return kResultOk;
	}

	if (gainTextEdit->hitTest(where, kGainId))
	{
		resultTag = kGainId;
		return kResultOk;
	}

	if (cutoffSlider->hitTest(where, kCutOffId))
	{
		resultTag = kCutOffId;
		return kResultOk;
	}

	if (cutoffTextEdit->hitTest(where, kCutOffId))
	{
		resultTag = kCutOffId;
		return kResultOk;
	}
	return kResultFalse;
}

void AGainLowPassEditorView::update(ParamID tag, ParamValue value)
{
	switch (tag)
	{
		//------------------
	case kGainId:
		if (gainSlider)
		{
			gainSlider->setValue((float)value);

			if (gainTextEdit)
			{
				String128 string;
				controller->getParamStringByValue(kGainId, value, string);

				String tmp(string);
				char text[128];
				tmp.copyTo8(text, 0, 127);

				gainTextEdit->setText(text);
			}
		}
		break;


	case kCutOffId:
		if (cutoffSlider)
		{
			cutoffSlider->setValue((float)value);

			if (cutoffTextEdit)
			{
				String128 string;
				controller->getParamStringByValue(kCutOffId, value, string);

				String tmp(string);
				char text[128];
				tmp.copyTo8(text, 0, 127);

				cutoffTextEdit->setText(text);
			}
		}
		break;
		//------------------

	case kVuPPMId:
		lastVuMeterValue = (float)value;
		break;
	}
}

tresult PLUGIN_API AGainLowPassEditorView::queryInterface(const char* iid, void** obj)
{
	QUERY_INTERFACE(iid, obj, IParameterFinder::iid, IParameterFinder)
	return VSTGUIEditor::queryInterface(iid, obj);
}

CMessageResult AGainLowPassEditorView::notify(CBaseObject* sender, const char* message)
{
	if (message == CVSTGUITimer::kMsgTimer)
	{
		if (vuMeter)
		{
			vuMeter->setValue(1.f - ((lastVuMeterValue - 1.f) * (lastVuMeterValue - 1.f)));
			lastVuMeterValue = 0.f;
		}
	}
	return VSTGUIEditor::notify(sender, message);
}

} // namespace Steinberg
} // namespace Vst