#pragma once
#include "DraggableListBox.h"


//==============================================================================

class MyListBoxItemData : public DraggableListBoxItemData
{
public:
    MyListBoxItemData() {}
    ~MyListBoxItemData() {};

    int size() override { return rowIds.size(); }
    void swapRows(int sourceRowidx, int targetRowIdx) override { rowIds.swap(sourceRowidx, targetRowIdx); }

    void paintRow(DraggableListBoxItem* item, int rowNumber, Graphics& g, Rectangle<int> bounds) override
    {
        // If row is being dragged, paint opaque image
        if (dragRowIdx == rowNumber)
        {
            g.fillAll(juce::Colours::transparentBlack);
            return;
        }

        g.fillAll(Colours::lightgrey);
        g.setColour(Colours::black);
        g.drawRect(bounds);
        g.drawText("Item: " + juce::String(rowIds[rowNumber]), bounds, Justification::centred);
    }

    void deleteRow(int idx) override { rowIds.remove(idx); };
    void addItemAtEnd() { rowIds.add(idCounter); idCounter++; };

    // Not required, just something I'm adding for confirmation of correct order after DnD.
    // This is an example of an operation on the entire list.
    /*
    void printItemsInOrder()
    {
        String msg = "items: ";
        for (int i = 0; i < rowIds.size(); i++)
            msg << rowIds.getUnchecked(i) << " ";
        DBG(msg);
    }
    */

    juce::Array<int> rowIds;

private:
    int idCounter = 0;
};


//==============================================================================


class MyListBox : public DraggableListBox
{
public:
    MyListBox(MyListBoxItemData& data) : DraggableListBox(data) {}

    void dragImageMove(juce::Point<int>& defaultTopLeft) override
    {
        defaultTopLeft.x = 0;
        if (defaultTopLeft.y < 0)
            defaultTopLeft.y = 0;
    }
};
