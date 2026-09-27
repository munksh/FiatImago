#include "developview.h"

#include <QPainter>

DevelopView::DevelopView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_mode(QStringLiteral("preview"))
{
    setOpaquePainting(false);
    setAntialiasing(true);
}

void DevelopView::setDeveloper(Developer *developer)
{
    if (developer == m_developer)
        return;
    if (m_developer)
        disconnect(m_developer, nullptr, this, nullptr);
    m_developer = developer;
    if (m_developer) {
        connect(m_developer, &Developer::previewChanged, this, &DevelopView::refresh);
        connect(m_developer, &Developer::originalChanged, this, &DevelopView::refresh);
        connect(m_developer, &Developer::inspectChanged, this, &DevelopView::refresh);
    }
    emit developerChanged();
    refresh();
}

void DevelopView::setMode(const QString &mode)
{
    if (mode == m_mode)
        return;
    m_mode = mode;
    emit modeChanged();
    refresh();
}

void DevelopView::paint(QPainter *painter)
{
    const QImage image = currentImage();
    if (image.isNull() || m_paintedRect.isEmpty())
        return;
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawImage(m_paintedRect, image);
}

void DevelopView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
    refresh();
}

QImage DevelopView::currentImage() const
{
    if (!m_developer)
        return QImage();
    if (m_mode == QLatin1String("original"))
        return m_developer->original();
    if (m_mode == QLatin1String("inspect"))
        return m_developer->inspectImage();
    return m_developer->preview();
}

void DevelopView::refresh()
{
    const QImage image = currentImage();
    QRectF rect;
    if (!image.isNull() && width() > 0 && height() > 0) {
        if (m_mode == QLatin1String("inspect")) {
            rect = QRectF((width() - image.width()) / 2.0, (height() - image.height()) / 2.0,
                          image.width(), image.height());
        } else {
            const QSizeF fit = QSizeF(image.size()).scaled(width(), height(), Qt::KeepAspectRatio);
            rect = QRectF((width() - fit.width()) / 2.0, (height() - fit.height()) / 2.0,
                          fit.width(), fit.height());
        }
    }
    if (rect != m_paintedRect) {
        m_paintedRect = rect;
        emit paintedRectChanged();
    }
    update();
}
