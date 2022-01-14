/*
  ==============================================================================

    DragContainer.cpp
    Created: 14 Jan 2022 10:12:57am
    Author:  tredu

  ==============================================================================
*/

#include "DragContainer.h"


//==============================================================================
class DragContainer::DragImageComponent : public Component,
    private Timer
{
public:
    DragImageComponent(const ScaledImage& im,
        const var& desc,
        Component* const sourceComponent,
        const MouseInputSource* draggingSource,
        DragContainer& ddc,
        Point<int> offset)
        : sourceDetails(desc, sourceComponent, Point<int>()),
        image(im),
        owner(ddc),
        mouseDragSource(draggingSource->getComponentUnderMouse()),
        imageOffset(transformOffsetCoordinates(sourceComponent, offset)),
        originalInputSourceIndex(draggingSource->getIndex()),
        originalInputSourceType(draggingSource->getType())
    {
        updateSize();

        if (mouseDragSource == nullptr)
            mouseDragSource = sourceComponent;

        mouseDragSource->addMouseListener(this, false);

        startTimer(200);

        setInterceptsMouseClicks(false, false);
        setAlwaysOnTop(true);
    }

    ~DragImageComponent() override
    {
        owner.dragImageComponents.remove(owner.dragImageComponents.indexOf(this), false);

        if (mouseDragSource != nullptr)
        {
            mouseDragSource->removeMouseListener(this);

            if (auto* current = getCurrentlyOver())
                if (current->isInterestedInDragSource(sourceDetails))
                    current->itemDragExit(sourceDetails);
        }

        owner.dragOperationEnded(sourceDetails);
    }

    void paint(Graphics& g) override
    {
        if (isOpaque())
            g.fillAll(Colours::white);

        g.setOpacity(1.0f);
        g.drawImage(image.getImage(), getLocalBounds().toFloat());
    }

    void mouseUp(const MouseEvent& e) override
    {
        if (e.originalComponent != this && isOriginalInputSource(e.source))
        {
            if (mouseDragSource != nullptr)
                mouseDragSource->removeMouseListener(this);

            // (note: use a local copy of this in case the callback runs
            // a modal loop and deletes this object before the method completes)
            DropTarget::SourceDetails& details = sourceDetails;
            DropTarget* finalTarget = nullptr;

            auto wasVisible = isVisible();
            setVisible(false);
            Component* unused;
            finalTarget = findTarget(e.getScreenPosition(), details.localPosition, unused);

            if (wasVisible) // fade the component and remove it - it'll be deleted later by the timer callback
                dismissWithAnimation(finalTarget == nullptr);

            if (auto* parent = getParentComponent())
                parent->removeChildComponent(this);

            if (finalTarget != nullptr)
            {
                currentlyOverComp = nullptr;
                finalTarget->itemDropped(details);
            }

            // careful - this object could now be deleted..
        }
    }

    void mouseDrag(const MouseEvent& e) override
    {
        if (e.originalComponent != this && isOriginalInputSource(e.source))
            updateLocation(true, e.getScreenPosition());
    }

    void updateLocation(const bool canDoExternalDrag, Point<int> screenPos)
    {
        DropTarget::SourceDetails& details = sourceDetails;

        setNewScreenPos(screenPos);

        Component* newTargetComp;
        auto* newTarget = findTarget(screenPos, details.localPosition, newTargetComp);

        setVisible(newTarget == nullptr || newTarget->shouldDrawDragImageWhenOver());

        if (newTargetComp != currentlyOverComp)
        {
            if (auto* lastTarget = getCurrentlyOver())
                if (details.sourceComponent != nullptr && lastTarget->isInterestedInDragSource(details))
                    lastTarget->itemDragExit(details);

            currentlyOverComp = newTargetComp;

            if (newTarget != nullptr
                && newTarget->isInterestedInDragSource(details))
                newTarget->itemDragEnter(details);
        }

        sendDragMove(details);

        if (canDoExternalDrag)
        {
            auto now = Time::getCurrentTime();

            if (getCurrentlyOver() != nullptr)
                lastTimeOverTarget = now;
        }

        forceMouseCursorUpdate();
    }

    void updateImage(const ScaledImage& newImage)
    {
        image = newImage;
        updateSize();
        repaint();
    }

    void timerCallback() override
    {
        forceMouseCursorUpdate();

        if (sourceDetails.sourceComponent == nullptr)
        {
            deleteSelf();
        }
        else
        {
            for (auto& s : Desktop::getInstance().getMouseSources())
            {
                if (isOriginalInputSource(s) && !s.isDragging())
                {
                    if (mouseDragSource != nullptr)
                        mouseDragSource->removeMouseListener(this);

                    deleteSelf();
                    break;
                }
            }
        }
    }

    bool keyPressed(const KeyPress& key) override
    {
        if (key == KeyPress::escapeKey)
        {
            dismissWithAnimation(true);
            deleteSelf();
            return true;
        }

        return false;
    }

    bool canModalEventBeSentToComponent(const Component* targetComponent) override
    {
        return targetComponent == mouseDragSource;
    }

    // (overridden to avoid beeps when dragging)
    void inputAttemptWhenModal() override {}

    DropTarget::SourceDetails sourceDetails;

private:
    ScaledImage image;
    DragContainer& owner;
    WeakReference<Component> mouseDragSource, currentlyOverComp;
    const Point<int> imageOffset;
    bool hasCheckedForExternalDrag = false;
    Time lastTimeOverTarget;
    int originalInputSourceIndex;
    MouseInputSource::InputSourceType originalInputSourceType;

    void updateSize()
    {
        const auto bounds = image.getScaledBounds().toNearestInt();
        setSize(bounds.getWidth(), bounds.getHeight());
    }

    void forceMouseCursorUpdate()
    {
        Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
    }

    DropTarget* getCurrentlyOver() const noexcept
    {
        return dynamic_cast<DropTarget*> (currentlyOverComp.get());
    }

    static Component* findDesktopComponentBelow(Point<int> screenPos)
    {
        auto& desktop = Desktop::getInstance();

        for (auto i = desktop.getNumComponents(); --i >= 0;)
        {
            auto* desktopComponent = desktop.getComponent(i);
            auto dPoint = desktopComponent->getLocalPoint(nullptr, screenPos);

            if (auto* c = desktopComponent->getComponentAt(dPoint))
            {
                auto cPoint = c->getLocalPoint(desktopComponent, dPoint);

                if (c->hitTest(cPoint.getX(), cPoint.getY()))
                    return c;
            }
        }

        return nullptr;
    }

    Point<int> transformOffsetCoordinates(const Component* const sourceComponent, Point<int> offsetInSource) const
    {
        return getLocalPoint(sourceComponent, offsetInSource) - getLocalPoint(sourceComponent, Point<int>());
    }

    DropTarget* findTarget(Point<int> screenPos, Point<int>& relativePos,
        Component*& resultComponent) const
    {
        auto* hit = getParentComponent();

        if (hit == nullptr)
            hit = findDesktopComponentBelow(screenPos);
        else
            hit = hit->getComponentAt(hit->getLocalPoint(nullptr, screenPos));

        // (note: use a local copy of this in case the callback runs
        // a modal loop and deletes this object before the method completes)
        DropTarget::SourceDetails details = sourceDetails;

        while (hit != nullptr)
        {
            if (auto* ddt = dynamic_cast<DropTarget*> (hit))
            {
                if (ddt->isInterestedInDragSource(details))
                {
                    relativePos = hit->getLocalPoint(nullptr, screenPos);
                    resultComponent = hit;
                    return ddt;
                }
            }

            hit = hit->getParentComponent();
        }

        resultComponent = nullptr;
        return nullptr;
    }

    void setNewScreenPos(Point<int> screenPos)
    {
        auto newPos = screenPos - imageOffset;

        if (auto* p = getParentComponent())
            newPos = p->getLocalPoint(nullptr, newPos);

        if (auto* target = getCurrentlyOver())
        {
            target->dragImageMove(newPos);
        }
        
        setTopLeftPosition(newPos);
    }

    void sendDragMove(DropTarget::SourceDetails& details) const
    {
        if (auto* target = getCurrentlyOver())
            if (target->isInterestedInDragSource(details))
                target->itemDragMove(details);
    }

    void deleteSelf()
    {
        delete this;
    }

    void dismissWithAnimation(const bool shouldSnapBack)
    {
        setVisible(true);
        auto& animator = Desktop::getInstance().getAnimator();

        if (shouldSnapBack && sourceDetails.sourceComponent != nullptr)
        {
            auto target = sourceDetails.sourceComponent->localPointToGlobal(sourceDetails.sourceComponent->getLocalBounds().getCentre());
            auto ourCentre = localPointToGlobal(getLocalBounds().getCentre());

            animator.animateComponent(this,
                getBounds() + (target - ourCentre),
                0.0f, 120,
                true, 1.0, 1.0);
        }
        else
        {
            animator.fadeOut(this, 120);
        }
    }

    bool isOriginalInputSource(const MouseInputSource& sourceToCheck)
    {
        return (sourceToCheck.getType() == originalInputSourceType
            && sourceToCheck.getIndex() == originalInputSourceIndex);
    }

    JUCE_DECLARE_NON_COPYABLE(DragImageComponent)
};


