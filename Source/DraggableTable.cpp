/*
  ==============================================================================

    DraggableTable.cpp
    Created: 2 Feb 2022 11:23:23am
    Author:  tredu

  ==============================================================================
*/

#include "DraggableTable.h"


template <typename RowComponentType>
static juce::AccessibilityActions getListRowAccessibilityActions(RowComponentType& rowComponent)
{
    auto onFocus = [&rowComponent]
    {
        rowComponent.owner.scrollToEnsureRowIsOnscreen(rowComponent.row);
        rowComponent.owner.selectRow(rowComponent.row);
    };

    auto onPress = [&rowComponent, onFocus]
    {
        onFocus();
        rowComponent.owner.keyPressed(juce::KeyPress(juce::KeyPress::returnKey));
    };

    auto onToggle = [&rowComponent]
    {
        rowComponent.owner.flipRowSelection(rowComponent.row);
    };

    return juce::AccessibilityActions().addAction(juce::AccessibilityActionType::focus, std::move(onFocus))
        .addAction(juce::AccessibilityActionType::press, std::move(onPress))
        .addAction(juce::AccessibilityActionType::toggle, std::move(onToggle));
}

class DraggableTable::RowComp
    : public juce::Component
    , public juce::TooltipClient
{
public:
    RowComp (DraggableTable& tlb) noexcept
        : owner (tlb)
    {
        setFocusContainerType (FocusContainerType::focusContainer);
    }

    void paint (juce::Graphics& g) override
    {
        if (auto* tableModel = owner.getModel())
        {
            tableModel->paintRowBackground (g, row, getWidth(), getHeight(), isSelected);

            auto& headerComp = owner.getHeader();
            auto numColumns = headerComp.getNumColumns (true);
            auto clipBounds = g.getClipBounds();

            for (int i = 0; i < numColumns; ++i)
            {
                if (columnComponents[i] == nullptr)
                {
                    auto columnRect = headerComp.getColumnPosition (i).withHeight (getHeight());

                    if (columnRect.getX() >= clipBounds.getRight())
                        break;

                    if (columnRect.getRight() > clipBounds.getX())
                    {
                        juce::Graphics::ScopedSaveState ss (g);

                        if (g.reduceClipRegion (columnRect))
                        {
                            g.setOrigin (columnRect.getX(), 0);
                            tableModel->paintCell (g, row, headerComp.getColumnIdOfIndex (i, true),
                                                   columnRect.getWidth(), columnRect.getHeight(), isSelected);
                        }
                    }
                }
            }
        }
    }

    void update (int newRow, bool isNowSelected)
    {
        jassert (newRow >= 0);

        if (newRow != row || isNowSelected != isSelected)
        {
            row = newRow;
            isSelected = isNowSelected;
            repaint();
        }

        auto* tableModel = owner.getModel();

        if (tableModel != nullptr && row < owner.getNumRows())
        {
            const juce::Identifier columnProperty ("_tableColumnId");
            auto numColumns = owner.getHeader().getNumColumns (true);

            for (int i = 0; i < numColumns; ++i)
            {
                auto columnId = owner.getHeader().getColumnIdOfIndex (i, true);
                auto* comp = columnComponents[i];

                if (comp != nullptr && columnId != static_cast<int> (comp->getProperties() [columnProperty]))
                {
                    columnComponents.set (i, nullptr);
                    comp = nullptr;
                }

                comp = tableModel->refreshComponentForCell (row, columnId, isSelected, comp);
                columnComponents.set (i, comp, false);

                if (comp != nullptr)
                {
                    comp->getProperties().set (columnProperty, columnId);

                    addAndMakeVisible (comp);
                    resizeCustomComp (i);
                }
            }

            columnComponents.removeRange (numColumns, columnComponents.size());
        }
        else
        {
            columnComponents.clear();
        }
    }

    void resized() override
    {
        for (int i = columnComponents.size(); --i >= 0;)
            resizeCustomComp (i);
    }

    void resizeCustomComp (int index)
    {
        if (auto* c = columnComponents.getUnchecked (index))
            c->setBounds (owner.getHeader().getColumnPosition (index)
                            .withY (0).withHeight (getHeight()));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        isDragging = false;
        selectRowOnMouseUp = false;

        if (isEnabled())
        {
            if (! isSelected)
            {
                owner.selectRowsBasedOnModifierKeys (row, e.mods, false);

                auto columnId = owner.getHeader().getColumnIdAtX (e.x);

                if (columnId != 0)
                    if (auto* m = owner.getModel())
                        m->cellClicked (row, columnId, e);
            }
            else
            {
                selectRowOnMouseUp = true;
            }
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isEnabled()
             && owner.getModel() != nullptr
             && e.mouseWasDraggedSinceMouseDown()
             && ! isDragging)
        {
            juce::SparseSet<int> rowsToDrag;

            if (owner.isRowSelected (row))
                rowsToDrag = owner.getSelectedRows();
            else
                rowsToDrag.addRange (juce::Range<int>::withStartAndLength (row, 1));

            if (rowsToDrag.size() > 0)
            {
                auto dragDescription = owner.getModel()->getDragSourceDescription (rowsToDrag);

                if (! (dragDescription.isVoid() || (dragDescription.isString() && dragDescription.toString().isEmpty())))
                {
                    isDragging = true;
//                    owner.startDragAndDrop (e, rowsToDrag, dragDescription, true);
                    if (DragContainer* container = DragContainer::findParentDragContainerFor(this))
                    {
                        if (!container->isDragAndDropActive())
                        {
                            juce::ScaledImage scaledImg (createComponentSnapshot(getLocalBounds()));
                            container->startDragging(row, this, scaledImg);
                            owner.getModel()->dragRowIdx = row;
                        }
                    }
                }
            }
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (selectRowOnMouseUp && e.mouseWasClicked() && isEnabled())
        {
            owner.selectRowsBasedOnModifierKeys (row, e.mods, true);

            auto columnId = owner.getHeader().getColumnIdAtX (e.x);

            if (columnId != 0)
                if (DraggableTableModel* m = owner.getModel())
                    m->cellClicked (row, columnId, e);
        }
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        auto columnId = owner.getHeader().getColumnIdAtX (e.x);

        if (columnId != 0)
            if (auto* m = owner.getModel())
                m->cellDoubleClicked (row, columnId, e);
    }

    juce::String getTooltip() override
    {
        auto columnId = owner.getHeader().getColumnIdAtX (getMouseXYRelative().getX());

        if (columnId != 0)
            if (auto* m = owner.getModel())
                return m->getCellTooltip (row, columnId);

        return {};
    }

    Component* findChildComponentForColumn (int columnId) const
    {
        return columnComponents [owner.getHeader().getIndexOfColumnId (columnId, true)];
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<RowAccessibilityHandler> (*this);
    }

    //==============================================================================
    class RowAccessibilityHandler  : public juce::AccessibilityHandler
    {
    public:
        RowAccessibilityHandler (RowComp& rowComp)
            : AccessibilityHandler (rowComp,
                                    juce::AccessibilityRole::row,
                                    getListRowAccessibilityActions (rowComp),
                                    { std::make_unique<RowComponentCellInterface> (*this) }),
              rowComponent (rowComp)
        {
        }

        juce::String getTitle() const override
        {
            if (auto* m = rowComponent.owner.ListBox::getModel())
                return m->getNameForRow (rowComponent.row);

            return {};
        }

        juce::String getHelp() const override  { return rowComponent.getTooltip(); }

        juce::AccessibleState getCurrentState() const override
        {
            if (auto* m = rowComponent.owner.getModel())
                if (rowComponent.row >= m->getNumRows())
                    return juce::AccessibleState().withIgnored();

            auto state = AccessibilityHandler::getCurrentState();

            state = state.withSelectable();

            if (rowComponent.isSelected)
                return state.withSelected();

            return state;
        }

        class RowComponentCellInterface  : public juce::AccessibilityCellInterface
        {
        public:
            RowComponentCellInterface (RowAccessibilityHandler& handler)
                : owner (handler)
            {
            }

            int getColumnIndex() const override      { return 0; }
            int getColumnSpan() const override       { return 1; }

            int getRowIndex() const override         { return owner.rowComponent.row; }
            int getRowSpan() const override          { return 1; }

            int getDisclosureLevel() const override  { return 0; }

            const AccessibilityHandler* getTableHandler() const override  { return owner.rowComponent.owner.getAccessibilityHandler(); }

        private:
            RowAccessibilityHandler& owner;
        };

    private:
        RowComp& rowComponent;
    };

    //==============================================================================
    DraggableTable& owner;
    juce::OwnedArray<juce::Component> columnComponents;
    int row = -1;
    bool isSelected = false, isDragging = false, selectRowOnMouseUp = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RowComp)
};

