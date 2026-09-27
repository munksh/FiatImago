import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import ".."
import "../components"

Page {
    id: page

    property QtObject developer
    property string name: ""
    property bool hasCameraData: true
    property string savedPath: ""
    property string failure: ""

    allowedOrientations: Orientation.Portrait

    function paint() { FiatImagoTheme.applyPalette(page) }
    Component.onCompleted: paint()

    ConfigurationValue {
        id: sizeSetting
        key: "/apps/harbour-fiatimago/exportSize"
        defaultValue: "full"
    }
    ConfigurationValue {
        id: qualitySetting
        key: "/apps/harbour-fiatimago/exportQuality"
        defaultValue: 92
    }
    ConfigurationValue {
        id: cameraDataSetting
        key: "/apps/harbour-fiatimago/exportCameraData"
        defaultValue: "keep"
    }

    Connections {
        target: page.developer
        onExportFinished: {
            page.failure = ""
            page.savedPath = path
        }
        onExportFailed: page.failure = message
    }

    Rectangle {
        anchors.fill: parent
        color: FiatImagoTheme.paper
    }

    Column {
        width: parent.width
        spacing: Theme.paddingMedium

        PageHead {
            title: qsTr("export")
            subtitle: page.name
        }

        Label {
            x: Theme.horizontalPageMargin
            text: qsTr("Size")
            color: FiatImagoTheme.secondaryText
            font.pixelSize: Theme.fontSizeExtraSmall
        }
        WordChoice {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            choices: [
                { label: qsTr("full"), value: "full" },
                { label: qsTr("half"), value: "half" }
            ]
            current: sizeSetting.value
            onChosen: sizeSetting.value = value
        }

        AdjustSlider {
            label: qsTr("Quality")
            minimumValue: 60
            maximumValue: 100
            defaultValue: 92
            signedValue: false
            value: Number(qualitySetting.value)
            onMoved: qualitySetting.value = Math.round(newValue)
        }

        Label {
            x: Theme.horizontalPageMargin
            visible: page.hasCameraData
            text: qsTr("Camera data")
            color: FiatImagoTheme.secondaryText
            font.pixelSize: Theme.fontSizeExtraSmall
        }
        WordChoice {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            visible: page.hasCameraData
            choices: [
                { label: qsTr("keep"), value: "keep" },
                { label: qsTr("remove"), value: "remove" }
            ]
            current: cameraDataSetting.value
            onChosen: cameraDataSetting.value = value
        }

        Label {
            x: Theme.horizontalPageMargin
            text: qsTr("Saved to")
            color: FiatImagoTheme.secondaryText
            font.pixelSize: Theme.fontSizeExtraSmall
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.WordWrap
            text: page.savedPath !== ""
                  ? "Pictures/fiat imago/" + page.savedPath.substring(page.savedPath.lastIndexOf("/") + 1)
                  : "Pictures/fiat imago"
            color: FiatImagoTheme.primaryText
            font.pixelSize: Theme.fontSizeSmall
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            visible: page.failure !== ""
            wrapMode: Text.WordWrap
            text: page.failure
            color: FiatImagoTheme.primaryText
            font.pixelSize: Theme.fontSizeSmall
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: page.developer !== null && page.developer.exporting
    }

    FiatButton {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.paddingLarge
        filled: true
        enabled: page.developer !== null && !page.developer.exporting
        text: page.savedPath !== "" ? qsTr("done") : qsTr("export")
        onClicked: {
            if (page.savedPath !== "") {
                pageStack.pop()
                return
            }
            page.failure = ""
            page.developer.exportImage(sizeSetting.value === "half",
                                       Number(qualitySetting.value),
                                       page.hasCameraData && cameraDataSetting.value === "keep")
        }
    }
}
