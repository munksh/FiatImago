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
    }

    Rectangle {
        anchors.fill: parent
        color: FiatImagoTheme.paper
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: Math.max(page.height, column.height)

        PullDownMenu {
            highlightColor: FiatImagoTheme.chromeAccent

            MenuItem {
                text: qsTr("Reset all")
                color: FiatImagoTheme.primaryText
                onClicked: dev.resetAll()
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
                text: qsTr("Export")
                color: FiatImagoTheme.primaryText
                onClicked: pageStack.push(Qt.resolvedUrl("ExportPage.qml"), {
                    developer: dev,
                    name: page.shortName,
                    hasCameraData: page.jpegPath !== ""
                })
            }
        }

        Column {
            id: column
            width: parent.width

            PageHead {
                title: page.shortName
                subtitle: dev.loaded ? (dev.isRaw ? "raw" : "jpeg") : ""
            }

            Item {
                id: stage
                width: parent.width
                height: Math.round(page.height * 0.5)

                DevelopView {
                    id: view
                    anchors.fill: parent
                    anchors.leftMargin: Theme.paddingMedium
                    anchors.rightMargin: Theme.paddingMedium
                    anchors.topMargin: histogram.visible ? histogram.height + Theme.paddingSmall : 0
                    anchors.bottomMargin: Theme.paddingLarge
                    developer: dev
                    mode: page.holding ? "original" : "preview"
                }

                MouseArea {
                    anchors.fill: view
                    enabled: page.tool !== "crop" && dev.loaded
                    onPressAndHold: page.holding = true
                    onReleased: page.holding = false
                    onCanceled: page.holding = false
                    onClicked: {
                        var r = view.paintedRect
                        if (r.width <= 0 || r.height <= 0) return
                        var u = (mouse.x - r.x) / r.width
                        var v = (mouse.y - r.y) / r.height
                        if (u < 0 || u > 1 || v < 0 || v > 1) return
                        pageStack.push(Qt.resolvedUrl("InspectPage.qml"), { developer: dev, u: u, v: v })
                    }
                }

                CropOverlay {
                    anchors.fill: view
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
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.horizontalPageMargin
                    anchors.bottom: parent.bottom
                    visible: dev.loaded && page.tool !== "crop"
                    text: page.holding ? qsTr("original") : qsTr("hold for original")
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
            }

            Column {
                width: parent.width
                visible: page.tool === "colour"

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
                    minimumValue: -15
                    maximumValue: 15
                    stepSize: 0.1
                    decimals: 1
                    unit: "°"
                    value: page.value("straighten")
                    onMoved: dev.set("straighten", newValue)
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
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.WordWrap
                    text: qsTr("Tap the photo to judge sharpness at 100 %.")
                    color: FiatImagoTheme.secondaryText
                    font.pixelSize: Theme.fontSizeExtraSmall
                }
            }

            Column {
                width: parent.width
                visible: page.tool === "effects"

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
                AdjustSlider {
                    label: qsTr("Blur")
                    minimumValue: 0
                    maximumValue: 100
                    signedValue: false
                    value: page.value("blur")
                    onMoved: dev.set("blur", newValue)
                }
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator { }
    }
}
