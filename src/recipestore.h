#ifndef RECIPESTORE_H
#define RECIPESTORE_H

#include <QObject>
#include <QVariantMap>

class RecipeStore : public QObject
{
    Q_OBJECT
public:
    explicit RecipeStore(QObject *parent = nullptr);
    ~RecipeStore();

    static RecipeStore *instance();

    bool contains(const QString &key) const;
    QVariantMap get(const QString &key) const;
    void put(const QString &key, const QVariantMap &recipe);
    void remove(const QString &key);

signals:
    void changed(const QString &key);

private:
    void write() const;

    QString m_path;
    QVariantMap m_all;
    static RecipeStore *s_instance;
};

#endif
