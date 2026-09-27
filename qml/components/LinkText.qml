import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

// A word you can tap: "change", "start from modern", "+ a speed not listed".
// Accent, and underlined when it stands among other text. Replaces the small
// pills; a pill around one word said "button" louder than the word did.

MouseArea {
    id: root

    property alias text: label.text
    property bool underline: true
    property bool italic: false
    property color color: FiatImagoTheme.accent
    property int fontSize: Theme.fontSizeSmall
    property int horizontalAlignment: Text.AlignLeft

    implicitWidth: label.implicitWidth + Theme.paddingSmall * 2
    implicitHeight: Math.max(Theme.itemSizeExtraSmall, label.implicitHeight + Theme.paddingSmall * 2)
    width: implicitWidth
    height: implicitHeight
    opacity: enabled ? 1.0 : 0.4

    Rectangle {
        anchors.fill: parent
        radius: Theme.paddingSmall
        color: FiatImagoTheme.highlightWash
        visible: root.pressed && root.containsMouse
    }

    Label {
        id: label
        anchors.verticalCenter: parent.verticalCenter
        x: root.horizontalAlignment === Text.AlignRight ? parent.width - width - Theme.paddingSmall
           : root.horizontalAlignment === Text.AlignHCenter ? (parent.width - width) / 2
           : Theme.paddingSmall
        width: Math.min(implicitWidth, root.width - Theme.paddingSmall * 2)
        truncationMode: TruncationMode.Fade
        color: root.color
        font.pixelSize: root.fontSize
        font.underline: root.underline
        font.italic: root.italic
        font.family: root.italic ? FiatImagoTheme.serif : Theme.fontFamily
    }
}
