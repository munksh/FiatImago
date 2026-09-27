import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

// The house page header. Right-aligned title, optional line under it.
//
// Not Silica's PageHeader, which draws its title in Theme.highlightColor --
// light text on light paper under Fiat colours plus a dark ambience.
//
// The title sits ON THE STATUS ROW, centred on the same line as the wordmark
// on the meter page, so moving between pages the eye finds every title at the
// same height. The line under it hangs below.
//
// The row is shared with the cutout, so the title is kept to the right of it:
// its width is capped at the space between the cutout and the right margin.
// A long title first shrinks a little, then fades, and never runs into the
// hole.

Item {
    id: root

    property string title: ""
    property string subtitle: ""

    width: parent ? parent.width : 0
    height: Math.max(FiatImagoTheme.statusRowCenter * 2, col.y + col.height) + Theme.paddingMedium

    // Half the cutout's width, if the platform will say; else a safe guess.
    readonly property real cutoutHalf: {
        if (typeof Screen === "undefined" || Screen === null) return Theme.itemSizeExtraSmall
        var c = Screen.topCutout
        if (c !== undefined && c !== null && c.width !== undefined && c.width > 0) return c.width / 2
        return Theme.itemSizeExtraSmall
    }
    readonly property real titleRoom: Math.max(Theme.itemSizeHuge,
        width / 2 - cutoutHalf - Theme.paddingLarge - Theme.horizontalPageMargin)

    Column {
        id: col
        anchors.right: parent.right
        anchors.rightMargin: Theme.horizontalPageMargin
        y: Math.max(0, FiatImagoTheme.statusRowCenter - titleLabel.height / 2)
        width: root.titleRoom

        Label {
            id: titleLabel
            width: parent.width
            horizontalAlignment: Text.AlignRight
            // A long title shrinks until it fits beside the cutout. No fade.
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: Theme.fontSizeExtraSmall
            text: root.title
            // A little smaller than the wordmark, as a page title under a
            // name should be, and small enough that every title in the app
            // fits beside the cutout at the same size.
            font.pixelSize: Math.round(Theme.fontSizeLarge * 0.85)
            font.family: FiatImagoTheme.serif
            color: FiatImagoTheme.primaryText
        }

        Label {
            width: parent.width
            visible: root.subtitle !== ""
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.Wrap
            text: root.subtitle
            font.pixelSize: Theme.fontSizeExtraSmall
            color: FiatImagoTheme.secondaryText
        }
    }
}
