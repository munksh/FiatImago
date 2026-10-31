import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.fiatimago 1.0
import ".."
import "../components"

Page {
    id: page

    property string itemKey: ""
    property string name: ""
    property string shortName: ""
    property string jpegPath: ""
    property string rawPath: ""
    property string jsonPath: ""
    property string altJsonPath: ""
    property string thumb: ""

    property string tool: "light"
    property bool holding: false
    property string colourView: "basic"
    property string effectsView: "vignette"
    property string mixBand: "Green"
    property real loupeU: 0.5
    property real loupeV: 0.5
    readonly property int loupeSize: Math.round(Math.min(stage.width, stage.height) * 0.45)

    function showLoupe() {
        dev.inspect(loupeU, loupeV, loupeSize, loupeSize)
    }

    // Zoom in the photo area. 1 shows the whole frame; zoomU and zoomV are
    // the point of the frame, in fractions, at the centre of the view. While
    // fingers move, the preview is only enlarged; when they lift, the part in
    // view is developed sharp from the full-size photo.
    property real zoom: 1
    property real zoomU: 0.5
    property real zoomV: 0.5
    property bool zooming: false
    property int zoomRequest: -1
    readonly property bool zoomed: zoom > 1.001
    readonly property real fitScale: dev.frameWidth > 0 && view.paintedRect.width > 0
                                     ? view.paintedRect.width / dev.frameWidth : 0
    readonly property real maxZoom: fitScale > 0 ? Math.max(1, 2 / fitScale) : 1

    // Half the part in view, in frame fractions, at zoom z.
    function halfSpan(z, viewSize, paintedSize) {
        return paintedSize > 0 ? Math.min(0.5, viewSize / (2 * paintedSize * z)) : 0.5
    }

    function clampZoomCentre() {
        var r = view.paintedRect
        var hu = halfSpan(zoom, lens.width, r.width)
        var hv = halfSpan(zoom, lens.height, r.height)
        zoomU = Math.max(hu, Math.min(1 - hu, zoomU))
        zoomV = Math.max(hv, Math.min(1 - hv, zoomV))
    }

    function requestZoom() {
        if (!zoomed) {
            dev.clearZoom()
            zoomRequest = -1
            return
        }
        var r = view.paintedRect
        var hu = halfSpan(zoom, lens.width, r.width)
        var hv = halfSpan(zoom, lens.height, r.height)
        zoomRequest = dev.zoom(zoomU - hu, zoomV - hv, 2 * hu, 2 * hv,
                               Math.round(2 * hu * r.width * zoom), Math.round(2 * hv * r.height * zoom))
    }

    function resetZoom() {
        zoom = 1
        zoomU = 0.5
        zoomV = 0.5
        zooming = false
        requestZoom()
    }

    onToolChanged: {
        if (tool === "crop")
            resetZoom()
        if (tool === "detail")
            showLoupe()
    }

    function resetMixer() {
        var bands = ["Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple", "Magenta"]
        for (var i = 0; i < bands.length; ++i) {
            dev.set("hue" + bands[i], 0)
            dev.set("sat" + bands[i], 0)
            dev.set("lum" + bands[i], 0)
        }
    }

    function kind() {
        return /\.png$/i.test(jpegPath) ? "png" : "jpeg"
    }

    function currentLook() {
        var id = dev.recipe.look
        return (id === undefined || id === "") ? "builtin:none" : id
    }

    function deleteLook(id, name) {
        remorse.execute(qsTr("Deleting %1").arg(name), function() {
            if (dev.recipe.look === id) dev.set("look", "")
            imagoPresets.remove(id)
        })
    }

    allowedOrientations: Orientation.Portrait

    function paint() { FiatImagoTheme.applyPalette(page) }

    function value(key) {
        var v = dev.recipe[key]
        return v === undefined ? 0 : v
    }

    // Width over height of the uncropped frame after the 90° turns.
    function frameAspect() {
        var a = dev.frameAspect > 0 ? dev.frameAspect : 1
        return (value("rotation") % 2) === 1 ? 1 / a : a
    }

    // Ratio in frame fractions (crop width over crop height); 0 is free.
    function ratioFor(aspect) {
        if (aspect === "free") return 0
        if (aspect === "original") return 1
        var parts = aspect.split(":")
        var a = Number(parts[0])
        var b = Number(parts[1])
        var r = Math.max(a, b) / Math.min(a, b)
        var fa = frameAspect()
        if (fa < 1) r = 1 / r
        return r / fa
    }

    function chooseAspect(aspect) {
        dev.set("aspect", aspect)
        var r = ratioFor(aspect)
        if (r <= 0) return
        var w = r >= 1 ? 1 : r
        var h = r >= 1 ? 1 / r : 1
        dev.setCrop((1 - w) / 2, (1 - h) / 2, w, h)
    }

    Component.onCompleted: {
        paint()
        imagoLibrary.setCurrent(shortName, thumb)
        dev.open({
            key: itemKey,
            name: name,
            jpeg: jpegPath,
            raw: rawPath,
            json: jsonPath,
            altJson: altJsonPath
        })
    }

    Developer {
        id: dev
        uncropped: page.tool === "crop"
        loupe: page.tool === "detail"
    }

    Rectangle {
        anchors.fill: parent
        color: FiatImagoTheme.paper
    }

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: Math.max(page.height, column.height)

        PullDownMenu {
            highlightColor: FiatImagoTheme.chromeAccent

            MenuItem {
                text: qsTr("Reset all")
                color: FiatImagoTheme.primaryText
                onClicked: {
                    page.resetZoom()
                    dev.resetAll()
                }
            }
            MenuItem {
                text: qsTr("Copy settings")
                color: FiatImagoTheme.primaryText
                onClicked: dev.copySettings()
            }
            MenuItem {
                visible: dev.hasClipboard
                text: qsTr("Paste settings")
                color: FiatImagoTheme.primaryText
                onClicked: dev.pasteSettings()
            }
            MenuItem {
                enabled: dev.loaded
                text: qsTr("Save look")
                color: FiatImagoTheme.primaryText
                onClicked: pageStack.push(Qt.resolvedUrl("SaveLookPage.qml"), {
                    developer: dev,
                    name: page.shortName
                })
            }
            MenuItem {
                enabled: dev.loaded
                text: qsTr("Export")
                color: FiatImagoTheme.primaryText
                onClicked: pageStack.push(Qt.resolvedUrl("ExportPage.qml"), {
                    developer: dev,
                    name: page.shortName,
                    hasCameraData: page.kind() === "jpeg" && jpegPath !== ""
                })
            }
        }

        Column {
            id: column
            width: parent.width

            PageHead {
                title: page.shortName
                subtitle: dev.loaded ? (dev.isRaw ? "raw" : page.kind()) : ""
            }

            Item {
                id: stage
                width: parent.width
                height: Math.round(page.height * 0.5)

                Item {
                    id: lens
                    anchors.fill: parent
                    anchors.leftMargin: Theme.paddingMedium
                    anchors.rightMargin: Theme.paddingMedium
                    anchors.topMargin: histogram.visible ? histogram.height + Theme.paddingSmall : 0
                    anchors.bottomMargin: Theme.paddingLarge
                    clip: true

                    DevelopView {
                        id: view
                        width: lens.width
                        height: lens.height
                        developer: dev
                        mode: page.holding ? "original" : "preview"
                        transform: [
                            Scale {
                                xScale: page.zoom
                                yScale: page.zoom
                            },
                            Translate {
                                x: lens.width / 2 - (view.paintedRect.x + page.zoomU * view.paintedRect.width) * page.zoom
                                y: lens.height / 2 - (view.paintedRect.y + page.zoomV * view.paintedRect.height) * page.zoom
                            }
                        ]
                    }

                    // The part in view, developed sharp; shown once it matches
                    // what the fingers left.
                    DevelopView {
                        readonly property real spanW: 2 * page.halfSpan(page.zoom, lens.width, view.paintedRect.width)
                                                      * view.paintedRect.width * page.zoom
                        readonly property real spanH: 2 * page.halfSpan(page.zoom, lens.height, view.paintedRect.height)
                                                      * view.paintedRect.height * page.zoom
                        visible: page.zoomed && !page.zooming && !page.holding
                                 && dev.zoomRequest === page.zoomRequest
                        x: (lens.width - spanW) / 2
                        y: (lens.height - spanH) / 2
                        width: spanW
                        height: spanH
                        developer: dev
                        mode: "zoom"
                    }
                }

                PinchArea {
                    id: photoPinch
                    anchors.fill: lens
                    enabled: page.tool !== "crop" && dev.loaded

                    property real startZoom: 1
                    property real startU: 0.5
                    property real startV: 0.5

                    onPinchStarted: {
                        startZoom = page.zoom
                        startU = page.zoomU
                        startV = page.zoomV
                        page.zooming = true
                    }
                    onPinchUpdated: {
                        var r = view.paintedRect
                        if (r.width <= 0 || r.height <= 0) return
                        var z = Math.max(1, Math.min(page.maxZoom, startZoom * pinch.scale))
                        // The point under the fingers stays under them.
                        var pu = startU + (pinch.startCenter.x - lens.width / 2) / (r.width * startZoom)
                        var pv = startV + (pinch.startCenter.y - lens.height / 2) / (r.height * startZoom)
                        page.zoom = z
                        page.zoomU = pu - (pinch.center.x - lens.width / 2) / (r.width * z)
                        page.zoomV = pv - (pinch.center.y - lens.height / 2) / (r.height * z)
                        page.clampZoomCentre()
                    }
                    onPinchFinished: {
                        page.zooming = false
                        page.requestZoom()
                    }

                    MouseArea {
                        anchors.fill: parent
                        preventStealing: page.zoomed

                        property real startX: 0
                        property real startY: 0
                        property real startU: 0.5
                        property real startV: 0.5
                        property bool moved: false

                        function finishMove() {
                            page.holding = false
                            if (!moved) return
                            page.zooming = false
                            page.requestZoom()
                        }

                        onPressed: {
                            startX = mouse.x
                            startY = mouse.y
                            startU = page.zoomU
                            startV = page.zoomV
                            moved = false
                        }
                        onPositionChanged: {
                            if (!page.zoomed) return
                            var dx = mouse.x - startX
                            var dy = mouse.y - startY
                            if (!moved && Math.abs(dx) + Math.abs(dy) > Theme.startDragDistance) {
                                moved = true
                                page.holding = false
                                page.zooming = true
                            }
                            if (!moved) return
                            var r = view.paintedRect
                            page.zoomU = startU - dx / (r.width * page.zoom)
                            page.zoomV = startV - dy / (r.height * page.zoom)
                            page.clampZoomCentre()
                        }
                        onPressAndHold: if (!moved) page.holding = true
                        onReleased: finishMove()
                        onCanceled: finishMove()
                        onClicked: {
                            if (moved || page.zoomed || page.tool !== "detail") return
                            var r = view.paintedRect
                            if (r.width <= 0 || r.height <= 0) return
                            var u = (mouse.x - r.x) / r.width
                            var v = (mouse.y - r.y) / r.height
                            if (u < 0 || u > 1 || v < 0 || v > 1) return
                            page.loupeU = u
                            page.loupeV = v
                            page.showLoupe()
                        }
                        onDoubleClicked: {
                            if (page.zoomed) {
                                page.resetZoom()
                                return
                            }
                            var r = view.paintedRect
                            if (r.width <= 0 || r.height <= 0 || page.fitScale <= 0) return
                            var u = (mouse.x - r.x) / r.width
                            var v = (mouse.y - r.y) / r.height
                            if (u < 0 || u > 1 || v < 0 || v > 1) return
                            // A double tap goes straight to 100 % at that point.
                            page.zoom = Math.max(1, Math.min(page.maxZoom, 1 / page.fitScale))
                            page.zoomU = u
                            page.zoomV = v
                            page.clampZoomCentre()
                            page.requestZoom()
                        }
                    }
                }

                Rectangle {
                    readonly property rect r: view.paintedRect
                    readonly property real side: dev.fullWidth > 0
                                                 ? page.loupeSize / dev.fullWidth * r.width : 0
                    visible: page.tool === "detail" && dev.loaded && !page.zoomed && side > 0
                    x: lens.x + r.x + page.loupeU * r.width - side / 2
                    y: lens.y + r.y + page.loupeV * r.height - side / 2
                    width: side
                    height: side
                    color: "transparent"
                    border.color: FiatImagoTheme.primaryText
                    border.width: 2
                }

                Rectangle {
                    id: loupe
                    visible: page.tool === "detail" && dev.loaded && !page.zoomed
                    anchors.right: lens.right
                    anchors.bottom: lens.bottom
                    width: page.loupeSize + 4
                    height: page.loupeSize + 4
                    color: FiatImagoTheme.paper
                    border.color: FiatImagoTheme.primaryText
                    border.width: 2
                    clip: true

                    DevelopView {
                        anchors.fill: parent
                        anchors.margins: 2
                        developer: dev
                        mode: "inspect"
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        size: BusyIndicatorSize.Small
                        running: loupe.visible && dev.inspectBusy
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        anchors.margins: Theme.paddingSmall
                        text: "100 %"
                        color: FiatImagoTheme.primaryText
                        font.pixelSize: Theme.fontSizeTiny
                    }
                }

                BusyIndicator {
                    anchors.centerIn: lens
                    size: BusyIndicatorSize.Small
                    running: page.zoomed && !page.zooming && dev.zoomBusy
                }

                CropOverlay {
                    anchors.fill: lens
                    visible: page.tool === "crop" && dev.loaded
                    frame: view.paintedRect
                    cropX: page.value("cropX")
                    cropY: page.value("cropY")
                    cropW: dev.recipe.cropW === undefined ? 1 : dev.recipe.cropW
                    cropH: dev.recipe.cropH === undefined ? 1 : dev.recipe.cropH
                    ratio: page.ratioFor(dev.recipe.aspect === undefined ? "original" : dev.recipe.aspect)
                    onCommitted: dev.setCrop(nx, ny, nw, nh)
                }

                Histogram {
                    id: histogram
                    anchors.top: parent.top
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    height: Theme.itemSizeExtraSmall
                    visible: page.tool !== "crop" && dev.loaded
                    bins: dev.histogram
                }

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.bottom: parent.bottom
                    visible: dev.loaded && page.zoomed && page.fitScale > 0
                    text: Math.round(page.zoom * page.fitScale * 100) + " %"
                    color: FiatImagoTheme.secondaryText
                    font.pixelSize: Theme.fontSizeTiny
                }

                Label {
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.horizontalPageMargin
                    anchors.bottom: parent.bottom
                    visible: dev.loaded && page.tool !== "crop"
                    text: page.holding ? qsTr("original")
                                       : page.zoomed ? qsTr("double tap for the whole photo")
                                                     : qsTr("pinch to zoom, hold for original")
                    color: FiatImagoTheme.secondaryText
                    font.pixelSize: Theme.fontSizeTiny
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    size: BusyIndicatorSize.Medium
                    running: dev.busy
                }

                Label {
                    anchors.centerIn: parent
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: !dev.busy && !dev.loaded && dev.message !== ""
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: dev.message
                    color: FiatImagoTheme.secondaryText
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: dev.loaded && dev.message !== ""
                wrapMode: Text.WordWrap
                text: dev.message
                color: FiatImagoTheme.secondaryText
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            WordChoice {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                choices: [
                    { label: qsTr("looks"), value: "looks" },
                    { label: qsTr("light"), value: "light" },
                    { label: qsTr("colour"), value: "colour" },
                    { label: qsTr("crop"), value: "crop" },
                    { label: qsTr("detail"), value: "detail" },
                    { label: qsTr("effects"), value: "effects" }
                ]
                current: page.tool
                onChosen: page.tool = value
            }

            Item { width: 1; height: Theme.paddingMedium }

            Column {
                width: parent.width
                visible: page.tool === "looks"
                spacing: Theme.paddingSmall

                SectionLabel {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    horizontalAlignment: Text.AlignRight
                    text: qsTr("Built in")
                }
                LookGrid {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    choices: imagoPresets.builtIn
                    current: page.currentLook()
                    onChosen: dev.applyLook(value, 100)
                }

                AdjustSlider {
                    visible: page.currentLook() !== "builtin:none"
                    label: qsTr("Strength")
                    minimumValue: 0
                    maximumValue: 150
                    defaultValue: 100
                    signedValue: false
                    unit: "%"
                    value: dev.recipe.lookStrength === undefined ? 100 : dev.recipe.lookStrength
                    onMoved: dev.applyLook(page.currentLook(), newValue)
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: page.currentLook() !== "builtin:none"
                    wrapMode: Text.WordWrap
                    text: qsTr("Moving the strength starts again from the look, so set it before fine-tuning.")
                    color: FiatImagoTheme.secondaryText
                    font.pixelSize: Theme.fontSizeExtraSmall
                }

                Item { width: 1; height: Theme.paddingMedium }

                SectionLabel {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    horizontalAlignment: Text.AlignRight
                    text: qsTr("Yours")
                }

                Repeater {
                    model: imagoPresets.user

                    ListItem {
                        id: userLook
                        readonly property string lookId: modelData.id
                        readonly property string lookName: modelData.name

                        width: parent.width
                        contentHeight: Theme.itemSizeSmall
                        highlightedColor: FiatImagoTheme.highlightWash
                        onClicked: dev.applyLook(lookId, 100)

                        Label {
                            x: Theme.horizontalPageMargin
                            width: parent.width - 2 * Theme.horizontalPageMargin
                            anchors.verticalCenter: parent.verticalCenter
                            truncationMode: TruncationMode.Fade
                            text: userLook.lookName
                            color: FiatImagoTheme.primaryText
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: page.currentLook() === userLook.lookId
                        }

                        menu: ContextMenu {
                            MenuItem {
                                text: qsTr("Delete")
                                onClicked: page.deleteLook(userLook.lookId, userLook.lookName)
                            }
                        }
                    }
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: imagoPresets.user.length === 0
                    wrapMode: Text.WordWrap
                    text: qsTr("Nothing saved yet. Save the settings of a photo you like with Save look in the pull-down menu.")
                    color: FiatImagoTheme.secondaryText
                    font.pixelSize: Theme.fontSizeExtraSmall
                }
            }

            Column {
                width: parent.width
                visible: page.tool === "light"

                AdjustSlider {
                    label: qsTr("Exposure")
                    minimumValue: dev.isRaw ? -4 : -2
                    maximumValue: dev.isRaw ? 4 : 2
                    stepSize: 0.05
                    decimals: 2
                    unit: "EV"
                    value: page.value("exposure")
                    onMoved: dev.set("exposure", newValue)
                }
                AdjustSlider {
                    label: qsTr("Contrast")
                    value: page.value("contrast")
                    onMoved: dev.set("contrast", newValue)
                }
                AdjustSlider {
                    label: qsTr("Highlights")
                    value: page.value("highlights")
                    onMoved: dev.set("highlights", newValue)
                }
                AdjustSlider {
                    label: qsTr("Shadows")
                    value: page.value("shadows")
                    onMoved: dev.set("shadows", newValue)
                }
                AdjustSlider {
                    label: qsTr("Whites")
                    value: page.value("whites")
                    onMoved: dev.set("whites", newValue)
                }
                AdjustSlider {
                    label: qsTr("Blacks")
                    value: page.value("blacks")
                    onMoved: dev.set("blacks", newValue)
                }

                LinkText {
                    x: Theme.horizontalPageMargin - Theme.paddingSmall
                    text: dev.showClipping ? qsTr("hide blown highlights") : qsTr("show blown highlights in red")
                    color: FiatImagoTheme.primaryText
                    onClicked: dev.showClipping = !dev.showClipping
                }
            }

            Column {
                width: parent.width
                visible: page.tool === "colour"

                WordChoice {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    choices: [
                        { label: qsTr("basic"), value: "basic" },
                        { label: qsTr("mixer"), value: "mixer" }
                    ]
                    current: page.colourView
                    onChosen: page.colourView = value
                }

                Column {
                    width: parent.width
                    visible: page.colourView === "basic"

                    AdjustSlider {
                        label: qsTr("Temperature")
                        value: page.value("temperature")
                        onMoved: dev.set("temperature", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Tint")
                        value: page.value("tint")
                        onMoved: dev.set("tint", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Saturation")
                        value: page.value("saturation")
                        onMoved: dev.set("saturation", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Vibrance")
                        value: page.value("vibrance")
                        onMoved: dev.set("vibrance", newValue)
                    }
                }

                Column {
                    width: parent.width
                    visible: page.colourView === "mixer"

                    WordChoice {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        choices: [
                            { label: qsTr("red"), value: "Red" },
                            { label: qsTr("orange"), value: "Orange" },
                            { label: qsTr("yellow"), value: "Yellow" },
                            { label: qsTr("green"), value: "Green" },
                            { label: qsTr("aqua"), value: "Aqua" },
                            { label: qsTr("blue"), value: "Blue" },
                            { label: qsTr("purple"), value: "Purple" },
                            { label: qsTr("magenta"), value: "Magenta" }
                        ]
                        current: page.mixBand
                        onChosen: page.mixBand = value
                    }

                    AdjustSlider {
                        label: qsTr("Hue")
                        minimumValue: -90
                        maximumValue: 90
                        unit: "°"
                        value: page.value("hue" + page.mixBand)
                        onMoved: dev.set("hue" + page.mixBand, newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Saturation")
                        value: page.value("sat" + page.mixBand)
                        onMoved: dev.set("sat" + page.mixBand, newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Lightness")
                        value: page.value("lum" + page.mixBand)
                        onMoved: dev.set("lum" + page.mixBand, newValue)
                    }

                    LinkText {
                        x: Theme.horizontalPageMargin - Theme.paddingSmall
                        text: qsTr("reset mixer")
                        color: FiatImagoTheme.primaryText
                        onClicked: page.resetMixer()
                    }
                }
            }

            Column {
                width: parent.width
                visible: page.tool === "crop"

                WordChoice {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    choices: [
                        { label: qsTr("free"), value: "free" },
                        { label: qsTr("original"), value: "original" },
                        { label: "1:1", value: "1:1" },
                        { label: "4:3", value: "4:3" },
                        { label: "3:2", value: "3:2" },
                        { label: "16:9", value: "16:9" }
                    ]
                    current: dev.recipe.aspect === undefined ? "original" : dev.recipe.aspect
                    onChosen: page.chooseAspect(value)
                }

                AdjustSlider {
                    label: qsTr("Straighten")
                    minimumValue: -45
                    maximumValue: 45
                    stepSize: 0.1
                    decimals: 1
                    unit: "°"
                    value: page.value("straighten")
                    onMoved: dev.set("straighten", newValue)
                }
                AdjustSlider {
                    label: qsTr("Vertical perspective")
                    value: page.value("perspectiveV")
                    onMoved: dev.set("perspectiveV", newValue)
                }
                AdjustSlider {
                    label: qsTr("Horizontal perspective")
                    value: page.value("perspectiveH")
                    onMoved: dev.set("perspectiveH", newValue)
                }

                Row {
                    x: Theme.horizontalPageMargin - Theme.paddingSmall
                    spacing: Theme.paddingLarge

                    LinkText {
                        text: qsTr("rotate 90°")
                        color: FiatImagoTheme.primaryText
                        onClicked: dev.set("rotation", (page.value("rotation") + 1) % 4)
                    }
                    LinkText {
                        text: qsTr("flip")
                        color: FiatImagoTheme.primaryText
                        onClicked: dev.set("flip", !dev.recipe.flip)
                    }
                    LinkText {
                        text: qsTr("reset crop")
                        color: FiatImagoTheme.primaryText
                        onClicked: {
                            dev.set("straighten", 0)
                            dev.set("perspectiveV", 0)
                            dev.set("perspectiveH", 0)
                            page.chooseAspect("original")
                            dev.setCrop(0, 0, 1, 1)
                        }
                    }
                }
            }

            Column {
                width: parent.width
                visible: page.tool === "detail"

                AdjustSlider {
                    label: qsTr("Sharpen")
                    minimumValue: 0
                    maximumValue: 100
                    signedValue: false
                    value: page.value("sharpenAmount")
                    onMoved: dev.set("sharpenAmount", newValue)
                }
                AdjustSlider {
                    label: qsTr("Radius")
                    minimumValue: 0.5
                    maximumValue: 3
                    defaultValue: 1
                    stepSize: 0.1
                    decimals: 1
                    signedValue: false
                    value: dev.recipe.sharpenRadius === undefined ? 1 : dev.recipe.sharpenRadius
                    onMoved: dev.set("sharpenRadius", newValue)
                }
                AdjustSlider {
                    label: qsTr("Colour noise")
                    minimumValue: 0
                    maximumValue: 100
                    signedValue: false
                    value: page.value("noiseChroma")
                    onMoved: dev.set("noiseChroma", newValue)
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.WordWrap
                    text: qsTr("Sharpening and noise only show at 100 %, so the loupe stays on that scale while you drag. Tap the photo to move it.")
                    color: FiatImagoTheme.secondaryText
                    font.pixelSize: Theme.fontSizeExtraSmall
                }
            }

            Column {
                width: parent.width
                visible: page.tool === "effects"

                WordChoice {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    choices: [
                        { label: qsTr("vignette"), value: "vignette" },
                        { label: qsTr("toning"), value: "toning" },
                        { label: qsTr("pattern"), value: "pattern" },
                        { label: qsTr("blur"), value: "blur" }
                    ]
                    current: page.effectsView
                    onChosen: page.effectsView = value
                }

                Column {
                    width: parent.width
                    visible: page.effectsView === "vignette"

                    AdjustSlider {
                        label: qsTr("Vignette")
                        value: page.value("vignetteAmount")
                        onMoved: dev.set("vignetteAmount", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Midpoint")
                        minimumValue: 0
                        maximumValue: 100
                        defaultValue: 50
                        signedValue: false
                        value: dev.recipe.vignetteMidpoint === undefined ? 50 : dev.recipe.vignetteMidpoint
                        onMoved: dev.set("vignetteMidpoint", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Feather")
                        minimumValue: 0
                        maximumValue: 100
                        defaultValue: 50
                        signedValue: false
                        value: dev.recipe.vignetteFeather === undefined ? 50 : dev.recipe.vignetteFeather
                        onMoved: dev.set("vignetteFeather", newValue)
                    }
                }

                Column {
                    width: parent.width
                    visible: page.effectsView === "toning"

                    AdjustSlider {
                        label: qsTr("Shadows hue")
                        minimumValue: 0
                        maximumValue: 360
                        signedValue: false
                        unit: "°"
                        value: page.value("toneShadowHue")
                        onMoved: dev.set("toneShadowHue", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Shadows strength")
                        minimumValue: 0
                        maximumValue: 100
                        signedValue: false
                        value: page.value("toneShadowAmount")
                        onMoved: dev.set("toneShadowAmount", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Highlights hue")
                        minimumValue: 0
                        maximumValue: 360
                        signedValue: false
                        unit: "°"
                        value: page.value("toneHighlightHue")
                        onMoved: dev.set("toneHighlightHue", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Highlights strength")
                        minimumValue: 0
                        maximumValue: 100
                        signedValue: false
                        value: page.value("toneHighlightAmount")
                        onMoved: dev.set("toneHighlightAmount", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Balance")
                        value: page.value("toneBalance")
                        onMoved: dev.set("toneBalance", newValue)
                    }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.WordWrap
                        text: qsTr("For sepia, take the saturation down first, then warm both sides.")
                        color: FiatImagoTheme.secondaryText
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                }

                Column {
                    width: parent.width
                    visible: page.effectsView === "pattern"

                    AdjustSlider {
                        label: qsTr("Pattern")
                        minimumValue: 0
                        maximumValue: 100
                        signedValue: false
                        value: page.value("patternAmount")
                        onMoved: dev.set("patternAmount", newValue)
                    }
                    AdjustSlider {
                        label: qsTr("Size")
                        minimumValue: 30
                        maximumValue: 150
                        defaultValue: 80
                        signedValue: false
                        value: dev.recipe.patternSize === undefined ? 80 : dev.recipe.patternSize
                        onMoved: dev.set("patternSize", newValue)
                    }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.WordWrap
                        text: qsTr("The triangles Sailfish draws on its own backgrounds. Size is how many fit across the photo.")
                        color: FiatImagoTheme.secondaryText
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                }

                Column {
                    width: parent.width
                    visible: page.effectsView === "blur"

                    AdjustSlider {
                        label: qsTr("Blur")
                        minimumValue: 0
                        maximumValue: 100
                        signedValue: false
                        value: page.value("blur")
                        onMoved: dev.set("blur", newValue)
                    }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator { }
    }
}
