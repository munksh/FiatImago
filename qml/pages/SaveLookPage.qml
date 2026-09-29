import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."
import "../components"

Page {
    id: page

    property QtObject developer
    property string name: ""

    allowedOrientations: Orientation.Portrait

    function paint() { FiatImagoTheme.applyPalette(page) }

    function save() {
        var n = lookName.text.trim()
        if (n === "" || !page.developer) return
        var id = imagoPresets.save(n, page.developer.lookSettings())
        if (id === "") return
        page.developer.set("look", id)
        page.developer.set("lookStrength", 100)
        pageStack.pop()
    }

    Component.onCompleted: {
        paint()
        lookName.forceActiveFocus()
    }

    Rectangle {
        anchors.fill: parent
        color: FiatImagoTheme.paper
    }

    Column {
        width: parent.width
        spacing: Theme.paddingMedium

        PageHead {
            title: qsTr("save look")
            subtitle: page.name
        }

        TextField {
            id: lookName
            width: parent.width
            label: qsTr("Name")
            placeholderText: qsTr("Name")
            EnterKey.enabled: text.trim() !== ""
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: page.save()
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.WordWrap
            text: qsTr("A look keeps light, colour, detail and effects from this photo, never the crop.")
            color: FiatImagoTheme.secondaryText
            font.pixelSize: Theme.fontSizeExtraSmall
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            visible: lookName.text.trim() !== "" && imagoPresets.exists(lookName.text)
            wrapMode: Text.WordWrap
            text: qsTr("Replaces your look with the same name.")
            color: FiatImagoTheme.primaryText
            font.pixelSize: Theme.fontSizeExtraSmall
        }
    }

    FiatButton {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.paddingLarge
        filled: true
        enabled: lookName.text.trim() !== ""
        text: qsTr("save")
        onClicked: page.save()
    }
}
