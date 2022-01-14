#include "MainComponent.h"

MainContentComponent::MainContentComponent()
    : listBoxModel(itemData)
    , listBox(itemData)
{
    itemData.addItemAtEnd();
    itemData.addItemAtEnd();
    itemData.addItemAtEnd();

    addBtn.setButtonText("Add Item...");
    addBtn.onClick = [this]()
    {
        itemData.addItemAtEnd();
        listBox.updateContent();
    };
    addAndMakeVisible(addBtn);

    listBox.setModel(&listBoxModel);
    listBox.setRowHeight(40);
    addAndMakeVisible(listBox);
    setSize (600, 400);
}

void MainContentComponent::paint (Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void MainContentComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto row = area.removeFromTop(24);
    addBtn.setBounds(row.removeFromRight(100));

    area.removeFromTop(6);
    listBox.setBounds(area);
}
