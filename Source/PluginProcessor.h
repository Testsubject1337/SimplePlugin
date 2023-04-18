/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    //EQ
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };

    bool inputEqBypassed{ false };
    bool lowCutBypassed{ false };
    bool peakBypassed{ false };
    bool HighCutBypassed{ false };

    //Reverb
    float mix{ 1.f };
    float roomSize{ 0.5f };
    float damping{ 0.5f };
    float preDelay{ 1.f };
    float low{ 0 };

    bool AnalyzerEnabled { true };
    

};



ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class SimplePluginAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimplePluginAudioProcessor();
    ~SimplePluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Paramaters", createParameterLayout() };

    


private:

    // =================== DSP UNITS ===================

    // =======EQ=======
    // Enum to represent the positions of different processing stages in the chain
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut,
        Reverb
    };
    
    // Define Filter and ProcessorChain types for easier use
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    using Coefficients = Filter::CoefficientsPtr;

    // Create left and right processing chains
    MonoChain leftChain, rightChain;

    // Function to update the peak filter with new chain settings
    void updatePeakFilter(const ChainSettings& chainSettings);
    
    // Function to update filter coefficients
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

    // Template function to update a specific filter stage in the chain
    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }

    // Template function to update the low-cut filter stages based on the slope
    template<typename ChainType, typename CoefficientType>
    inline void updateCutFilter(ChainType& LowCut, const CoefficientType& cutCoefficients, const Slope& lowCutSlope)
    {
        // Initially bypass all filter stages
        LowCut.template setBypassed<0>(true);
        LowCut.template setBypassed<1>(true);
        LowCut.template setBypassed<2>(true);
        LowCut.template setBypassed<3>(true);

        // Update the filter stages based on the low-cut slope
        switch (lowCutSlope)
        {
            case Slope_48:
            {
                update<3>(LowCut, cutCoefficients);
            }
            case Slope_36:
            {
                update<2>(LowCut, cutCoefficients);
            }
            case Slope_24:
            {
                update<1>(LowCut, cutCoefficients);
            }
            case Slope_12:
            {
                update<0>(LowCut, cutCoefficients);
            }
        }
    }


    //=======Reverb=======
    juce::dsp::Reverb reverb;

    void updateReverbParameters(const ChainSettings& chainSettings);


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimplePluginAudioProcessor)
};


