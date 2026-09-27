#include "recipestore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

RecipeStore *RecipeStore::s_instance = nullptr;

RecipeStore::RecipeStore(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_path = dir + QStringLiteral("/recipes.json");

    QFile file(m_path);
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject())
            m_all = doc.object().toVariantMap();
    }
}

RecipeStore::~RecipeStore()
{
    if (s_instance == this)
        s_instance = nullptr;
}

RecipeStore *RecipeStore::instance()
{
    return s_instance;
}

bool RecipeStore::contains(const QString &key) const
{
    return m_all.contains(key);
}

QVariantMap RecipeStore::get(const QString &key) const
{
    return m_all.value(key).toMap();
}

void RecipeStore::put(const QString &key, const QVariantMap &recipe)
{
    if (key.isEmpty() || m_all.value(key).toMap() == recipe)
        return;
    m_all.insert(key, recipe);
    write();
    emit changed(key);
}

void RecipeStore::remove(const QString &key)
{
    if (!m_all.contains(key))
        return;
    m_all.remove(key);
    write();
    emit changed(key);
}

void RecipeStore::write() const
{
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(QJsonObject::fromVariantMap(m_all)).toJson(QJsonDocument::Compact));
    file.commit();
}
