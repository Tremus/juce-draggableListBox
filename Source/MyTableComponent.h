/*
  ==============================================================================

    MyTableComponent.h
    Created: 2 Feb 2022 1:14:49pm
    Author:  tredu

  ==============================================================================
*/

#pragma once
#include "DraggableTable.h"


//==============================================================================

class MyTableData : public DraggableTableModel
{
public:
    MyTableData() {}
    ~MyTableData() {};

    int getNumRows() override { return rowIds.size(); }
    void swapRows(int sourceRowidx, int targetRowIdx) override { rowIds.swap(sourceRowidx, targetRowIdx); }

    void paintRowBackground(juce::Graphics& g,
        int rowNumber,
        int width, int height,
        bool rowIsSelected) override
    {
        // If row is being dragged, paint opaque image
        if (dragRowIdx == rowNumber)
        {
            g.fillAll(juce::Colours::transparentBlack);
            return;
        }

        g.fillAll(juce::Colours::lightgrey);
        g.setColour(juce::Colours::black);
        g.drawRect(0, 0, width, height);
    }

    void paintCell(juce::Graphics& g,
        int rowNumber,
        int columnId,
        int width, int height,
        bool rowIsSelected) override
    {
        if (dragRowIdx == rowNumber)
        {
            g.fillAll(juce::Colours::transparentBlack);
            return;
        }
        auto text = "Item: " + juce::String(rowIds[rowNumber]) + ":" + juce::String(columnId);
        g.drawText(text, 0, 0, width, height, juce::Justification::centred);
    }

    // OPTIONAL. See item 5. in the README
    void deleteRow(int idx) override { rowIds.remove(idx); };

    void addItemAtEnd()
    {
        rowIds.add(idCounter);
        idCounter++;
    };

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
    // All ids are unique
    int idCounter = 0;
};


//==============================================================================


class MyTable : public DraggableTable
{
public:
    MyTable(MyTableData& data) : DraggableTable("", &data) {}

    // OPTIONAL. See item 6. in the README
    void dragImageMove(juce::Point<int>& defaultTopLeft) override
    {
        // Constrains the image horizontally
        defaultTopLeft.x = 0;

        // Constrains the image vertically when user drags row up
        if (defaultTopLeft.y < 0)
            defaultTopLeft.y = 0;

        int rowHeight = getRowHeight();
        int bottom = defaultTopLeft.y + rowHeight;
        // Constrains the image vertically when user drags row down
        if (bottom > getHeight())
            defaultTopLeft.y = getHeight() - rowHeight;
    }
};
