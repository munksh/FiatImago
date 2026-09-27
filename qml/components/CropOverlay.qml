import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

// Crop frame over the uncropped image. Values are fractions of the frame;
// ratio is width over height in those fractions (0 is free).
Item {
    id: root

    property rect frame: Qt.rect(0, 0, 0, 0)
    property real cropX: 0
    property real cropY: 0
    property real cropW: 1
    property real cropH: 1
    property real ratio: 0

    signal committed(real nx, real ny, real nw, real nh)

    readonly property real minSize: 0.1
    property bool dragging: false

    property real lx: 0
    property real ly: 0
    property real lw: 1
    property real lh: 1

    readonly property real px: frame.x + lx * frame.width
    readonly property real py: frame.y + ly * frame.height
    readonly property real pw: lw * frame.width
    readonly property real ph: lh * frame.height

    function sync() {
        lx = cropX
        ly = cropY
        lw = cropW
        lh = cropH
    }
    function clamp(v, low, high) {
        return Math.max(low, Math.min(high, v))
    }
    function commit() {
        dragging = false
        committed(lx, ly, lw, lh)
    }
    function dragCorner(right, bottom, nx, ny) {
        var fx = right ? lx : lx + lw
        var fy = bottom ? ly : ly + lh
        var w = clamp(right ? nx - fx : fx - nx, minSize, right ? 1 - fx : fx)
        var h = clamp(bottom ? ny - fy : fy - ny, minSize, bottom ? 1 - fy : fy)
        if (ratio > 0) {
            if (w / h > ratio) w = h * ratio
            else h = w / ratio
        }
        lx = right ? fx : fx - w
        ly = bottom ? fy : fy - h
        lw = w
        lh = h
    }

    onCropXChanged: if (!dragging) sync()
    onCropYChanged: if (!dragging) sync()
    onCropWChanged: if (!dragging) sync()
    onCropHChanged: if (!dragging) sync()
    Component.onCompleted: sync()

    Rectangle {
        x: root.frame.x; y: root.frame.y
        width: root.frame.width; height: Math.max(0, root.py - root.frame.y)
        color: FiatImagoTheme.paper; opacity: 0.7
    }
    Rectangle {
        x: root.frame.x; y: root.py + root.ph
        width: root.frame.width; height: Math.max(0, root.frame.y + root.frame.height - root.py - root.ph)
        color: FiatImagoTheme.paper; opacity: 0.7
    }
    Rectangle {
        x: root.frame.x; y: root.py
        width: Math.max(0, root.px - root.frame.x); height: root.ph
        color: FiatImagoTheme.paper; opacity: 0.7
    }
    Rectangle {
        x: root.px + root.pw; y: root.py
        width: Math.max(0, root.frame.x + root.frame.width - root.px - root.pw); height: root.ph
        color: FiatImagoTheme.paper; opacity: 0.7
    }

    Repeater {
        model: 2
        Rectangle {
            visible: root.dragging
            x: root.px + root.pw * (index + 1) / 3; y: root.py
            width: 1; height: root.ph
            color: FiatImagoTheme.innerBorder
        }
    }
    Repeater {
        model: 2
        Rectangle {
            visible: root.dragging
            x: root.px; y: root.py + root.ph * (index + 1) / 3
            width: root.pw; height: 1
            color: FiatImagoTheme.innerBorder
        }
    }

    Rectangle {
        x: root.px; y: root.py
        width: root.pw; height: root.ph
        color: "transparent"
        border.color: FiatImagoTheme.primaryText
        border.width: 2
    }

    MouseArea {
        x: root.px; y: root.py
        width: root.pw; height: root.ph
        preventStealing: true

        property real startPx: 0
        property real startPy: 0
        property real startX: 0
        property real startY: 0

        onPressed: {
            var p = mapToItem(root, mouse.x, mouse.y)
            startPx = p.x
            startPy = p.y
            startX = root.lx
            startY = root.ly
            root.dragging = true
        }
        onPositionChanged: {
            var p = mapToItem(root, mouse.x, mouse.y)
            root.lx = root.clamp(startX + (p.x - startPx) / root.frame.width, 0, 1 - root.lw)
            root.ly = root.clamp(startY + (p.y - startPy) / root.frame.height, 0, 1 - root.lh)
        }
        onReleased: root.commit()
        onCanceled: root.commit()
    }

    Repeater {
        model: 4
        Item {
            id: handle
            readonly property bool rightSide: index % 2 === 1
            readonly property bool bottomSide: index >= 2
            readonly property real arm: Theme.paddingLarge * 1.5
            readonly property real thick: Math.max(4, Math.round(Theme.paddingSmall))

            width: Theme.itemSizeSmall
            height: Theme.itemSizeSmall
            x: (rightSide ? root.px + root.pw : root.px) - width / 2
            y: (bottomSide ? root.py + root.ph : root.py) - height / 2

            Rectangle {
                x: handle.rightSide ? handle.width / 2 - handle.arm : handle.width / 2
                y: handle.bottomSide ? handle.height / 2 - handle.thick : handle.height / 2
                width: handle.arm; height: handle.thick
                color: FiatImagoTheme.primaryText
            }
            Rectangle {
                x: handle.rightSide ? handle.width / 2 - handle.thick : handle.width / 2
                y: handle.bottomSide ? handle.height / 2 - handle.arm : handle.height / 2
                width: handle.thick; height: handle.arm
                color: FiatImagoTheme.primaryText
            }

            MouseArea {
                anchors.fill: parent
                preventStealing: true
                onPressed: root.dragging = true
                onPositionChanged: {
                    var p = mapToItem(root, mouse.x, mouse.y)
                    root.dragCorner(handle.rightSide, handle.bottomSide,
                                    (p.x - root.frame.x) / root.frame.width,
                                    (p.y - root.frame.y) / root.frame.height)
                }
                onReleased: root.commit()
                onCanceled: root.commit()
            }
        }
    }
}
