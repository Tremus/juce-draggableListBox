#include "MainComponent.h"

MainContentComponent::MainContentComponent()
    : table(itemData)
{
    itemData.addItemAtEnd();
    itemData.addItemAtEnd();
    itemData.addItemAtEnd();

    auto& header = table.getHeader();
    header.addColumn("Col 1", 1, 100);
    header.addColumn("Col 2", 2, 100);
    header.addColumn("Col 3", 3, 100);
    header.setColumnVisible(1, true);
    header.setColumnVisible(2, true);
    header.setColumnVisible(3, true);
    header.setStretchToFitActive(true);

    table.setModel(&itemData);
    table.setRowHeight(40);

    addBtn.setButtonText("Add Item...");
    addBtn.onClick = [this]()
    {
        itemData.addItemAtEnd();
        table.updateContent();
    };
    addAndMakeVisible(addBtn);

    addAndMakeVisible(table);
    setSize (600, 400);
}

void MainContentComponent::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainContentComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto row = area.removeFromTop(24);
    addBtn.setBounds(row.removeFromRight(100));

    area.removeFromTop(6);
    table.setBounds(area);
    table.updateContent();
}
