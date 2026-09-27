import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."
import "../components"

Page {
    id: page

    allowedOrientations: Orientation.Portrait

    function paint() { FiatImagoTheme.applyPalette(page) }
    Component.onCompleted: paint()

    Rectangle {
        anchors.fill: parent
        color: FiatImagoTheme.paper
    }

    SilicaGridView {
        id: grid

        anchors.fill: parent
        model: imagoLibrary
        cellWidth: width / 3
        cellHeight: Math.round(cellWidth * 4 / 3)

        PullDownMenu {
            highlightColor: FiatImagoTheme.chromeAccent

            MenuItem {
                text: qsTr("About")
                color: FiatImagoTheme.primaryText
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Refresh")
                color: FiatImagoTheme.primaryText
                onClicked: imagoLibrary.refresh()
            }
        }

        header: Column {
            width: grid.width

            Item {
                width: parent.width
                height: FiatImagoTheme.statusRowCenter + wordmark.height / 2 + Theme.paddingMedium
                Text {
                    id: wordmark
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.top: parent.top
                    anchors.topMargin: Math.max(0, FiatImagoTheme.statusRowCenter - height / 2)
                    text: "fiat imago"
                    color: FiatImagoTheme.primaryText
                    font.pixelSize: Theme.fontSizeLarge
                    font.family: FiatImagoTheme.serif
                    font.italic: true
                }
            }

            WordChoice {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                choices: [
                    { label: qsTr("all"), value: "all" },
                    { label: qsTr("raw"), value: "raw" },
                    { label: qsTr("jpeg"), value: "jpeg" },
                    { label: qsTr("edited"), value: "edited" }
                ]
                current: imagoLibrary.filter
                onChosen: imagoLibrary.filter = value
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        delegate: BackgroundItem {
            id: cell

            width: grid.cellWidth
            height: grid.cellHeight
            highlightedColor: FiatImagoTheme.highlightWash

            Image {
                id: thumb
                anchors.fill: parent
                anchors.margins: Math.round(Theme.paddingSmall / 2)
                fillMode: Image.PreserveAspectCrop
                clip: true
                asynchronous: true
                sourceSize.width: grid.cellHeight
                source: model.thumb === undefined ? "" : model.thumb
            }

            Rectangle {
                anchors.left: thumb.left
                anchors.bottom: thumb.bottom
                anchors.margins: Theme.paddingSmall
                visible: model.hasRaw === true
                width: rawLabel.width + Theme.paddingSmall * 2
                height: rawLabel.height
                color: Theme.rgba(FiatImagoTheme.paper, 0.75)
                Label {
                    id: rawLabel
                    anchors.centerIn: parent
                    text: "RAW"
                    color: FiatImagoTheme.primaryText
                    font.pixelSize: Theme.fontSizeTiny
                    font.bold: true
                }
            }

            Rectangle {
                anchors.right: thumb.right
                anchors.bottom: thumb.bottom
                anchors.margins: Theme.paddingMedium
                visible: model.edited === true
                width: Theme.paddingMedium
                height: width
                radius: width / 2
                color: FiatImagoTheme.primaryText
                border.color: FiatImagoTheme.paper
                border.width: 1
            }

            onClicked: pageStack.push(Qt.resolvedUrl("EditPage.qml"), {
                itemKey: model.key,
                name: model.name,
                shortName: model.shortName,
                jpegPath: model.jpegPath,
                rawPath: model.rawPath,
                jsonPath: model.jsonPath,
                altJsonPath: model.altJsonPath,
                thumb: model.thumb
            })
        }

        VerticalScrollDecorator { }
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        visible: imagoLibrary.count === 0
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: imagoLibrary.filter === "edited" ? qsTr("Nothing edited yet.")
            : imagoLibrary.filter === "raw" ? qsTr("No RAW files in Pictures.\nRAWfish saves them there.")
            : qsTr("No photos in Pictures yet.")
        color: FiatImagoTheme.secondaryText
        font.pixelSize: Theme.fontSizeMedium
        font.family: FiatImagoTheme.serif
        font.italic: true
    }
}