//==============================================================================
class DraggableTable::Header  : public juce::TableHeaderComponent
{
public:
    Header (DraggableTable& tlb)  : owner (tlb) {}

    void addMenuItems (juce::PopupMenu& menu, int columnIdClicked)
    {
        if (owner.isAutoSizeMenuOptionShown())
        {
            menu.addItem (autoSizeColumnId, TRANS("Auto-size this column"), columnIdClicked != 0);
            menu.addItem (autoSizeAllId, TRANS("Auto-size all columns"), owner.getHeader().getNumColumns (true) > 0);
            menu.addSeparator();
        }

        TableHeaderComponent::addMenuItems (menu, columnIdClicked);
    }

    void reactToMenuItem (int menuReturnId, int columnIdClicked)
    {
        switch (menuReturnId)
        {
            case autoSizeColumnId:      owner.autoSizeColumn (columnIdClicked); break;
            case autoSizeAllId:         owner.autoSizeAllColumns(); break;
            default:                    TableHeaderComponent::reactToMenuItem (menuReturnId, columnIdClicked); break;
        }
    }

private:
    DraggableTable& owner;

    enum { autoSizeColumnId = 0xf836743, autoSizeAllId = 0xf836744 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};

//==============================================================================
DraggableTable::DraggableTable (const juce::String& name, DraggableTableModel* const m)
    : juce::ListBox (name, nullptr), model (m)
{
    ListBox::setModel(this);
    setHeader (std::make_unique<Header> (*this));
}

DraggableTable::~DraggableTable()
{
}

void DraggableTable::setModel(DraggableTableModel* newModel)
{
    if (model != newModel)
    {
        model = newModel;
        updateContent();
    }
}

void DraggableTable::setHeader (std::unique_ptr<juce::TableHeaderComponent> newHeader)
{
    if (newHeader == nullptr)
    {
        jassertfalse; // you need to supply a real header for a table!
        return;
    }

    juce::Rectangle<int> newBounds (100, 28);

    if (header != nullptr)
        newBounds = header->getBounds();

    header = newHeader.get();
    header->setBounds (newBounds);

    setHeaderComponent (std::move (newHeader));

    header->addListener (this);
}

int DraggableTable::getHeaderHeight() const noexcept
{
    return header->getHeight();
}

void DraggableTable::setHeaderHeight (int newHeight)
{
    header->setSize (header->getWidth(), newHeight);
    //resized();
}

void DraggableTable::autoSizeColumn (int columnId)
{
    auto width = model != nullptr ? model->getColumnAutoSizeWidth (columnId) : 0;

    if (width > 0)
        header->setColumnWidth (columnId, width);
}

void DraggableTable::autoSizeAllColumns()
{
    for (int i = 0; i < header->getNumColumns (true); ++i)
        autoSizeColumn (header->getColumnIdOfIndex (i, true));
}

void DraggableTable::setAutoSizeMenuOptionShown (bool shouldBeShown) noexcept
{
    autoSizeOptionsShown = shouldBeShown;
}

juce::Rectangle<int> DraggableTable::getCellPosition (int columnId, int rowNumber, bool relativeToComponentTopLeft) const
{
    auto headerCell = header->getColumnPosition (header->getIndexOfColumnId (columnId, true));

    if (relativeToComponentTopLeft)
        headerCell.translate (header->getX(), 0);

    return getRowPosition (rowNumber, relativeToComponentTopLeft)
            .withX (headerCell.getX())
            .withWidth (headerCell.getWidth());
}

juce::Component* DraggableTable::getCellComponent (int columnId, int rowNumber) const
{
    if (auto* rowComp = dynamic_cast<RowComp*> (getComponentForRowNumber (rowNumber)))
        return rowComp->findChildComponentForColumn (columnId);

    return nullptr;
}

void DraggableTable::scrollToEnsureColumnIsOnscreen (int columnId)
{
    auto& scrollbar = getHorizontalScrollBar();
    auto pos = header->getColumnPosition (header->getIndexOfColumnId (columnId, true));

    auto x = scrollbar.getCurrentRangeStart();
    auto w = scrollbar.getCurrentRangeSize();

    if (pos.getX() < x)
        x = pos.getX();
    else if (pos.getRight() > x + w)
        x += juce::jmax (0.0, pos.getRight() - (x + w));

    scrollbar.setCurrentRangeStart (x);
}

int DraggableTable::getNumRows()
{
    return model != nullptr ? model->getNumRows() : 0;
}

void DraggableTable::paintListBoxItem (int, juce::Graphics&, int, int, bool)
{
}

juce::Component* DraggableTable::refreshComponentForRow (int rowNumber, bool rowSelected, Component* existingComponentToUpdate)
{
    if (existingComponentToUpdate == nullptr)
        existingComponentToUpdate = new RowComp (*this);

    static_cast<RowComp*> (existingComponentToUpdate)->update (rowNumber, rowSelected);

    return existingComponentToUpdate;
}

void DraggableTable::selectedRowsChanged (int row)
{
    if (model != nullptr)
        model->selectedRowsChanged (row);
}

void DraggableTable::deleteKeyPressed (int row)
{
    if (model != nullptr)
        model->deleteKeyPressed (row);
}

void DraggableTable::returnKeyPressed (int row)
{
    if (model != nullptr)
        model->returnKeyPressed (row);
}

void DraggableTable::backgroundClicked (const juce::MouseEvent& e)
{
    if (model != nullptr)
        model->backgroundClicked (e);
}

void DraggableTable::listWasScrolled()
{
    if (model != nullptr)
        model->listWasScrolled();
}

void DraggableTable::tableColumnsChanged (juce::TableHeaderComponent*)
{
    setMinimumContentWidth (header->getTotalWidth());
    repaint();
    updateColumnComponents();
}

void DraggableTable::tableColumnsResized (juce::TableHeaderComponent*)
{
    setMinimumContentWidth (header->getTotalWidth());
    repaint();
    updateColumnComponents();
}

void DraggableTable::tableSortOrderChanged (juce::TableHeaderComponent*)
{
    if (model != nullptr)
        model->sortOrderChanged (header->getSortColumnId(),
                                 header->isSortedForwards());
}

void DraggableTable::tableColumnDraggingChanged (juce::TableHeaderComponent*, int columnIdNowBeingDragged_)
{
    columnIdNowBeingDragged = columnIdNowBeingDragged_;
    repaint();
}

void DraggableTable::resized()
{
    jassert(ListBox::getModel() != nullptr);
    ListBox::resized();

    header->resizeAllColumnsToFit (getVisibleContentWidth());
    setMinimumContentWidth (header->getTotalWidth());
}

void DraggableTable::updateColumnComponents() const
{
    auto firstRow = getRowContainingPosition (0, 0);

    for (int i = firstRow + getNumRowsOnScreen() + 2; --i >= firstRow;)
        if (auto* rowComp = dynamic_cast<RowComp*> (getComponentForRowNumber (i)))
            rowComp->resized();
}

std::unique_ptr<juce::AccessibilityHandler> DraggableTable::createAccessibilityHandler()
{
    class TableInterface : public juce::AccessibilityTableInterface
    {
    public:
        TableInterface(DraggableTable& tableListBoxToWrap)
            : tableListBox(tableListBoxToWrap)
        {
        }

        int getNumRows() const override
        {
            if (auto* tableModel = tableListBox.getModel())
                return tableModel->getNumRows();
            jassertfalse;
            return 0;
        }

        int getNumColumns() const override
        {
            return tableListBox.getHeader().getNumColumns(false);
        }

        const juce::AccessibilityHandler* getCellHandler(int row, int column) const override
        {
            if (juce::isPositiveAndBelow(row, getNumRows()))
            {
                if (juce::isPositiveAndBelow(column, getNumColumns()))
                    if (auto* cellComponent = tableListBox.getCellComponent(tableListBox.getHeader().getColumnIdOfIndex(column, false), row))
                        return cellComponent->getAccessibilityHandler();

                if (auto* rowComp = tableListBox.getComponentForRowNumber(row))
                    return rowComp->getAccessibilityHandler();
            }

            return nullptr;
        }

    private:
        DraggableTable& tableListBox;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableInterface)
    };

