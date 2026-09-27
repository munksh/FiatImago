import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.fiatimago 1.0
import ".."

// 100 %: the developed photo at full resolution around the point that was
// tapped. Drag to look around, tap to go back.
Page {
    id: page

    property QtObject developer
    property real u: 0.5
    property real v: 0.5

    allowedOrientations: Orientation.Portrait
    backNavigation: false

    function paint() { FiatImagoTheme.applyPalette(page) }
    function request() {
        if (developer) developer.inspect(u, v, Math.round(width), Math.round(height))
    }

    Component.onCompleted: {
        paint()
        request()
    }

    Rectangle {
        anchors.fill: parent
        color: FiatImagoTheme.paper
    }

    DevelopView {
        id: view
        width: page.width
        height: page.height
        developer: page.developer
        mode: "inspect"
    }

    Connections {
        target: page.developer
        onInspectChanged: {
            view.x = 0
            view.y = 0
        }
    }

    MouseArea {
        anchors.fill: parent
        preventStealing: true

        property real startX: 0
        property real startY: 0
        property bool moved: false

        onPressed: {
            startX = mouse.x
            startY = mouse.y
            moved = false
        }
        onPositionChanged: {
            var dx = mouse.x - startX
            var dy = mouse.y - startY
            if (Math.abs(dx) + Math.abs(dy) > Theme.startDragDistance) moved = true
            if (moved) {
                view.x = dx
                view.y = dy
            }
        }
        onReleased: {
            if (!moved) {
                pageStack.pop()
                return
            }
            var fw = page.developer ? page.developer.fullWidth : 0
            var fh = page.developer ? page.developer.fullHeight : 0
            if (fw > 0 && fh > 0) {
                page.u = Math.max(0, Math.min(1, page.u - view.x / fw))
                page.v = Math.max(0, Math.min(1, page.v - view.y / fh))
            }
            page.request()
        }
    }

    Label {
        anchors.left: parent.left
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.paddingLarge
        text: "100 %"
        color: FiatImagoTheme.secondaryText
        font.pixelSize: Theme.fontSizeExtraSmall
    }

    Label {
        anchors.right: parent.right
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.paddingLarge
        text: qsTr("tap to go back")
        color: FiatImagoTheme.secondaryText
        font.pixelSize: Theme.fontSizeExtraSmall
    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: page.developer !== null && page.developer.inspectBusy
    }
}
