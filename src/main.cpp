#include <QtQuick>
#include <sailfishapp.h>

#include "developer.h"
#include "developview.h"
#include "library.h"
#include "presetstore.h"
#include "recipestore.h"
#include "thumbnailprovider.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName(QStringLiteral("se.munkstolen"));
    app->setApplicationName(QStringLiteral("harbour-fiatimago"));

    qmlRegisterType<Developer>("harbour.fiatimago", 1, 0, "Developer");
    qmlRegisterType<DevelopView>("harbour.fiatimago", 1, 0, "DevelopView");

    RecipeStore store;
    PresetStore presets;
    Library library(&store);

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->engine()->addImageProvider(QStringLiteral("thumbs"), new ThumbnailProvider);
    view->rootContext()->setContextProperty(QStringLiteral("appVersion"), QString::fromUtf8(APP_VERSION));
    view->rootContext()->setContextProperty(QStringLiteral("imagoLibrary"), &library);
    view->rootContext()->setContextProperty(QStringLiteral("imagoPresets"), &presets);
    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    return app->exec();
}