    return std::make_unique<juce::AccessibilityHandler>(*this,
        juce::AccessibilityRole::list,
        juce::AccessibilityActions{},
        juce::AccessibilityHandler::Interfaces{ std::make_unique<TableInterface>(*this) });
};


void DraggableTable::itemDragMove(const SourceDetails& dragSourceDetails)
{
    int mouseOverIdx = getRowContainingPosition(dragSourceDetails.localPosition.x,
        dragSourceDetails.localPosition.y);
    if (mouseOverIdx == -1) { return; } // failed finding row

    // user is dragging source row over the source row
    // do nothing...
    if (mouseOverIdx == model->dragRowIdx)
    {
        return;
    }

    // user is dragging source row over another row.
    model->swapRows(model->dragRowIdx, mouseOverIdx);
    // save the new row index the user is dragging
    model->dragRowIdx = mouseOverIdx;
    // update information in rows
    updateContent();
    // ListBox must be repainted, or else it will display old clipped images of
    // the old rows. This would look like GUI bug to the user otherwise.
    repaint();
}

void DraggableTable::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    model->draggingOutsideContainer = false;
}
void DraggableTable::itemDragExit(const SourceDetails& dragSourceDetails)
{
    model->draggingOutsideContainer = true;
}

void DraggableTable::dragOperationEnded(const DropTarget::SourceDetails& dragSourceDetails)
{
    if (model->draggingOutsideContainer)
    {
        model->deleteRow(model->dragRowIdx);
    }

    model->dragRowIdx = -1;
    updateContent();
    repaint();
}



