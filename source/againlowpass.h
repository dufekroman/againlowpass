#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace Steinberg {
namespace Vst {

class AGainLowPass : public AudioEffect {
public:
	AGainLowPass::AGainLowPass();
	virtual AGainLowPass::~AGainLowPass() {};
	static FUnknown* createInstance(void* context) {return (IAudioProcessor*)new AGainLowPass;}

	/** Called at first after constructor */
	tresult PLUGIN_API initialize(FUnknown* context);

	/** Called at the end before destructor */
	tresult PLUGIN_API terminate();

	/** Switch the Plug-in on/off */
	tresult PLUGIN_API setActive(TBool state);

	/** Here we go...the process call */
	tresult PLUGIN_API process(ProcessData& data);

	///** Test of a communication channel between controller and component */
	//tresult receiveText(const char* text);

	/** For persistence */
	tresult PLUGIN_API setState(IBStream* state);
	tresult PLUGIN_API getState(IBStream* state);

	/** Will be called before any process call */
	tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup);

	/** Bus arrangement managing: in this example the 'again' will be mono for mono input/output and stereo for other arrangements. */
	tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts);

	/** Asks if a given sample size is supported see \ref SymbolicSampleSizes. */
	tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize);

	/** We want to receive message. */
	//tresult PLUGIN_API notify(IMessage* message);

protected:
	/*template <typename SampleType>
	SampleType processAudio(SampleType** input, SampleType** output, int32 numChannels, int32 sampleFrames, float gain);*/


	float fCutoff;

	float fGain;
	float fGainReduction;
	float fVuPPMOld;

	float tmp0;
	float tmp1;

	int32 currentProcessMode;

	bool  bHalfGain;
	bool  bBypass;
};
} // namespace Vst
} // namespace Steinberg