/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimplePluginAudioProcessor::SimplePluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimplePluginAudioProcessor::~SimplePluginAudioProcessor()
{
}

//==============================================================================
const juce::String SimplePluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimplePluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimplePluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimplePluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimplePluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimplePluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimplePluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimplePluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimplePluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimplePluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimplePluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    // Prepare Chain
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    // Prepare the Reverb effect
    spec.numChannels = 2; // Assuming stereo input/output
    reverb.prepare(spec);


    auto chainSettings = getChainSettings(apvts);

    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
    case Slope_12:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<1>(false);
        break;
    }
          
    case Slope_36:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<2>(false);
        break;
    }

    case Slope_48:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<3>(false);
        break;
    }
    }

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
    case Slope_12:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<1>(false);
        break;
    }

    case Slope_36:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<2>(false);
        break;
    }

    case Slope_48:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<3>(false);
        break;
    }
    }

}

void SimplePluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimplePluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimplePluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    //Updates
    auto chainSettings = getChainSettings(apvts);

    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(), 2 * (chainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
    case Slope_12:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<1>(false);
        break;
    }

    case Slope_36:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<2>(false);
        break;
    }

    case Slope_48:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *cutCoefficients[0];
        leftLowCut.setBypassed<3>(false);
        break;
    }
    }

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
    case Slope_12:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<1>(false);
        break;
    }

    case Slope_36:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<2>(false);
        break;
    }

    case Slope_48:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *cutCoefficients[0];
        rightLowCut.setBypassed<3>(false);
        break;
    }
    }

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
    

}

//==============================================================================
bool SimplePluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimplePluginAudioProcessor::createEditor()
{

    //return new SimplePluginAudioProcessorEditor (*this);
    return new ::juce::GenericAudioProcessorEditor(*this);

}

//==============================================================================
void SimplePluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimplePluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout SimplePluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout parameterLayout;


    //===============SIMPLE EQ FOR INPUT===================

    //LowCut Frequency
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
        "LowCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f));

    //HighCut Freqency
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
        "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f));

    //Peak Frequency
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
        "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        750.f));

    //Peak Gain
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
        "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
        0.0f));

    //Peak Quality
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
        "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f));

    //String Array for db/Oct
    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    //Slope-Choice
    parameterLayout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    parameterLayout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));


    //===================REVERB===================
    // Mix parameter
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
        "Mix", "Mix", 0.0f, 1.0f, 0.5f));

    // Room size parameter
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
        "RoomSize", "Room Size", 0.0f, 1.0f, 0.5f));

    // Damping parameter
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
        "Damping", "Damping", 0.0f, 1.0f, 0.5f));

    // Pre-delay parameter
    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
        "PreDelay", "Pre-Delay", 0.0f, 100.0f, 0.0f));

    parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
        "Low", "Low", -24.0f, 24.0f, 0.0f));



    //===================BYPASS===================
    parameterLayout.add(std::make_unique<juce::AudioParameterBool>("Input EQ Bypassed", "Input EQ Bypassed", false));
    parameterLayout.add(std::make_unique<juce::AudioParameterBool>("LowCut Bypassed", "LowCut Bypassed", false));
    parameterLayout.add(std::make_unique<juce::AudioParameterBool>("Peak Bypassed", "Peak Bypassed", false));
    parameterLayout.add(std::make_unique<juce::AudioParameterBool>("HighCut Bypassed", "HighCut Bypassed", false));
    parameterLayout.add(std::make_unique<juce::AudioParameterBool>("Analyzer Enabled", "Analyzer Enabled", true));

    return parameterLayout;
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());


    settings.roomSize = apvts.getRawParameterValue("RoomSize")->load();
    settings.damping = apvts.getRawParameterValue("Damping")->load();
    settings.preDelay = apvts.getRawParameterValue("PreDelay")->load();
    settings.low = apvts.getRawParameterValue("Low")->load();
    settings.mix = apvts.getRawParameterValue("Mix")->load();

    return settings;
}



void SimplePluginAudioProcessor::updateReverbParameters()
{
    juce::dsp::Reverb::Parameters params;
    params.roomSize = *apvts.getRawParameterValue("roomSize");
    params.damping = *apvts.getRawParameterValue("damping");
    params.wetLevel = *apvts.getRawParameterValue("wetLevel");
    params.dryLevel = *apvts.getRawParameterValue("dryLevel");
    params.width = 1.0f; // You can adjust the width parameter as needed
    params.freezeMode = 0.0f; // You can set this to 1.0f to enable freeze mode

    reverb.setParameters(params);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimplePluginAudioProcessor();
}

