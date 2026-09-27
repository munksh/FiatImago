pragma Singleton
import QtQuick 2.0
import Sailfish.Silica 1.0

QtObject {
    id: t

    // fiat imago has one palette and no colour switch: a neutral grey, so the
    // photograph is the only colour on the screen. The family names are kept
    // so shared components work unchanged; the accent is the text colour.
    readonly property bool ambient: false
    readonly property bool dark: true
    readonly property string serif: "Georgia"

    // ---- the notch ----
    function cutoutHeight() {
        if (typeof Screen === "undefined" || Screen === null) return -1
        var c = Screen.topCutout
        if (c === undefined || c === null) return -1
        if (typeof c === "number") return c
        if (c.height !== undefined) return c.height
        return -1
    }
    readonly property real headerTopInsetFallback: Theme.paddingLarge * 1.5
    readonly property real headerTopInset: {
        var c = cutoutHeight()
        return c >= 0 ? c + Theme.paddingMedium : headerTopInsetFallback
    }
    // Written down, not derived. Two derivations walked the wordmark up the screen.
    readonly property real statusRowCenter: Theme.itemSizeLarge / 2

    // ---- text and accent ----
    readonly property color primaryText: "#E6E6E6"
    readonly property color secondaryText: Qt.rgba(0.902, 0.902, 0.902, 0.6)
    readonly property color accent: primaryText

    function mixColor(a, b, f) {
        return Qt.rgba(a.r * (1 - f) + b.r * f, a.g * (1 - f) + b.g * f, a.b * (1 - f) + b.b * f, 1.0)
    }
    readonly property color chromeAccent: primaryText

    // ---- paper and surfaces ----
    readonly property color paper: "#262626"
    readonly property color backgroundHigh: paper
    readonly property color backgroundLow: paper
    readonly property color card: "#303030"
    readonly property color surface: card
    readonly property color cardBorder: Theme.rgba(primaryText, 0.45)
    readonly property color innerBorder: Theme.rgba(primaryText, 0.22)
    readonly property color recessFill: Theme.rgba(primaryText, 0.05)
    readonly property color recessBorder: Theme.rgba(primaryText, 0.16)
    readonly property real cardRadius: Theme.paddingLarge * 2
    readonly property int cardBorderWidth: 2

    readonly property color pillFill: Theme.rgba(primaryText, 0.15)
    readonly property color pillBorder: Theme.rgba(primaryText, 0.55)
    readonly property color pillFillActive: Theme.rgba(accent, 0.15)
    readonly property color pillBorderActive: Theme.rgba(accent, 0.45)

    readonly property color dotIdle: Theme.rgba(primaryText, 0.22)
    readonly property color highlightWash: Theme.rgba(primaryText, 0.10)

    // A function, not a chain of readonly bindings: the chained version came
    // out undefined on the device, and an undefined colour renders black.
    function markOn(c) {
        if (c === undefined || c === null) return "#F5F5F5"
        return (c.r * 0.299 + c.g * 0.587 + c.b * 0.114) > 0.55 ? "#1A1A1A" : "#F5F5F5"
    }
    readonly property color onAccent: markOn(accent)

    // Munkstolen's mark in imago's grey: 4.5:1 on the paper.
    readonly property color makerMark: "#8C8C8C"

    readonly property color wrong: primaryText
    readonly property real markSize: Theme.itemSizeSmall * 0.6

    function applyPalette(item) {
        if (item === null || item === undefined) return
        var p = item.palette
        if (p === undefined || p === null) return
        try { p.colorScheme = Theme.LightOnDark } catch (e) { }
        try { p.primaryColor = primaryText } catch (e) { }
        try { p.secondaryColor = secondaryText } catch (e) { }
        try { p.highlightColor = chromeAccent } catch (e) { }
        try { p.secondaryHighlightColor = Theme.rgba(chromeAccent, 0.6) } catch (e) { }
        try { p.highlightBackgroundColor = Theme.rgba(primaryText, 0.12) } catch (e) { }
        try { p.errorColor = wrong } catch (e) { }
        try { p.highlightDimmerColor = paper } catch (e) { }
        try { p.overlayBackgroundColor = paper } catch (e) { }
    }

    // ---- cover geometry, identical in every Fiat app ----
    readonly property real coverWordmarkTop: Theme.paddingLarge
    readonly property real coverSideMargin: Theme.paddingLarge
    readonly property real coverFigureFraction: 0.28
    readonly property real coverFigureFractionShape: 0.20
    readonly property int coverFigureSize: Theme.fontSizeHuge
    readonly property real coverArtFraction: 0.5
}
