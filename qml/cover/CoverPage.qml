// Shows the photo being developed, so the tile says where you left off.
// No actions: every edit needs the full screen.
import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

CoverBackground {
    id: cover

    // ---- state ----
    readonly property var current: imagoLibrary.current
    readonly property bool hasPhoto: current !== undefined && current !== null
                                     && current.thumb !== undefined && current.thumb !== ""

    // ---- paper ----
    Rectangle {
        anchors.fill: parent
        color: FiatImagoTheme.paper
    }

    // ---- wordmark ----
    Label {
        anchors.top: parent.top
        anchors.topMargin: FiatImagoTheme.coverWordmarkTop
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
        text: "fiat imago"
        color: FiatImagoTheme.secondaryText
        font.pixelSize: Theme.fontSizeTiny
        font.family: FiatImagoTheme.serif
        font.italic: true
    }

    // ---- figure ----
    Column {
        y: cover.height * FiatImagoTheme.coverFigureFractionShape
        width: parent.width
        spacing: Theme.paddingSmall
        visible: cover.hasPhoto

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            width: cover.width * FiatImagoTheme.coverArtFraction
            height: width
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            sourceSize.width: 256
            source: cover.hasPhoto ? cover.current.thumb : ""
        }
        Label {
            x: FiatImagoTheme.coverSideMargin
            width: parent.width - 2 * FiatImagoTheme.coverSideMargin
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
            text: cover.hasPhoto && cover.current.name !== undefined ? cover.current.name : ""
            color: FiatImagoTheme.secondaryText
            font.pixelSize: Theme.fontSizeExtraSmall
        }
    }

    Label {
        y: cover.height * FiatImagoTheme.coverFigureFractionShape
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        visible: !cover.hasPhoto
        text: qsTr("no photo")
        color: FiatImagoTheme.secondaryText
        font.pixelSize: Theme.fontSizeSmall
        font.family: FiatImagoTheme.serif
        font.italic: true
    }

    // ---- cover actions ----
}
