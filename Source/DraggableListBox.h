#pragma once
#include "JuceHeader.h"
#include "DragContainer.h"

class DraggableListBoxItem;

//==============================================================================

// Base class for holding all relevant row data
class DraggableListBoxItemData
{
public:
    virtual ~DraggableListBoxItemData() {};
    virtual void paintRow(DraggableListBoxItem*, int, juce::Graphics&, juce::Rectangle<int>) = 0;
    virtual int size() = 0;
    virtual void swapRows(int sourceRowidx, int targetRowIdx)=0;
    virtual void deleteRow(int idx) {};

    int dragRowIdx = -1;
    bool draggingOutsideContainer = false;
};


//==============================================================================


class DraggableListBox
    : public juce::ListBox
    , public DragContainer
    , public DropTarget
{
public:
    DraggableListBox(DraggableListBoxItemData& md): modelData(md) {}

    // DropTarget
    bool isInterestedInDragSource(const SourceDetails&) override { return true; }
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails&) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override {}

protected:
    void dragOperationEnded(const DropTarget::SourceDetails& dragSourceDetails) override;

    DraggableListBoxItemData& modelData;
};


//==============================================================================


class DraggableListBoxModel : public juce::ListBoxModel
{
public:
    DraggableListBoxModel(DraggableListBoxItemData& md): modelData(md) {}
    int getNumRows() override { return modelData.size(); }
    void paintListBoxItem(int, Graphics&, int, int, bool) override {}
    Component* refreshComponentForRow(int, bool, Component*) override;

protected:
    DraggableListBoxItemData& modelData;
};


//==============================================================================


// Row component class that knows how to drag itself.
class DraggableListBoxItem : public juce::Component
{
public:
    DraggableListBoxItem(DraggableListBoxItemData& md, int rn)
        : rowIdx(rn)
        , modelData(md) {}

    // It's recommended that you use the paint method in DraggableListBoxItemData
    // as that class should have all your data for all rows
    void paint(juce::Graphics& g) override
    {
        modelData.paintRow(this, rowIdx, g, getLocalBounds());
    }
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

    int rowIdx;
protected:
    DraggableListBoxItemData& modelData;
    juce::MouseCursor savedCursor;
};
