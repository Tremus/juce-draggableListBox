#pragma once
#include "JuceHeader.h"
//#include "MyListComponent.h"
#include "MyTableComponent.h"

class MainContentComponent   : public juce::Component
{
public:
    MainContentComponent();
    virtual ~MainContentComponent() = default;
    
    void paint (juce::Graphics&) override;
    void resized() override;
    
private:
    juce::TextButton addBtn;

    MyTableData itemData;
    MyTable table;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
