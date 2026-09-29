#ifndef PRESETSTORE_H
#define PRESETSTORE_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Looks: a recipe's light, colour, detail and effects settings, without the
// geometry. The built-in ones ship with the app and cannot change; the
// user's own are kept in the app's data folder.
class PresetStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList builtIn READ builtIn CONSTANT)
    Q_PROPERTY(QVariantList user READ user NOTIFY userChanged)

public:
    explicit PresetStore(QObject *parent = nullptr);
    ~PresetStore();

    static PresetStore *instance();

    QVariantList builtIn() const;
    QVariantList user() const;

    // Ids are "builtin:<key>" or "user:<name>".
    QVariantMap values(const QString &id) const;
    Q_INVOKABLE bool exists(const QString &name) const;
    Q_INVOKABLE QString save(const QString &name, const QVariantMap &values);
    Q_INVOKABLE void remove(const QString &id);

signals:
    void userChanged();

private:
    void write() const;

    QString m_path;
    QVariantList m_user;
    static PresetStore *s_instance;
};

#endif
