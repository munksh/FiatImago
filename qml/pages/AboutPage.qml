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

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingMedium

            PageHead {
                title: qsTr("about")
                subtitle: "fiat imago"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeMedium
                font.family: FiatImagoTheme.serif
                color: FiatImagoTheme.primaryText
                text: qsTr("The camera saved a JPEG in a hurry. The RAW beside it still holds the highlights the JPEG threw away.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: FiatImagoTheme.secondaryText
                text: qsTr("fiat imago develops the RAW files RAWfish saves, and ordinary photos too: light, colour, crop, sharpness and vignette, changing while your finger moves. What you export is exactly what you saw, and the original is never touched. Every export is a new file.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: FiatImagoTheme.secondaryText
                text: qsTr("The app is grey on purpose. The only colour on the screen is your photograph, so nothing around it bends your judgement of it.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: FiatImagoTheme.secondaryText
                text: qsTr("The developing follows published methods: Malvar–He–Cutler demosaicing, and colour through the transform the camera itself recorded for each shot. The code was written with AI assistance and checked against test images with a known answer.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText
                font.pixelSize: Theme.fontSizeExtraSmall
                color: FiatImagoTheme.secondaryText
                text: qsTr("<b>fiat</b> — Latin, <i>let there be</i>. From <i>fiat lux</i> in the Vulgate: let there be light, and there was light. The first app took the phrase. The rest of the family kept the verb.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText
                font.pixelSize: Theme.fontSizeExtraSmall
                color: FiatImagoTheme.secondaryText
                text: qsTr("<b>imago</b> — image, likeness. A sensor only counts light; developing the count is what makes it an image.")
            }

            Item { width: 1; height: Theme.paddingLarge }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.itemSizeSmall
                height: 1
                color: FiatImagoTheme.innerBorder
            }
            Item { width: 1; height: Theme.paddingMedium }

            Column {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                spacing: Theme.paddingSmall

                Label {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: FiatImagoTheme.serif
                    font.italic: true
                    color: FiatImagoTheme.primaryText
                    text: "Imago animi vultus,\nindices oculi"
                }
                Label {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: FiatImagoTheme.secondaryText
                    text: qsTr("The face is the image of the mind, the eyes its interpreters.")
                }
                Label {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: FiatImagoTheme.secondaryText
                    text: "Cicero, De Oratore III.221"
                }
            }

            Item { width: 1; height: Theme.paddingMedium }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.itemSizeSmall
                height: 1
                color: FiatImagoTheme.innerBorder
            }

            SectionLabel { x: Theme.horizontalPageMargin; text: qsTr("Your data") }

            Label {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: FiatImagoTheme.secondaryText
                text: qsTr("Photos are read from Pictures, Pictures/Camera and Pictures/RAWfish, and from a memory card, and never changed. Your edits are kept in the app's own storage, not in the photos. Exports are saved to Pictures/fiat imago. Nothing leaves the phone: no account, no network.")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: content.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: FiatImagoTheme.secondaryText
                text: qsTr("Permissions: Pictures, to read your photos and save the exports, and removable media, to read a memory card.")
            }

            SectionLabel { x: Theme.horizontalPageMargin; text: qsTr("Made by") }

            Label {
                x: Theme.horizontalPageMargin
                text: "Munkstolen"
                font.pixelSize: Theme.fontSizeMedium
                font.family: FiatImagoTheme.serif
                color: FiatImagoTheme.primaryText
            }
            Label {
                x: Theme.horizontalPageMargin
                text: "Caesar Prometheus Ivarsson"
                font.pixelSize: Theme.fontSizeExtraSmall
                color: FiatImagoTheme.secondaryText
            }

            BackgroundItem {
                width: content.width
                height: Theme.itemSizeMedium
                highlightedColor: FiatImagoTheme.highlightWash
                onClicked: Qt.openUrlExternally("https://munkstolen.se")
                Column {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    Label {
                        text: "munkstolen.se"
                        color: FiatImagoTheme.primaryText
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    Label {
                        text: qsTr("Everything else I make")
                        color: FiatImagoTheme.secondaryText
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                }
            }

            BackgroundItem {
                width: content.width
                height: Theme.itemSizeMedium
                highlightedColor: FiatImagoTheme.highlightWash
                onClicked: Qt.openUrlExternally("https://github.com/munksh/FiatImago")
                Column {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    Label {
                        text: "github.com/munksh/FiatImago"
                        color: FiatImagoTheme.primaryText
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    Label {
                        text: qsTr("Source and issues · MIT licence")
                        color: FiatImagoTheme.secondaryText
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                }
            }

            SectionLabel { x: Theme.horizontalPageMargin; text: qsTr("The fiat family") }

            Repeater {
                model: [
                    { name: "fiat agenda", what: qsTr("let there be doing — a task list"), icon: "images/family/harbour-fiatagenda.png", url: "https://openrepos.net/content/munkstolen/fiat-agenda-task-list" },
                    { name: "fiat margo", what: qsTr("let there be edge — keeps edges"), icon: "images/family/harbour-fiatmargo.png", url: "https://openrepos.net/content/munkstolen/fiat-margo-keeps-edges" },
                    { name: "fiat glossa", what: qsTr("let there be tongue — a translator"), icon: "images/family/harbour-fiatglossa.png", url: "https://openrepos.net/content/munkstolen/fiat-glossa-a-deepl-translator" },
                    { name: "fiat vox", what: qsTr("let there be voice — a chromatic tuner"), icon: "images/family/harbour-fiatvox.png", url: "https://openrepos.net/content/munkstolen/fiat-vox-chromatic-tuner" },
                    { name: "fiat pons", what: qsTr("let there be bridge — a native Qobuz client"), icon: "images/family/harbour-fiatpons.png", url: "https://openrepos.net/content/munkstolen/fiat-pons-native-qobuz-client" },
                    { name: "fiat lux", what: qsTr("let there be light — a light meter for film - Coming soon"), icon: "images/family/harbour-fiatlux.png", url: "" },
                    { name: "fiat cor", what: qsTr("let there be heart — a metronome"), icon: "images/family/harbour-fiatcor.png", url: "https://openrepos.net/content/munkstolen/fiat-cor-a-metronome" },
                    { name: "fiat passus", what: qsTr("let there be step — a step counter - Coming soon"), icon: "images/family/harbour-fiatpassus.png", url: "" },
                    { name: "fiat mos", what: qsTr("let there be habit — a habit tracker"), icon: "images/family/harbour-fiatmos.png", url: "https://openrepos.net/content/munkstolen/fiat-mos-habit-tracker" },
                    { name: "fiat imago", what: qsTr("let there be image — this one"), icon: "images/family/harbour-fiatimago.png", url: "" }
                ]

                delegate: BackgroundItem {
                    width: content.width
                    height: Theme.itemSizeMedium
                    enabled: modelData.url !== ""
                    highlightedColor: FiatImagoTheme.highlightWash
                    onClicked: Qt.openUrlExternally(modelData.url)

                    Image {
                        id: familyIcon
                        x: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.itemSizeSmall
                        height: Theme.itemSizeSmall
                        sourceSize.width: Theme.itemSizeSmall
                        sourceSize.height: Theme.itemSizeSmall
                        fillMode: Image.PreserveAspectFit
                        source: modelData.icon
                    }
                    Column {
                        anchors.left: familyIcon.right
                        anchors.leftMargin: Theme.paddingLarge
                        anchors.right: parent.right
                        anchors.rightMargin: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        Label {
                            width: parent.width
                            text: modelData.name
                            color: modelData.url !== "" ? FiatImagoTheme.accent : FiatImagoTheme.primaryText
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: FiatImagoTheme.serif
                        }
                        Label {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: modelData.what
                            color: FiatImagoTheme.secondaryText
                            font.pixelSize: Theme.fontSizeExtraSmall
                        }
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                text: qsTr("Version %1").arg(typeof appVersion !== "undefined" ? appVersion : qsTr("unknown"))
                color: FiatImagoTheme.secondaryText
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            Item { width: 1; height: Theme.itemSizeExtraSmall }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.itemSizeSmall
                height: 1
                color: FiatImagoTheme.innerBorder
            }
            MunkstolenMark {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.itemSizeMedium
                frame: "ring"
                color: FiatImagoTheme.makerMark
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "munkstolen"
                color: FiatImagoTheme.makerMark
                font.pixelSize: Theme.fontSizeSmall
                font.family: FiatImagoTheme.serif
                font.italic: true
            }
        }

        VerticalScrollDecorator { }
    }
}
