import QtQuick 2.0
import Sailfish.Silica 1.0
import "."
import "pages"

ApplicationWindow {
    id: app

    initialPage: Component { LibraryPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: Orientation.Portrait
    _defaultPageOrientations: Orientation.Portrait

    Component.onCompleted: FiatImagoTheme.applyPalette(app)
}
