#define _USE_MATH_DEFINES
#include <cmath>

#include "againlowpass.h"
#include "againcids.h"
#include "againparamids.h"

#include "pluginterfaces/base/ustring.h"	// for UString128
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstpresetkeys.h"
#include "pluginterfaces/vst/ivstevents.h"

namespace Steinberg {
namespace Vst {
AGainLowPass::AGainLowPass() :
	fGain(1.f),
	fGainReduction(0.f),
	fVuPPMOld(0.f),
	currentProcessMode(-1), // -1 means not initialized
	bHalfGain(false),
	bBypass(false),
	fCutoff(0.f),
	tmp0 (0.f),
	tmp1(0.f)
{
	setControllerClass(AGainLowPassControllerUID);
}

tresult PLUGIN_API AGainLowPass::initialize(FUnknown* context)
{
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize(context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//---create Audio In/Out buses------
	// we want a stereo Input and a Stereo Output
	addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
	addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);

	//---create Event In/Out buses (1 bus with only 1 channel)------
	addEventInput(STR16("Event In"), 1);

	return kResultOk;
}

tresult PLUGIN_API AGainLowPass::terminate()
{
	// nothing to do here yet...except calling our parent terminate
	return AudioEffect::terminate();
}

tresult PLUGIN_API AGainLowPass::setActive(TBool state)
{
	// reset the VuMeter value
	fVuPPMOld = 0.f;

	// call our parent setActive
	return AudioEffect::setActive(state);
}

tresult PLUGIN_API AGainLowPass::setState(IBStream* state)
{
	// called when we load a preset, the model has to be reloaded

	float savedGain = 0.f;
	if (state->read(&savedGain, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}

	float savedGainReduction = 0.f;
	if (state->read(&savedGainReduction, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}

	int32 savedBypass = 0;
	if (state->read(&savedBypass, sizeof(int32)) != kResultOk)
	{
		return kResultFalse;
	}

	float savedCutoff = 0.f;
	if (state->read(&savedCutoff, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}


#if BYTEORDER == kBigEndian
	SWAP_32(savedGain)
		SWAP_32(savedGainReduction)
		SWAP_32(savedBypass)
#endif

	fGain = savedGain;
	fGainReduction = savedGainReduction;
	bBypass = savedBypass > 0;
	fCutoff = savedCutoff;
	

	return kResultOk;
}

tresult PLUGIN_API AGainLowPass::getState(IBStream* state)
{
	// here we need to save the model

	float toSaveGain = fGain;
	float toSaveGainReduction = fGainReduction;
	int32 toSaveBypass = bBypass ? 1 : 0;
	float toSaveCutoff = fCutoff;

#if BYTEORDER == kBigEndian
	SWAP_32(toSaveGain)
		SWAP_32(toSaveGainReduction)
		SWAP_32(toSaveBypass)
		SWAP_32(toSaveCutoff)
#endif

		state->write(&toSaveGain, sizeof(float));
	state->write(&toSaveGainReduction, sizeof(float));
	state->write(&toSaveBypass, sizeof(int32));
	state->write(&toSaveCutoff, sizeof(float));
	return kResultOk;
}

tresult PLUGIN_API AGainLowPass::setupProcessing(ProcessSetup& newSetup)
{
	// called before the process call, always in a disable state (not active)

	// here we keep a trace of the processing mode (offline,...) for example.
	currentProcessMode = newSetup.processMode;

	return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API AGainLowPass::setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
	if (numIns == 1 && numOuts == 1)
	{
		// the host wants Mono => Mono (or 1 channel -> 1 channel)
		if (SpeakerArr::getChannelCount(inputs[0]) == 1 && SpeakerArr::getChannelCount(outputs[0]) == 1)
		{
			AudioBus* bus = FCast<AudioBus>(audioInputs.at(0));
			if (bus)
			{
				// check if we are Mono => Mono, if not we need to recreate the buses
				if (bus->getArrangement() != inputs[0])
				{
					removeAudioBusses();
					addAudioInput(STR16("Mono In"), inputs[0]);
					addAudioOutput(STR16("Mono Out"), inputs[0]);
				}
				return kResultOk;
			}
		}
		// the host wants something else than Mono => Mono, in this case we are always Stereo => Stereo
		else
		{
			AudioBus* bus = FCast<AudioBus>(audioInputs.at(0));
			if (bus)
			{
				tresult result = kResultFalse;

				// the host wants 2->2 (could be LsRs -> LsRs)
				if (SpeakerArr::getChannelCount(inputs[0]) == 2 && SpeakerArr::getChannelCount(outputs[0]) == 2)
				{
					removeAudioBusses();
					addAudioInput(STR16("Stereo In"), inputs[0]);
					addAudioOutput(STR16("Stereo Out"), outputs[0]);
					result = kResultTrue;
				}
				// the host want something different than 1->1 or 2->2 : in this case we want stereo
				else if (bus->getArrangement() != SpeakerArr::kStereo)
				{
					removeAudioBusses();
					addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
					addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
					result = kResultFalse;
				}

				return result;
			}
		}
	}
	return kResultFalse;
}

tresult PLUGIN_API AGainLowPass::canProcessSampleSize(int32 symbolicSampleSize)
{
	if (symbolicSampleSize == kSample32)
		return kResultTrue;

	// we support double processing
	if (symbolicSampleSize == kSample64)
		return kResultTrue;

	return kResultFalse;
}

tresult PLUGIN_API AGainLowPass::process(ProcessData& data)
{
	// finally the process function
	// In this example there are 4 steps:
	// 1) Read inputs parameters coming from host (in order to adapt our model values)
	// 2) Read inputs events coming from host (we apply a gain reduction depending of the velocity of pressed key)
	// 3) Process the gain of the input buffer to the output buffer
	// 4) Write the new VUmeter value to the output Parameters queue


	//---1) Read inputs parameter changes-----------
	IParameterChanges* paramChanges = data.inputParameterChanges;
	if (paramChanges)
	{
		int32 numParamsChanged = paramChanges->getParameterCount();
		// for each parameter which are some changes in this audio block:
		for (int32 i = 0; i < numParamsChanged; i++)
		{
			IParamValueQueue* paramQueue = paramChanges->getParameterData(i);
			if (paramQueue)
			{
				ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount();
				switch (paramQueue->getParameterId())
				{
				case kGainId:
					// we use in this example only the last point of the queue.
					// in some wanted case for specific kind of parameter it makes sense to retrieve all points
					// and process the whole audio block in small blocks.
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
					{
						fGain = (float)value;
					}
					break;
				case kCutOffId:
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
					{
						fCutoff = (float)value;
					}
					break;


				case kBypassId:
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
					{
						bBypass = (value > 0.5f);
					}
					break;
				}
			}
		}
	}

	//-------------------------------------
	//---3) Process Audio---------------------
	//-------------------------------------
	if (data.numInputs == 0 || data.numOutputs == 0)
	{
		// nothing to do
		return kResultOk;
	}

	// (simplification) we suppose in this example that we have the same input channel count than the output
	int32 numChannels = data.inputs[0].numChannels;

	//---get audio buffers----------------
	uint32 sampleFramesSize = getSampleFramesSizeInBytes(data.numSamples);
	void** in = getChannelBuffersPointer(data.inputs[0]);
	void** out = getChannelBuffersPointer(data.outputs[0]);

	//---check if silence---------------
	// normally we have to check each channel (simplification)
	if (data.inputs[0].silenceFlags != 0)
	{
		// mark output silence too
		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		// the Plug-in has to be sure that if it sets the flags silence that the output buffer are clear
		for (int32 i = 0; i < numChannels; i++)
		{
			// dont need to be cleared if the buffers are the same (in this case input buffer are already cleared by the host)
			if (in[i] != out[i])
			{
				memset(out[i], 0, sampleFramesSize);
			}
		}

		// nothing to do at this point
		return kResultOk;
	}

	// mark our outputs has not silent
	data.outputs[0].silenceFlags = 0;

	//---in bypass mode outputs should be like inputs-----
	if (bBypass)
	{
		for (int32 i = 0; i < numChannels; i++)
		{
			// dont need to be copied if the buffers are the same
			if (in[i] != out[i])
			{
				memcpy(out[i], in[i], sampleFramesSize);
			}
		}
		// in this example we do not update the VuMeter in Bypass
	}
	else
	{
		float fVuPPM = 0.f;

		//---apply gain factor----------
		float gain = (fGain - (fGainReduction/2));

		// if the applied gain is nearly zero, we could say that the outputs are zeroed and we set the silence flags. 
		if (gain < 0.0000001)
		{
			for (int32 i = 0; i < numChannels; i++)
			{
				memset(out[i], 0, sampleFramesSize);
			}
			data.outputs[0].silenceFlags = (1 << numChannels) - 1;  // this will set to 1 all channels
		}
		else
		{
			// pocitam s tim, ze jsou jen 2 kanaly
			int32 sampleFrames = data.numSamples;
			float* ptrIn1 = (float*)in[0];
			float* ptrOut1 = (float*)out[0];
			float* ptrIn2 = (float*)in[1];
			float* ptrOut2 = (float*)out[1];
			float plainCutoff = (5000.f - 200.f) * fCutoff + 200.f;
			float x = exp(-2.0 * M_PI * plainCutoff / 44100);
			float a0 = 1.0 - x;
			float b1 = -x;

			while (--sampleFrames >= 0)
			{
				tmp0 = (a0*(*ptrIn1++) - b1*tmp0) * gain;
				(*ptrOut1++) = tmp0;

				tmp1 = (a0*(*ptrIn2++) - b1*tmp1) * gain;
				(*ptrOut2++) = tmp1;

				if (tmp1 > fVuPPM)
				{
					fVuPPM = tmp1;
				}
			}


		}

		//---3) Write outputs parameter changes-----------
		IParameterChanges* outParamChanges = data.outputParameterChanges;
		// a new value of VuMeter will be send to the host 
		// (the host will send it back in sync to our controller for updating our editor)
		if (outParamChanges && fVuPPMOld != fVuPPM)
		{
			int32 index = 0;
			IParamValueQueue* paramQueue = outParamChanges->addParameterData(kVuPPMId, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fVuPPM, index2);
			}
		}
		fVuPPMOld = fVuPPM;
	}

	return kResultOk;
}

/* musel bych do sablony davat jako argumenty reference na temp promenne, tak pro zjednoduseni to dam primo do tela process
//------------------------------------------------------------------------
template <typename SampleType>
SampleType AGainLowPass::processAudio(SampleType** in, SampleType** out, int32 numChannels, int32 sampleFrames, float gain)
{
	...
}*/
}
}