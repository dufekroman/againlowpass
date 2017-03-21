#include "againlowpasscontroller.h"
#include "againparamids.h"
#include "againlowpasseditor.h"
#include "stdio.h"
#include "pluginterfaces/base/ustring.h"
//#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include "pluginterfaces/base/ibstream.h"

namespace Steinberg {
	namespace Vst {

//
// je spravne dat GainParameter do zvlastniho souboru a 
// nedefinovat ho pro kazdy plugin zvlast, ale ja nechci zasahovat do puvodnich souboru
//
class GainParameter1 : public Parameter
{
public:
	GainParameter1(int32 flags, int32 id);

	virtual void toString(ParamValue normValue, String128 string) const;
	virtual bool fromString(const TChar* string, ParamValue& normValue) const;
};

GainParameter1::GainParameter1(int32 flags, int32 id)
{
	Steinberg::UString(info.title, USTRINGSIZE(info.title)).assign(USTRING("Gain"));
	Steinberg::UString(info.units, USTRINGSIZE(info.units)).assign(USTRING("dB"));

	info.flags = flags;
	info.id = id;
	info.stepCount = 0;
	info.defaultNormalizedValue = 0.5f;
	info.unitId = kRootUnitId;

	setNormalized(1.f);
}

void GainParameter1::toString(ParamValue normValue, String128 string) const
{
	char text[32];
	if (normValue > 0.0001)
	{
		sprintf(text, "%.2f", 20 * log10f((float)normValue));
	}
	else
	{
		strcpy(text, "-oo");
	}

	Steinberg::UString(string, 128).fromAscii(text);
}

bool GainParameter1::fromString(const TChar* string, ParamValue& normValue) const
{
	String wrapper((TChar*)string); // don't know buffer size here!
	double tmp = 0.0;
	if (wrapper.scanFloat(tmp))
	{
		// allow only values between -oo and 0dB
		if (tmp > 0.0)
		{
			tmp = -tmp;
		}

		normValue = expf(logf(10.f) * (float)tmp / 20.f);
		return true;
	}
	return false;
}

//cutoff frekvence je celociselna, ale CHorizontalSlider pocita s float vnitrni reprezentaci, takze to bude sranda

class CutoffParameter : public Parameter
{
public:
	CutoffParameter(int32 flags, int32 id);