//==============================================================================
void DraggableTableModel::cellClicked (int, int, const juce::MouseEvent&)       {}
void DraggableTableModel::cellDoubleClicked (int, int, const juce::MouseEvent&) {}
void DraggableTableModel::backgroundClicked (const juce::MouseEvent&)           {}
void DraggableTableModel::sortOrderChanged (int, bool)                    {}
int DraggableTableModel::getColumnAutoSizeWidth (int)                     { return 0; }
void DraggableTableModel::selectedRowsChanged (int)                       {}
void DraggableTableModel::deleteKeyPressed (int)                          {}
void DraggableTableModel::returnKeyPressed (int)                          {}
void DraggableTableModel::listWasScrolled()                               {}

juce::String DraggableTableModel::getCellTooltip (int /*rowNumber*/, int /*columnId*/)    { return {}; }
juce::var DraggableTableModel::getDragSourceDescription (const juce::SparseSet<int>& currentlySelectedRows)
{
    const auto& ranges = currentlySelectedRows.getRanges();
    int idx = ranges[0].getStart();
    return idx;
}

juce::Component* DraggableTableModel::refreshComponentForCell (int, int, bool, juce::Component* existingComponentToUpdate)
{
    ignoreUnused (existingComponentToUpdate);
    jassert (existingComponentToUpdate == nullptr); // indicates a failure in the code that recycles the components
    return nullptr;
}
