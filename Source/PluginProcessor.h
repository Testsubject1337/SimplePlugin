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

    //EQ
    
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut,
        Reverb
    };
    
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    using Coefficients = Filter::CoefficientsPtr;
    MonoChain leftChain, rightChain;

    void updatePeakFilter(const ChainSettings& chainSettings);
    
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }


    template<typename ChainType, typename CoefficientType>
    inline void updateCutFilter(ChainType& LowCut, const CoefficientType& cutCoefficients, const Slope& lowCutSlope)
    {
        LowCut.template setBypassed<0>(true);
        LowCut.template setBypassed<1>(true);
        LowCut.template setBypassed<2>(true);
        LowCut.template setBypassed<3>(true);

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


        /* case Slope_12:
         {
             *LowCut.get<0>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<0>(false);
             break;
         }
         case Slope_24:
         {
             *LowCut.template get<0>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<0>(false);
             *LowCut.template get<1>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<1>(false);
             break;
         }

         case Slope_36:
         {
             *LowCut.template get<0>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<0>(false);
             *LowCut.template get<1>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<1>(false);
             *LowCut.template get<2>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<2>(false);
             break;
         }

         case Slope_48:
         {
             *LowCut.template get<0>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<0>(false);
             *LowCut.template get<1>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<1>(false);
             *LowCut.template get<2>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<2>(false);
             *LowCut.template get<3>().coefficients = *cutCoefficients[0];
             LowCut.template setBypassed<3>(false);
             break;
         }*/
        }
    }


    // Reverb
    juce::dsp::Reverb reverb;
    
    void updateReverbParameters();






    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimplePluginAudioProcessor)
};


