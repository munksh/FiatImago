import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

// WordChoice laid out in even columns, for a set of words that should read
// as a table rather than a sentence.
Grid {
    id: grid

    property var choices: []
    property var current
    property int columnCount: 5

    signal chosen(var value)

    columns: columnCount
    rowSpacing: Theme.paddingSmall

    Repeater {
        model: grid.choices

        MouseArea {
            id: word
            readonly property bool on: modelData.value === grid.current

            width: grid.width / grid.columnCount
            height: Theme.itemSizeExtraSmall
            onClicked: grid.chosen(modelData.value)

            Rectangle {
                anchors.fill: parent
                radius: Theme.paddingSmall
                color: FiatImagoTheme.highlightWash
                visible: word.pressed
            }

            Label {
                id: wordLabel
                anchors.centerIn: parent
                width: Math.min(implicitWidth, word.width - Theme.paddingSmall * 2)
                horizontalAlignment: Text.AlignHCenter
                truncationMode: TruncationMode.Fade
                text: modelData.label
                color: word.on ? FiatImagoTheme.accent : FiatImagoTheme.secondaryText
                font.pixelSize: Theme.fontSizeSmall
                font.bold: word.on
            }

            Rectangle {
                anchors.top: wordLabel.bottom
                anchors.topMargin: Theme.paddingSmall / 2
                anchors.horizontalCenter: wordLabel.horizontalCenter
                width: wordLabel.width
                height: Math.max(2, Theme.paddingSmall / 2)
                color: FiatImagoTheme.accent
                visible: word.on
            }
        }
    }
}
