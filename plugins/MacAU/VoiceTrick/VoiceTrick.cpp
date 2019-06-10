/*
*	File:		VoiceTrick.cpp
*	
*	Version:	1.0
* 
*	Created:	5/9/19
*	
*	Copyright:  Copyright � 2019 Airwindows, All Rights Reserved
* 
*	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc. ("Apple") in 
*				consideration of your agreement to the following terms, and your use, installation, modification 
*				or redistribution of this Apple software constitutes acceptance of these terms.  If you do 
*				not agree with these terms, please do not use, install, modify or redistribute this Apple 
*				software.
*
*				In consideration of your agreement to abide by the following terms, and subject to these terms, 
*				Apple grants you a personal, non-exclusive license, under Apple's copyrights in this 
*				original Apple software (the "Apple Software"), to use, reproduce, modify and redistribute the 
*				Apple Software, with or without modifications, in source and/or binary forms; provided that if you 
*				redistribute the Apple Software in its entirety and without modifications, you must retain this 
*				notice and the following text and disclaimers in all such redistributions of the Apple Software. 
*				Neither the name, trademarks, service marks or logos of Apple Computer, Inc. may be used to 
*				endorse or promote products derived from the Apple Software without specific prior written 
*				permission from Apple.  Except as expressly stated in this notice, no other rights or 
*				licenses, express or implied, are granted by Apple herein, including but not limited to any 
*				patent rights that may be infringed by your derivative works or by other works in which the 
*				Apple Software may be incorporated.
*
*				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO WARRANTIES, EXPRESS OR 
*				IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY 
*				AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE 
*				OR IN COMBINATION WITH YOUR PRODUCTS.
*
*				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL 
*				DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
*				OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, 
*				REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER 
*				UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN 
*				IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
/*=============================================================================
	VoiceTrick.cpp
	
=============================================================================*/
#include "VoiceTrick.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