	virtual void toString(ParamValue normValue, String128 string) const;
	virtual bool fromString(const TChar* string, ParamValue& normValue) const;
};

CutoffParameter::CutoffParameter(int32 flags, int32 id)
{
	Steinberg::UString(info.title, USTRINGSIZE(info.title)).assign(USTRING("Cutoff"));
	Steinberg::UString(info.units, USTRINGSIZE(info.units)).assign(USTRING("Hz"));

	info.flags = flags;
	info.id = id;
	info.stepCount = 0;
	info.defaultNormalizedValue = 200;
	info.unitId = kRootUnitId;

	setNormalized(200);
}

void CutoffParameter::toString(ParamValue normValue, String128 string) const
{
	char text[32];
	if (normValue > 0)
	{
		sprintf(text, "%u", ((uint32)normValue));
	}
	else
	{
		strcpy(text, "0");
	}

	Steinberg::UString(string, 128).fromAscii(text);
}

bool CutoffParameter::fromString(const TChar* string, ParamValue& normValue) const
{
	String wrapper((TChar*)string); // don't know buffer size here!
	uint32 tmp = 0;
	if (wrapper.scanUInt32(tmp))
	{


		normValue = (float)tmp;
		return true;
	}
	return false;
}

tresult PLUGIN_API AGainLowPassController::initialize(FUnknown* context)
{
	tresult result = EditControllerEx1::initialize(context);
	if (result != kResultOk)
	{
		return result;
	}

	//--- Create Units-------------
	UnitInfo unitInfo;
	Unit* unit;

	// create a unit1 for the gain and cutoff
	unitInfo.id = 1;
	unitInfo.parentUnitId = kRootUnitId;	// attached to the root unit

	Steinberg::UString(unitInfo.name, USTRINGSIZE(unitInfo.name)).assign(USTRING("Unit1"));

	unitInfo.programListId = kNoProgramListId;

	unit = new Unit(unitInfo);
	addUnit(unit);

	//---Create Parameters------------

	//---Gain parameter--
	GainParameter1* gainParam = new GainParameter1(ParameterInfo::kCanAutomate, kGainId);
	parameters.addParameter(gainParam);
	gainParam->setUnitID(1);

	//---Cutoff parameter--
	CutoffParameter* cutoffParam = new CutoffParameter(ParameterInfo::kCanAutomate, kCutOffId);
	parameters.addParameter(cutoffParam);
	cutoffParam->setUnitID(1);
	

	//---VuMeter parameter---
	int32 stepCount = 0;
	ParamValue defaultVal = 0;
	int32 flags = ParameterInfo::kIsReadOnly;
	int32 tag = kVuPPMId;
	parameters.addParameter(STR16("VuPPM"), 0, stepCount, defaultVal, flags, tag);

	//---Bypass parameter---
	stepCount = 1;
	defaultVal = 0;
	flags = ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass;
	tag = kBypassId;
	parameters.addParameter(STR16("Bypass"), 0, stepCount, defaultVal, flags, tag);

	return result;
}


tresult PLUGIN_API AGainLowPassController::terminate()
{
	viewsArray.removeAll();

	return EditControllerEx1::terminate();
}

tresult PLUGIN_API AGainLowPassController::setComponentState(IBStream* state) //TODO WTF WTF WTF
{
	// we receive the current state of the component (processor part)
	// we read only the gain and bypass value...
	if (state)
	{
		float savedGain = 0.f;
		if (state->read(&savedGain, sizeof(float)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32(savedGain)
#endif
			setParamNormalized(kGainId, savedGain);

		// jump the GainReduction
		state->seek(sizeof(float), IBStream::kIBSeekCur);

		// read the bypass
		int32 bypassState;
		if (state->read(&bypassState, sizeof(bypassState)) == kResultTrue)
		{
#if BYTEORDER == kBigEndian
			SWAP_32(bypassState)
#endif
				setParamNormalized(kBypassId, bypassState ? 1 : 0);
		}



		uint32 savedCutoff = 200;
		if (state->read(&savedCutoff, sizeof(uint32)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32(savedCutoff)
#endif

		setParamNormalized(kCutOffId, savedCutoff);
	}

	return kResultOk;
}

IPlugView* PLUGIN_API AGainLowPassController::createView(const char* name)
{
	if (name && strcmp(name, "editor") == 0)
	{
		AGainLowPassEditorView* view = new AGainLowPassEditorView(this);
		return view;
	}
	return 0;
}

//to se nikdy nevola
tresult PLUGIN_API AGainLowPassController::setState(IBStream * state)
{
	return kResultOk;
}

// to se nikdy nevola
tresult PLUGIN_API AGainLowPassController::getState(IBStream * state)
{
	return kResultOk;
}

tresult PLUGIN_API AGainLowPassController::setParamNormalized(ParamID tag, ParamValue value)
{
	// called from host to update our parameters state
	tresult result = EditControllerEx1::setParamNormalized(tag, value);

	for (int32 i = 0; i < viewsArray.total(); i++)
	{
		if (viewsArray.at(i))
		{
			viewsArray.at(i)->update(tag, value);
		}
	}

	return result;
}

tresult PLUGIN_API AGainLowPassController::getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string)
{
	return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

tresult PLUGIN_API AGainLowPassController::getParamValueByString(ParamID tag, TChar* string, ParamValue& valueNormalized)
{
	return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

void AGainLowPassController::addDependentView(AGainLowPassEditorView* view)
{
	viewsArray.add(view);
}

//------------------------------------------------------------------------
void AGainLowPassController::removeDependentView(AGainLowPassEditorView* view)
{
	for (int32 i = 0; i < viewsArray.total(); i++)
	{
		if (viewsArray.at(i) == view)
		{
			viewsArray.removeAt(i);
			break;
		}
	}
}

//------------------------------------------------------------------------
void AGainLowPassController::editorAttached(EditorView* editor)
{
	AGainLowPassEditorView* view = dynamic_cast<AGainLowPassEditorView*> (editor);
	if (view)
	{
		addDependentView(view);
	}
}

//------------------------------------------------------------------------
void AGainLowPassController::editorRemoved(EditorView* editor)
{
	AGainLowPassEditorView* view = dynamic_cast<AGainLowPassEditorView*> (editor);
	if (view)
	{
		removeDependentView(view);
	}
}

/*tresult PLUGIN_API AGainLowPassController::queryInterface(const char* iid, void** obj)
{
	QUERY_INTERFACE(iid, obj, IMidiMapping::iid, IMidiMapping)
	return EditControllerEx1::queryInterface(iid, obj);
}*/
}
}