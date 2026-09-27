import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

// The two buttons of the family. Filled: the one thing this page is for
// (measure, save, load). Outlined: the second thing (log shot).
//
// A rounded rectangle, not a pill: the corners are the card radius, not half
// the height.

BackgroundItem {
    id: root

    property string text: ""
    property string detail: ""        // small, after the text: "EV 11.3"
    property bool filled: true

    width: parent ? parent.width : implicitWidth
    height: Theme.itemSizeMedium + Theme.paddingMedium
    highlightedColor: "transparent"
    opacity: enabled ? 1.0 : 0.35

    readonly property color ink: filled ? FiatImagoTheme.markOn(FiatImagoTheme.accent) : FiatImagoTheme.primaryText

    Rectangle {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        height: Theme.itemSizeMedium
        radius: Theme.paddingLarge
        color: root.filled
               ? (root.highlighted ? Qt.darker(FiatImagoTheme.accent, 1.2) : FiatImagoTheme.accent)
               : (root.highlighted ? FiatImagoTheme.highlightWash : Theme.rgba(FiatImagoTheme.card, 0.6))
        border.color: FiatImagoTheme.accent
        border.width: root.filled ? 0 : 1

        Row {
            anchors.centerIn: parent
            spacing: Theme.paddingMedium
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.text
                color: root.ink
                font.pixelSize: Theme.fontSizeMedium
                font.family: FiatImagoTheme.serif
                font.italic: true
                font.bold: root.filled
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.detail !== ""
                text: root.detail
                color: root.ink
                opacity: 0.9
                font.pixelSize: Theme.fontSizeSmall
            }
        }
    }
}
