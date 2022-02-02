/*
  ==============================================================================

    DraggableTable.h
    Created: 2 Feb 2022 11:23:23am
    Author:  tredu

  ==============================================================================
*/

#pragma once
#include "DragContainer.h"


//==============================================================================

class DraggableTableModel
{
public:
    //==============================================================================
    DraggableTableModel() = default;
    virtual ~DraggableTableModel() = default;

    //==============================================================================
    virtual int getNumRows() = 0;
    virtual void swapRows(int sourceRowidx, int targetRowIdx)=0;
    virtual void deleteRow(int rowIdx) {}
    virtual void paintRowBackground (juce::Graphics&,
                                     int rowNumber,
                                     int width, int height,
                                     bool rowIsSelected) = 0;
    virtual void paintCell (juce::Graphics&,
                            int rowNumber,
                            int columnId,
                            int width, int height,
                            bool rowIsSelected) = 0;

    //==============================================================================
    virtual juce::Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                                      juce::Component* existingComponentToUpdate);

    //==============================================================================
    virtual void cellClicked (int rowNumber, int columnId, const juce::MouseEvent&);
    virtual void cellDoubleClicked (int rowNumber, int columnId, const juce::MouseEvent&);
    virtual void backgroundClicked (const juce::MouseEvent&);

    //==============================================================================
    virtual void sortOrderChanged (int newSortColumnId, bool isForwards);

    //==============================================================================
    virtual int getColumnAutoSizeWidth (int columnId);
    virtual juce::String getCellTooltip (int rowNumber, int columnId);

    //==============================================================================
    virtual void selectedRowsChanged (int lastRowSelected);
    virtual void deleteKeyPressed (int lastRowSelected);
    virtual void returnKeyPressed (int lastRowSelected);
    virtual void listWasScrolled();
    virtual juce::var getDragSourceDescription (const juce::SparseSet<int>& currentlySelectedRows);
    
    int dragRowIdx = -1;
    bool draggingOutsideContainer = false;
};


//==============================================================================

class DraggableTable
    : public  juce::ListBox
    , private juce::ListBoxModel
    , public DragContainer
    , public DropTarget
    , private juce::TableHeaderComponent::Listener
{
public:
    DraggableTable (const juce::String& componentName = juce::String(),
                    DraggableTableModel* model = nullptr);

    ~DraggableTable() override;

    //==============================================================================
    void setModel (DraggableTableModel* newModel);

    DraggableTableModel* getModel() const noexcept { return model; }

    //==============================================================================
    juce::TableHeaderComponent& getHeader() const noexcept { return *header; }
    void setHeader (std::unique_ptr<juce::TableHeaderComponent> newHeader);
    void setHeaderHeight (int newHeight);
    int getHeaderHeight() const noexcept;

    //==============================================================================
    void autoSizeColumn (int columnId);
    void autoSizeAllColumns();
    void setAutoSizeMenuOptionShown (bool shouldBeShown) noexcept;
    bool isAutoSizeMenuOptionShown() const noexcept { return autoSizeOptionsShown; }
    juce::Rectangle<int> getCellPosition (int columnId, int rowNumber,
                                    bool relativeToComponentTopLeft) const;
    juce::Component* getCellComponent (int columnId, int rowNumber) const;
    void scrollToEnsureColumnIsOnscreen (int columnId);

    //==============================================================================
    int getNumRows() override;
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override;
    juce::Component* refreshComponentForRow (int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void selectedRowsChanged (int row) override;
    void deleteKeyPressed (int currentSelectedRow) override;
    void returnKeyPressed (int currentSelectedRow) override;
    void backgroundClicked (const juce::MouseEvent&) override;
    void listWasScrolled() override;
    void tableColumnsChanged (juce::TableHeaderComponent*) override;
    void tableColumnsResized (juce::TableHeaderComponent*) override;
    void tableSortOrderChanged (juce::TableHeaderComponent*) override;
    void tableColumnDraggingChanged (juce::TableHeaderComponent*, int) override;
    void resized() override;

    //==============================================================================
    // DropTarget
    bool isInterestedInDragSource(const SourceDetails&) override { return true; }
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails&) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override {}

protected:
    void dragOperationEnded(const DropTarget::SourceDetails& dragSourceDetails) override;

private:
    //==============================================================================
    class Header;
    class RowComp;

    juce::TableHeaderComponent* header = nullptr;
    DraggableTableModel* model;
    int columnIdNowBeingDragged = 0;
    bool autoSizeOptionsShown = true;

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    void updateColumnComponents() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggableTable)
};
