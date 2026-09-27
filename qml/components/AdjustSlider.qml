import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

// Drag moves the value relative to where the finger landed, so a small
// correction never jumps. A double tap returns to the default.
Item {
    id: root

    property string label: ""
    property real value: 0
    property real minimumValue: -100
    property real maximumValue: 100
    property real defaultValue: 0
    property real stepSize: 1
    property int decimals: 0
    property string unit: ""
    property bool signedValue: true

    signal moved(real newValue)

    width: parent ? parent.width : 0
    height: Theme.itemSizeSmall

    readonly property real span: maximumValue - minimumValue
    readonly property real fillFrom: signedValue ? defaultValue : minimumValue

    function clamped(v) {
        return Math.max(minimumValue, Math.min(maximumValue, v))
    }
    function positionOf(v) {
        return track.width * (clamped(v) - minimumValue) / span
    }
    function snapped(v) {
        var c = clamped(v)
        return stepSize > 0 ? Math.round(c / stepSize) * stepSize : c
    }
    function formatted(v) {
        var text = Math.abs(v).toFixed(decimals)
        if (signedValue && v > 0 && text !== Number(0).toFixed(decimals)) text = "+" + text
        if (signedValue && v < 0 && text !== Number(0).toFixed(decimals)) text = "\u2212" + text
        return unit === "" ? text : text + " " + unit
    }

    Label {
        id: nameLabel
        x: Theme.horizontalPageMargin
        anchors.top: parent.top
        anchors.topMargin: Theme.paddingSmall
        text: root.label
        color: FiatImagoTheme.primaryText
        font.pixelSize: Theme.fontSizeSmall
    }

    Label {
        anchors.right: parent.right
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.baseline: nameLabel.baseline
        text: root.formatted(root.value)
        color: FiatImagoTheme.secondaryText
        font.pixelSize: Theme.fontSizeSmall
    }

    Item {
        id: track
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * Theme.horizontalPageMargin
        height: Theme.paddingLarge
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.paddingSmall

        Rectangle {
            id: rail
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: Math.max(2, Math.round(Theme.paddingSmall / 2))
            radius: height / 2
            color: FiatImagoTheme.innerBorder
        }

        Rectangle {
            visible: root.signedValue && root.defaultValue > root.minimumValue
                     && root.defaultValue < root.maximumValue
            x: root.positionOf(root.defaultValue) - width / 2
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(2, Math.round(Theme.paddingSmall / 3))
            height: Theme.paddingLarge
            color: FiatImagoTheme.cardBorder
        }

        Rectangle {
            readonly property real from: root.positionOf(Math.min(root.value, root.fillFrom))
            readonly property real to: root.positionOf(Math.max(root.value, root.fillFrom))
            x: from
            width: Math.max(0, to - from)
            anchors.verticalCenter: parent.verticalCenter
            height: rail.height
            color: FiatImagoTheme.primaryText
        }

        Rectangle {
            x: root.positionOf(root.value) - width / 2
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.paddingLarge
            height: width
            radius: width / 2
            color: FiatImagoTheme.primaryText
        }
    }

    MouseArea {
        anchors.fill: parent
        preventStealing: true

        property real startX: 0
        property real startValue: 0
        property real lastClick: 0

        onPressed: {
            startX = mouse.x
            startValue = root.value
        }
        onPositionChanged: {
            var v = root.snapped(startValue + (mouse.x - startX) / track.width * root.span)
            if (v !== root.value) root.moved(v)
        }
        onClicked: {
            var now = new Date().getTime()
            if (now - lastClick < 400) {
                root.moved(root.defaultValue)
                lastClick = 0
            } else {
                lastClick = now
            }
        }
    }
}
