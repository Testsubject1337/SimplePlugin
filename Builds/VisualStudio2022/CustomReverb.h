#pragma once

#include <JuceHeader.h>


class CustomReverb
{
public:
    CustomReverb();
    ~CustomReverb();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParameters(float roomSize, float damping, float wetLevel, float dryLevel);
    void process(juce::AudioBuffer<float>& buffer);

private:
    // Hier können Sie private Attribute und Methoden für Ihren benutzerdefinierten Reverb implementieren

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomReverb)
};