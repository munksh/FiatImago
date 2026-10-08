TARGET = harbour-fiatimago

CONFIG += sailfishapp c++14
QT += quick

isEmpty(APP_VERSION) {
    APP_VERSION = 0.0.0-dev
}
DEFINES += APP_VERSION=\\\"$$APP_VERSION\\\"

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3
LIBS += -lpthread

HEADERS += \
    src/imaging.h \
    src/rawmeta.h \
    src/imagesource.h \
    src/recipe.h \
    src/pipeline.h \
    src/recipestore.h \
    src/presetstore.h \
    src/library.h \
    src/thumbnailprovider.h \
    src/exporter.h \
    src/developer.h \
    src/developview.h

SOURCES += \
    src/main.cpp \
    src/imaging.cpp \
    src/rawmeta.cpp \
    src/imagesource.cpp \
    src/recipe.cpp \
    src/pipeline.cpp \
    src/recipestore.cpp \
    src/presetstore.cpp \
    src/library.cpp \
    src/thumbnailprovider.cpp \
    src/exporter.cpp \
    src/developer.cpp \
    src/developview.cpp

DISTFILES += \
    qml/harbour-fiatimago.qml \
    qml/qmldir \
    qml/FiatImagoTheme.qml \
    qml/cover/CoverPage.qml \
    qml/pages/LibraryPage.qml \
    qml/pages/EditPage.qml \
    qml/pages/InspectPage.qml \
    qml/pages/ExportPage.qml \
    qml/pages/SaveLookPage.qml \
    qml/pages/AboutPage.qml \
    qml/pages/images/family/*.png \
    qml/components/AdjustSlider.qml \
    qml/components/CropOverlay.qml \
    qml/components/Histogram.qml \
    qml/components/LookGrid.qml \
    qml/components/PageHead.qml \
    qml/components/SectionLabel.qml \
    qml/components/MunkstolenMark.qml \
    qml/components/WordChoice.qml \
    qml/components/LinkText.qml \
    qml/components/FiatButton.qml \
    rpm/harbour-fiatimago.spec \
    CHANGELOG.md \
    harbour-fiatimago.desktop \
    LICENSE

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# The shared components, the family icons and the LICENSE come from Fiat Lux
# through tools/copy-family-parts.sh. Without them the app is a white screen.
REQUIRED_FILES = \
    $${TARGET}.desktop \
    qml/$${TARGET}.qml \
    qml/qmldir \
    qml/FiatImagoTheme.qml \
    qml/components/PageHead.qml \
    qml/components/SectionLabel.qml \
    qml/components/MunkstolenMark.qml \
    qml/components/WordChoice.qml \
    qml/components/LinkText.qml \
    qml/components/FiatButton.qml \
    qml/pages/images/family/harbour-fiatimago.png \
    qml/pages/images/family/harbour-fiatagenda.png \
    qml/pages/images/family/harbour-fiatcor.png \
    qml/pages/images/family/harbour-fiatglossa.png \
    qml/pages/images/family/harbour-fiatlux.png \
    qml/pages/images/family/harbour-fiatmargo.png \
    qml/pages/images/family/harbour-fiatmos.png \
    qml/pages/images/family/harbour-fiatpons.png \
    qml/pages/images/family/harbour-fiatpassus.png \
    qml/pages/images/family/harbour-fiatratio.png \
    qml/pages/images/family/harbour-fiatvox.png \
    LICENSE

for(f, REQUIRED_FILES) {
    !exists($$PWD/$$f): error("Missing $$f -- expected it at $$PWD/$$f (run tools/copy-family-parts.sh)")
}

licensefile.files = $$PWD/LICENSE
licensefile.path = /usr/share/$${TARGET}
INSTALLS += licensefile