//==============================================================================
DragContainer::DragContainer() = default;

DragContainer::~DragContainer() = default;

void DragContainer::startDragging(const var& sourceDescription,
    Component* sourceComponent,
    const ScaledImage& dragImage,
    const bool allowDraggingToExternalWindows,
    const Point<int>* imageOffsetFromMouse,
    const MouseInputSource* inputSourceCausingDrag)
{
    if (isAlreadyDragging(sourceComponent))
        return;

    auto* draggingSource = getMouseInputSourceForDrag(sourceComponent, inputSourceCausingDrag);

    if (draggingSource == nullptr || !draggingSource->isDragging())
    {
        jassertfalse;   // You must call startDragging() from within a mouseDown or mouseDrag callback!
        return;
    }

    const auto lastMouseDown = draggingSource->getLastMouseDownPosition().roundToInt();

    struct ImageAndOffset
    {
        ScaledImage image;
        Point<double> offset;
    };

    const auto imageToUse = [&]() -> ImageAndOffset
    {
        if (!dragImage.getImage().isNull())
            return { dragImage, imageOffsetFromMouse != nullptr ? dragImage.getScaledBounds().getConstrainedPoint(-imageOffsetFromMouse->toDouble())
                                                                : dragImage.getScaledBounds().getCentre() };

        const auto scaleFactor = 2.0;
        auto image = sourceComponent->createComponentSnapshot(sourceComponent->getLocalBounds(), true, (float)scaleFactor)
            .convertedToFormat(Image::ARGB);
        image.multiplyAllAlphas(0.6f);

        const auto relPos = sourceComponent->getLocalPoint(nullptr, lastMouseDown).toDouble();
        const auto clipped = (image.getBounds().toDouble() / scaleFactor).getConstrainedPoint(relPos);

        Image fade(Image::SingleChannel, image.getWidth(), image.getHeight(), true);
        Graphics fadeContext(fade);

        ColourGradient gradient;
        gradient.isRadial = true;
        gradient.point1 = clipped.toFloat() * scaleFactor;
        gradient.point2 = gradient.point1 + Point<float>(0.0f, scaleFactor * 400.0f);
        gradient.addColour(0.0, Colours::white);
        gradient.addColour(0.375, Colours::white);
        gradient.addColour(1.0, Colours::transparentWhite);

        fadeContext.setGradientFill(gradient);
        fadeContext.fillAll();

        Image composite(Image::ARGB, image.getWidth(), image.getHeight(), true);
        Graphics compositeContext(composite);

        compositeContext.reduceClipRegion(fade, {});
        compositeContext.drawImageAt(image, 0, 0);

        return { ScaledImage(composite, scaleFactor), clipped };
    }();

    auto* dragImageComponent = dragImageComponents.add(new DragImageComponent(imageToUse.image, sourceDescription, sourceComponent,
        draggingSource, *this, imageToUse.offset.roundToInt()));

    if (allowDraggingToExternalWindows)
    {
        if (!Desktop::canUseSemiTransparentWindows())
            dragImageComponent->setOpaque(true);

        dragImageComponent->addToDesktop(ComponentPeer::windowIgnoresMouseClicks
            | ComponentPeer::windowIsTemporary
            | ComponentPeer::windowIgnoresKeyPresses);
    }
    else
    {
        if (auto* thisComp = dynamic_cast<Component*> (this))
        {
            thisComp->addChildComponent(dragImageComponent);
        }
        else
        {
            jassertfalse;   // Your DragContainer needs to be a Component!
            return;
        }
    }

    dragImageComponent->sourceDetails.localPosition = sourceComponent->getLocalPoint(nullptr, lastMouseDown);
    dragImageComponent->updateLocation(false, lastMouseDown);

#if JUCE_WINDOWS
    // Under heavy load, the layered window's paint callback can often be lost by the OS,
    // so forcing a repaint at least once makes sure that the window becomes visible..
    if (auto* peer = dragImageComponent->getPeer())
        peer->performAnyPendingRepaintsNow();
#endif

    dragOperationStarted(dragImageComponent->sourceDetails);
}

