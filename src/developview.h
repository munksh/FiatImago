#ifndef DEVELOPVIEW_H
#define DEVELOPVIEW_H

#include <QPointer>
#include <QQuickPaintedItem>

#include "developer.h"

class DevelopView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(Developer *developer READ developer WRITE setDeveloper NOTIFY developerChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QRectF paintedRect READ paintedRect NOTIFY paintedRectChanged)

public:
    explicit DevelopView(QQuickItem *parent = nullptr);

    Developer *developer() const { return m_developer; }
    void setDeveloper(Developer *developer);
    QString mode() const { return m_mode; }
    void setMode(const QString &mode);
    QRectF paintedRect() const { return m_paintedRect; }

    void paint(QPainter *painter) override;

signals:
    void developerChanged();
    void modeChanged();
    void paintedRectChanged();

protected:
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    QImage currentImage() const;
    void refresh();

    QPointer<Developer> m_developer;
    QString m_mode;
    QRectF m_paintedRect;
};

#endif
