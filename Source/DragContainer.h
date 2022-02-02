#include <JuceHeader.h>


class DropTarget
{
public:
    virtual ~DropTarget() = default;

    //==============================================================================
    class SourceDetails
    {
    public:
        SourceDetails(const juce::var& description,
                      juce::Component* sourceComponent,
                      juce::Point<int> localPosition) noexcept;
                      juce::var description;
                      juce::WeakReference<juce::Component> sourceComponent;
                      juce::Point<int> localPosition;
    };

    //==============================================================================
    virtual bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) = 0;
    virtual void itemDragEnter(const SourceDetails& dragSourceDetails);
    virtual void itemDragMove(const SourceDetails& dragSourceDetails);
    virtual void itemDragExit(const SourceDetails& dragSourceDetails);
    virtual void itemDropped(const SourceDetails& dragSourceDetails) = 0;
    virtual void dragImageMove(juce::Point<int>& defaultTopLeft);
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
    void startDragging(const juce::var& sourceDescription,
                       juce::Component* sourceComponent,
                       const juce::ScaledImage& dragImage = juce::ScaledImage(),
                       bool allowDraggingToOtherJuceWindows = false,
                       const juce::Point<int>* imageOffsetFromMouse = nullptr,
                       const juce::MouseInputSource* inputSourceCausingDrag = nullptr);

    bool isDragAndDropActive() const;
    juce::var getCurrentDragDescription() const;

    //void setCurrentDragImage(const juce::ScaledImage& newImage);

    static DragContainer* findParentDragContainerFor(juce::Component* childComponent);
    //==============================================================================
protected:
    virtual void dragOperationStarted(const DropTarget::SourceDetails&);
    virtual void dragOperationEnded(const DropTarget::SourceDetails&);

private:
    //==============================================================================
    class DragImageComponent;
    juce::OwnedArray<DragImageComponent> dragImageComponents;

    const juce::MouseInputSource* getMouseInputSourceForDrag(juce::Component* sourceComponent, const juce::MouseInputSource* inputSourceCausingDrag);
    bool isAlreadyDragging(juce::Component* sourceComponent) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragContainer)
};
