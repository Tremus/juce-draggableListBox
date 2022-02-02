#include "DraggableListBox.h"


//==============================================================================

void DraggableListBox::itemDragMove(const SourceDetails& dragSourceDetails)
{
    int mouseOverIdx = getRowContainingPosition(dragSourceDetails.localPosition.x,
        dragSourceDetails.localPosition.y);
    if (mouseOverIdx == -1) { return; } // failed finding row

    // user is dragging source row over the source row
    // do nothing...
    if (mouseOverIdx == modelData.dragRowIdx)
    {
        return;
    }

    // user is dragging source row over another row.
    modelData.swapRows(modelData.dragRowIdx, mouseOverIdx);
    // save the new row index the user is dragging
    modelData.dragRowIdx = mouseOverIdx;
    // update information in rows
    updateContent();
    // ListBox must be repainted, or else it will display old clipped images of
    // the old rows. This would look like GUI bug to the user otherwise.
    repaint();
}

void DraggableListBox::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    modelData.draggingOutsideContainer = false;
}
void DraggableListBox::itemDragExit(const SourceDetails& dragSourceDetails)
{
    modelData.draggingOutsideContainer = true;
}

void DraggableListBox::dragOperationEnded(const DropTarget::SourceDetails& dragSourceDetails)
{
    if (modelData.draggingOutsideContainer)
    {
        modelData.deleteRow(modelData.dragRowIdx);
    }

    modelData.dragRowIdx = -1;
    updateContent();
    repaint();
}

//==============================================================================

juce::Component* DraggableListBoxModel::refreshComponentForRow(int rowNumber,
                                                               bool /*isRowSelected*/,
                                                               juce::Component* existingComponentToUpdate)
{
    std::unique_ptr<DraggableListBoxItem> item(dynamic_cast<DraggableListBoxItem*>(existingComponentToUpdate));

    if (juce::isPositiveAndBelow(rowNumber, modelData.size()))
    {
        if (item == nullptr)
            item = std::make_unique<DraggableListBoxItem>(modelData, rowNumber);
        else
            item->rowIdx = rowNumber;
    }
    return item.release();
}

//==============================================================================

void DraggableListBoxItem::mouseEnter(const juce::MouseEvent&)
{
    savedCursor = getMouseCursor();
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void DraggableListBoxItem::mouseExit(const juce::MouseEvent&)
{
    setMouseCursor(savedCursor);
}

void DraggableListBoxItem::mouseDrag(const juce::MouseEvent& e)
{
    if (modelData.dragRowIdx == rowIdx) { return; }
    if (DragContainer* container = DragContainer::findParentDragContainerFor(this))
    {
        if (!container->isDragAndDropActive())
        {
            juce::ScaledImage scaledImg (createComponentSnapshot(getLocalBounds()));
            container->startDragging(rowIdx, this, scaledImg);
            modelData.dragRowIdx = rowIdx;
        }
    }
}