COMPONENT_ENTRY(VoiceTrick)


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	VoiceTrick::VoiceTrick
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VoiceTrick::VoiceTrick(AudioUnit component)
	: AUEffectBase(component)
{
	CreateElements();
	Globals()->UseIndexedParameters(kNumberOfParameters);
	SetParameter(kParam_One, kDefaultValue_ParamOne );
         
#if AU_DEBUG_DISPATCHER
	mDebugDispatcher = new AUDebugDispatcher (this);
#endif
	
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	VoiceTrick::GetParameterValueStrings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult			VoiceTrick::GetParameterValueStrings(AudioUnitScope		inScope,
                                                                AudioUnitParameterID	inParameterID,
                                                                CFArrayRef *		outStrings)
{
        
    return kAudioUnitErr_InvalidProperty;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	VoiceTrick::GetParameterInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult			VoiceTrick::GetParameterInfo(AudioUnitScope		inScope,
                                                        AudioUnitParameterID	inParameterID,
                                                        AudioUnitParameterInfo	&outParameterInfo )
{
	ComponentResult result = noErr;

	outParameterInfo.flags = 	kAudioUnitParameterFlag_IsWritable
						|		kAudioUnitParameterFlag_IsReadable;
    
    if (inScope == kAudioUnitScope_Global) {
        switch(inParameterID)
        {
           case kParam_One:
                AUBase::FillInParameterName (outParameterInfo, kParameterOneName, false);
                outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
                outParameterInfo.minValue = 0.0;
                outParameterInfo.maxValue = 1.0;
                outParameterInfo.defaultValue = kDefaultValue_ParamOne;
                break;
           default:
                result = kAudioUnitErr_InvalidParameter;
                break;
            }
	} else {
        result = kAudioUnitErr_InvalidParameter;
    }
    


	return result;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	VoiceTrick::GetPropertyInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult			VoiceTrick::GetPropertyInfo (AudioUnitPropertyID	inID,
                                                        AudioUnitScope		inScope,
                                                        AudioUnitElement	inElement,
                                                        UInt32 &		outDataSize,
                                                        Boolean &		outWritable)
{
	return AUEffectBase::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// state that plugin supports only stereo-in/stereo-out processing
UInt32 VoiceTrick::SupportedNumChannels(const AUChannelInfo ** outInfo)
{
	if (outInfo != NULL)
	{
		static AUChannelInfo info;
		info.inChannels = 2;
		info.outChannels = 2;
		*outInfo = &info;
	}

	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	VoiceTrick::GetProperty
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult			VoiceTrick::GetProperty(	AudioUnitPropertyID inID,
                                                        AudioUnitScope 		inScope,
                                                        AudioUnitElement 	inElement,
                                                        void *			outData )
{
	return AUEffectBase::GetProperty (inID, inScope, inElement, outData);
}

//	VoiceTrick::Initialize
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult VoiceTrick::Initialize()
{
    ComponentResult result = AUEffectBase::Initialize();
    if (result == noErr)
        Reset(kAudioUnitScope_Global, 0);
    return result;
}

#pragma mark ____VoiceTrickEffectKernel



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	VoiceTrick::VoiceTrickKernel::Reset()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult		VoiceTrick::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	iirLowpassA = 0.0;
	iirLowpassB = 0.0;
	iirLowpassC = 0.0;
	iirLowpassD = 0.0;
	iirLowpassE = 0.0;
	iirLowpassF = 0.0;
	count = 0;
	lowpassChase = 0.0;
	lowpassAmount = 1.0;
	lastLowpass = 1000.0;
	fpd = 17;
	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	VoiceTrick::ProcessBufferLists
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus		VoiceTrick::ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags,
													const AudioBufferList & inBuffer,
                                                    AudioBufferList & outBuffer,
                                                    UInt32 			inFramesToProcess)
{
	Float32 * inputL = (Float32*)(inBuffer.mBuffers[0].mData);
	Float32 * inputR = (Float32*)(inBuffer.mBuffers[1].mData);
	Float32 * outputL = (Float32*)(outBuffer.mBuffers[0].mData);
	Float32 * outputR = (Float32*)(outBuffer.mBuffers[1].mData);
	UInt32 nSampleFrames = inFramesToProcess;
	
	lowpassChase = pow(GetParameter( kParam_One ),2);
	//should not scale with sample rate, because values reaching 1 are important
	//to its ability to bypass when set to max
	Float64 lowpassSpeed = 300 / (fabs( lastLowpass - lowpassChase)+1.0);
	lastLowpass = lowpassChase;	
	Float64 invLowpass;
	
	while (nSampleFrames-- > 0) {
		long double inputSampleL = *inputL;
		long double inputSampleR = *inputR;
		if (fabs(inputSampleL)<1.18e-37) inputSampleL = fpd * 1.18e-37;
		if (fabs(inputSampleR)<1.18e-37) inputSampleR = fpd * 1.18e-37;
		
		lowpassAmount = (((lowpassAmount*lowpassSpeed)+lowpassChase)/(lowpassSpeed + 1.0)); invLowpass = 1.0 - lowpassAmount;
		//setting chase functionality of Capacitor Lowpass. I could just use this value directly from the control,
		//but if I say it's the lowpass out of Capacitor it should literally be that in every behavior.
				
		long double inputSample = (inputSampleL + inputSampleR) * 0.5;
		//this is now our mono audio
		
		count++; if (count > 5) count = 0; switch (count)
		{
			case 0:
				iirLowpassA = (iirLowpassA * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassA;
				iirLowpassB = (iirLowpassB * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassB;
				iirLowpassD = (iirLowpassD * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassD;
				break;
			case 1:
				iirLowpassA = (iirLowpassA * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassA;
				iirLowpassC = (iirLowpassC * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassC;
				iirLowpassE = (iirLowpassE * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassE;
				break;
			case 2:
				iirLowpassA = (iirLowpassA * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassA;
				iirLowpassB = (iirLowpassB * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassB;
				iirLowpassF = (iirLowpassF * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassF;
				break;
			case 3:
				iirLowpassA = (iirLowpassA * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassA;
				iirLowpassC = (iirLowpassC * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassC;
				iirLowpassD = (iirLowpassD * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassD;
				break;
			case 4:
				iirLowpassA = (iirLowpassA * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassA;
				iirLowpassB = (iirLowpassB * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassB;
				iirLowpassE = (iirLowpassE * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassE;
				break;
			case 5:
				iirLowpassA = (iirLowpassA * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassA;
				iirLowpassC = (iirLowpassC * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassC;
				iirLowpassF = (iirLowpassF * invLowpass) + (inputSample * lowpassAmount); inputSample = iirLowpassF;
				break;
		}
		//Highpass Filter chunk. This is three poles of IIR highpass, with a 'gearbox' that progressively
		//steepens the filter after minimizing artifacts.
		

		inputSampleL = -inputSample;
		inputSampleR = inputSample;
		
		//and now the output is mono, maybe filtered, and phase flipped to cancel at the microphone.
		//The purpose of all this is to allow for recording of lead vocals without use of headphones:
		//or at least sealed headphones. You should be able to use this to record vocals with either
		//open-back headphones, or literally speakers in the room so long as the mic is exactly
		//equidistant from each speaker/headphone side.
		
		//You'll probably want to not use voice monitoring: just mute the track being recorded, or monitor
		//only reverb and echo for vibe. Direct sound is the singer's direct sound.
		
		//The filtering is because, if you use open-back headphones and move your head, highs will
		//bleed through first like a through-zero flange coming out of cancellation (which it is).
		//Therefore, you can filter off highs until the bleed isn't annoying.
		//Or just run with it, it shouldn't be that loud.
		
		//Thanks to Peter Gabriel for many great examples of hit vocals recorded just like this :)
		
		//begin 32 bit stereo floating point dither
		int expon; frexpf((float)inputSampleL, &expon);
		fpd ^= fpd << 13; fpd ^= fpd >> 17; fpd ^= fpd << 5;
		inputSampleL += ((double(fpd)-uint32_t(0x7fffffff)) * 5.5e-36l * pow(2,expon+62));
		frexpf((float)inputSampleR, &expon);
		fpd ^= fpd << 13; fpd ^= fpd >> 17; fpd ^= fpd << 5;
		inputSampleR += ((double(fpd)-uint32_t(0x7fffffff)) * 5.5e-36l * pow(2,expon+62));
		//end 32 bit stereo floating point dither
		
		*outputL = inputSampleL;
		*outputR = inputSampleR;
		//direct stereo out
		
		inputL += 1;
		inputR += 1;
		outputL += 1;
		outputR += 1;
	}
	return noErr;
}
