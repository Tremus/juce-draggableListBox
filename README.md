# juce-draggableListBox
In July 2018, Charles Schiermeyer (aka *matkatmusic*) submitted a [query to the JUCE Forum](https://forum.juce.com/t/listbox-drag-to-reorder-solved/28477), asking for help implementing drag-to-reorder functionality in the JUCE *ListBox* class. Shortly afterward, the forum post was marked "Solved", and Charles was kind enough to provide a [full example implementation hosted on BitBucket](https://bitbucket.org/MatkatMusic/listboxreorder/).

This is a revised version of Charles's code...

**edit 2022** this is also a revised version of @getdunne's [code](https://github.com/getdunne/juce-draggableListBox)

Dragging a row this version will cause the rows to swap as you drag.

To implement a JUCE list-box with drag-to-reorder capability:
1. Include the files *DraggableListBox.h*, *DraggableListBox.cpp*, *DragContainer.cpp*, *DragContainer.cpp* in your JUCE project.
2. Define your own app-specific class for your actual list data, which inherits from *DraggableListBoxItemData*, ensuring that you override the following member functions:
   - *size()* should return the number of list elements.
   - *paintRow()* is a callback to render one specific list item.
   - *swapRows()* is a callback which reorders the list, moving the dragged item either before or after the item it was dropped onto.
3. Initialize your model object with references to your *DraggableListBox*, *DraggableListBoxModel* and your app-specific data object.
4. Set your *DraggableListBox*'s model to be your model object (by calling the former's *setModel()* member).
5. *Optional*: override *deleteRow()* on your app-specific data object to automatically delete rows that are dragged outside of your list.
6. *Optional*: You can change the default location for the dragged image component as it moves. To do this, create a ListBox class that inherits from *DraggableListBox*. Then overriding the function *dragImageMove()*.  

In this example, all custom list-related classes are defined in *MyListComponent.h/.cpp*, and the surrounding GUI is defined in *MainContentComponent.h/.cpp*. Both contain examples of how to implement the optional features  (*Main.cpp* is a Projucer-generated file and is completely generic.)
