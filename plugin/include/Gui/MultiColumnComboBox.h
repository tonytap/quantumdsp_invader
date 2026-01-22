#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Multi-column ComboBox that displays items in columns with up to maxItemsPerColumn items per column
class MultiColumnComboBox : public juce::ComboBox
{
public:
    MultiColumnComboBox(const juce::String& componentName = {}, int maxItemsPerColumn = 10)
        : juce::ComboBox(componentName), maxPerColumn(maxItemsPerColumn)
    {
    }

    void showPopup() override
    {
        const int numItems = getNumItems();
        if (numItems == 0)
            return;

        // Calculate number of columns needed
        const int numColumns = (numItems + maxPerColumn - 1) / maxPerColumn;

        juce::PopupMenu menu;

        // Organize items into columns
        for (int col = 0; col < numColumns; ++col)
        {
            juce::PopupMenu columnMenu;

            const int startIdx = col * maxPerColumn;
            const int endIdx = juce::jmin(startIdx + maxPerColumn, numItems);

            for (int i = startIdx; i < endIdx; ++i)
            {
                const int itemId = i + 1; // IDs are 1-indexed
                columnMenu.addItem(itemId, getItemText(i), true, getSelectedId() == itemId);
            }

            // Add this column as a submenu
            if (numColumns > 1)
            {
                juce::String columnName = juce::String(col * maxPerColumn + 1) + "-" + juce::String(endIdx);
                menu.addSubMenu(columnName, columnMenu);
            }
            else
            {
                // If only one column, don't use submenu
                menu = columnMenu;
            }
        }

        // Show the menu
        menu.showMenuAsync(juce::PopupMenu::Options()
                                .withTargetComponent(this)
                                .withMinimumWidth(getWidth())
                                .withMaximumNumColumns(1)
                                .withStandardItemHeight(20),
                          [this](int result)
                          {
                              if (result != 0)
                                  setSelectedId(result, juce::sendNotificationAsync);
                          });
    }

private:
    int maxPerColumn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiColumnComboBox)
};
