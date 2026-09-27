import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

// One choice out of a few, written as words: "pull 1  box  push 1  push 2".
// The chosen word is in the accent with a bar under it, the rest are grey.
// The same rule the plate uses for a chosen cell, without the frame.

Flow {
    id: root

    // [{ label: "box", value: ... }, ...]
    property var choices: []
    property var current
    signal chosen(var value)

    spacing: Theme.paddingLarge

    Repeater {
        model: root.choices
        delegate: MouseArea {
            id: word
            readonly property bool on: modelData.value === root.current
            width: wordLabel.implicitWidth + Theme.paddingSmall * 2
            height: Theme.itemSizeExtraSmall
            onClicked: root.chosen(modelData.value)

            Rectangle {
                anchors.fill: parent
                radius: Theme.paddingSmall
                color: FiatImagoTheme.highlightWash
                visible: word.pressed && word.containsMouse
            }

            Label {
                id: wordLabel
                anchors.centerIn: parent
                text: modelData.label
                color: word.on ? FiatImagoTheme.accent : FiatImagoTheme.secondaryText
                font.pixelSize: Theme.fontSizeSmall
                font.bold: word.on
            }

            Rectangle {
                visible: word.on
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: wordLabel.bottom
                anchors.topMargin: Theme.paddingSmall / 2
                width: wordLabel.width
                height: Math.max(2, Theme.paddingSmall / 2)
                color: FiatImagoTheme.accent
            }
        }
    }
}