bool DragContainer::isDragAndDropActive() const
{
    return dragImageComponents.size() > 0;
}

var DragContainer::getCurrentDragDescription() const
{
    // If you are performing drag and drop in a multi-touch environment then
    // you should use the getDragDescriptionForIndex() method instead!
    jassert(dragImageComponents.size() < 2);

    return dragImageComponents.size() != 0 ? dragImageComponents[0]->sourceDetails.description
        : var();
}

/*
void DragContainer::setCurrentDragImage(const ScaledImage& newImage)
{
    // If you are performing drag and drop in a multi-touch environment then
    // you should use the setDragImageForIndex() method instead!
    jassert(dragImageComponents.size() < 2);

    dragImageComponents[0]->updateImage(newImage);
}
*/

DragContainer* DragContainer::findParentDragContainerFor(Component* c)
{
    return c != nullptr ? c->findParentComponentOfClass<DragContainer>() : nullptr;
}

void DragContainer::dragOperationStarted(const DropTarget::SourceDetails&) {}
void DragContainer::dragOperationEnded(const DropTarget::SourceDetails&) {}

const MouseInputSource* DragContainer::getMouseInputSourceForDrag(Component* sourceComponent,
    const MouseInputSource* inputSourceCausingDrag)
{
    if (inputSourceCausingDrag == nullptr)
    {
        auto minDistance = std::numeric_limits<float>::max();
        auto& desktop = Desktop::getInstance();

        auto centrePoint = sourceComponent ? sourceComponent->getScreenBounds().getCentre().toFloat() : Point<float>();
        auto numDragging = desktop.getNumDraggingMouseSources();

        for (auto i = 0; i < numDragging; ++i)
        {
            if (auto* ms = desktop.getDraggingMouseSource(i))
            {
                auto distance = ms->getScreenPosition().getDistanceSquaredFrom(centrePoint);

                if (distance < minDistance)
                {
                    minDistance = distance;
                    inputSourceCausingDrag = ms;
                }
            }
        }
    }

    // You must call startDragging() from within a mouseDown or mouseDrag callback!
    jassert(inputSourceCausingDrag != nullptr && inputSourceCausingDrag->isDragging());

    return inputSourceCausingDrag;
}

bool DragContainer::isAlreadyDragging(Component* component) const noexcept
{
    for (auto* dragImageComp : dragImageComponents)
    {
        if (dragImageComp->sourceDetails.sourceComponent == component)
            return true;
    }

    return false;
}

//==============================================================================
DropTarget::SourceDetails::SourceDetails(const var& desc, Component* comp, Point<int> pos) noexcept
    : description(desc),
    sourceComponent(comp),
    localPosition(pos)
{
}

void DropTarget::itemDragEnter(const SourceDetails&) {}
void DropTarget::itemDragMove(const SourceDetails&) {}
void DropTarget::itemDragExit(const SourceDetails&) {}
void DropTarget::dragImageMove(Point<int>&) {}
bool DropTarget::shouldDrawDragImageWhenOver() { return true; }
