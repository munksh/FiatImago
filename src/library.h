#ifndef LIBRARY_H
#define LIBRARY_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QVariantMap>
#include <QVector>

class RecipeStore;

class Library : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QVariantMap current READ current NOTIFY currentChanged)

public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        NameRole,
        ShortNameRole,
        JpegRole,
        RawRole,
        JsonRole,
        AltJsonRole,
        HasRawRole,
        EditedRole,
        ThumbRole
    };

    explicit Library(RecipeStore *store, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);
    int count() const { return m_visible.size(); }
    QVariantMap current() const { return m_current; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setCurrent(const QString &name, const QString &thumb);

    static QString thumbSource(const QString &jpeg, const QString &raw,
                               const QString &json, const QString &altJson);

signals:
    void filterChanged();
    void countChanged();
    void currentChanged();

private:
    struct Entry {
        QString key;
        QString name;
        QString jpeg;
        QString raw;
        QString json;
        QString altJson;
        QDateTime modified;
        bool edited = false;
    };

    void applyFilter();
    void onRecipeChanged(const QString &key);

    RecipeStore *m_store;
    QString m_filter;
    QVector<Entry> m_all;
    QVector<int> m_visible;
    QVariantMap m_current;
};

#endif
