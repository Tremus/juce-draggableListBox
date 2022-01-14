#include <JuceHeader.h>


class DropTarget
{
public:
    virtual ~DropTarget() = default;

    //==============================================================================
    class SourceDetails
    {
    public:
        SourceDetails(const var& description,
            Component* sourceComponent,
            Point<int> localPosition) noexcept;
        var description;
        WeakReference<Component> sourceComponent;
        Point<int> localPosition;
    };

    //==============================================================================
    virtual bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) = 0;
    virtual void itemDragEnter(const SourceDetails& dragSourceDetails);
    virtual void itemDragMove(const SourceDetails& dragSourceDetails);
    virtual void itemDragExit(const SourceDetails& dragSourceDetails);
    virtual void itemDropped(const SourceDetails& dragSourceDetails) = 0;
    virtual void dragImageMove(Point<int>& defaultTopLeft);
    virtual bool shouldDrawDragImageWhenOver();
};

//==============================================================================

class DragContainer
{
public:
    //==============================================================================
    DragContainer();

    virtual ~DragContainer();

    //==============================================================================
    void startDragging(const var& sourceDescription,
        Component* sourceComponent,
        const ScaledImage& dragImage = ScaledImage(),
        bool allowDraggingToOtherJuceWindows = false,
        const Point<int>* imageOffsetFromMouse = nullptr,
        const MouseInputSource* inputSourceCausingDrag = nullptr);

    bool isDragAndDropActive() const;
    var getCurrentDragDescription() const;

    //void setCurrentDragImage(const ScaledImage& newImage);

    static DragContainer* findParentDragContainerFor(Component* childComponent);
    //==============================================================================
protected:
    virtual void dragOperationStarted(const DropTarget::SourceDetails&);
    virtual void dragOperationEnded(const DropTarget::SourceDetails&);

private:
    //==============================================================================
    class DragImageComponent;
    OwnedArray<DragImageComponent> dragImageComponents;

    const MouseInputSource* getMouseInputSourceForDrag(Component* sourceComponent, const MouseInputSource* inputSourceCausingDrag);
    bool isAlreadyDragging(Component* sourceComponent) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragContainer)
};
