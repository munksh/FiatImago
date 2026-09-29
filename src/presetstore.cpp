#include "presetstore.h"

#include "recipe.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QVector>

#include <initializer_list>
#include <utility>

PresetStore *PresetStore::s_instance = nullptr;

namespace {

struct Look {
    const char *key;
    const char *label;
    QVariantMap values;
};

QVariantMap v(std::initializer_list<std::pair<const char *, double>> items)
{
    QVariantMap m;
    for (const auto &i : items)
        m.insert(QLatin1String(i.first), i.second);
    return m;
}

const QVector<Look> &looks()
{
    static const QVector<Look> list = {
        { "none", QT_TRANSLATE_NOOP("PresetStore", "none"), QVariantMap() },
        { "recover", QT_TRANSLATE_NOOP("PresetStore", "recover"),
          v({ { "exposure", 0.15 }, { "highlights", -65 }, { "whites", -15 }, { "shadows", 35 }, { "contrast", 8 }, { "vibrance", 12 } }) },
        { "warm", QT_TRANSLATE_NOOP("PresetStore", "warm"),
          v({ { "temperature", 20 }, { "tint", 4 }, { "contrast", 8 }, { "vibrance", 12 },
              { "hueGreen", -15 }, { "hueAqua", 12 }, { "satAqua", -10 }, { "satBlue", -8 } }) },
        { "golden", QT_TRANSLATE_NOOP("PresetStore", "golden"),
          v({ { "temperature", 14 }, { "tint", 2 }, { "contrast", 25 }, { "highlights", -30 },
              { "hueGreen", -65 }, { "satGreen", 55 }, { "lumGreen", 10 },
              { "hueYellow", -20 }, { "satYellow", 40 }, { "satOrange", 40 },
              { "hueAqua", 12 }, { "satAqua", -45 }, { "satBlue", -45 }, { "lumBlue", -15 },
              { "vignetteAmount", -15 } }) },
        { "cool", QT_TRANSLATE_NOOP("PresetStore", "cool"),
          v({ { "temperature", -18 }, { "tint", -3 }, { "contrast", 6 }, { "saturation", -5 } }) },
        { "punchy", QT_TRANSLATE_NOOP("PresetStore", "punchy"),
          v({ { "contrast", 32 }, { "blacks", -15 }, { "highlights", -20 }, { "vibrance", 35 },
              { "saturation", 5 }, { "sharpenAmount", 30 } }) },
        { "matte", QT_TRANSLATE_NOOP("PresetStore", "matte"),
          v({ { "contrast", -22 }, { "blacks", 45 }, { "highlights", -15 }, { "saturation", -18 },
              { "temperature", 4 } }) },
        { "mono", QT_TRANSLATE_NOOP("PresetStore", "mono"),
          v({ { "saturation", -100 }, { "contrast", 28 }, { "blacks", -12 }, { "highlights", -20 },
              { "sharpenAmount", 20 } }) },
        { "sepia", QT_TRANSLATE_NOOP("PresetStore", "sepia"),
          v({ { "saturation", -100 }, { "contrast", 12 }, { "blacks", 8 }, { "highlights", -15 },
              { "toneShadowHue", 24 }, { "toneShadowAmount", 85 },
              { "toneHighlightHue", 38 }, { "toneHighlightAmount", 70 } }) },
        { "facet", QT_TRANSLATE_NOOP("PresetStore", "facet"),
          v({ { "patternAmount", 45 }, { "patternSize", 80 }, { "contrast", -8 }, { "blacks", 10 },
              { "saturation", -12 } }) }
    };
    return list;
}

QVariantMap onlyLook(const QVariantMap &values)
{
    const QStringList keys = Recipe::lookKeys();
    QVariantMap out;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        if (keys.contains(it.key()))
            out.insert(it.key(), it.value());
    }
    return out;
}

}

PresetStore::PresetStore(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_path = dir + QStringLiteral("/presets.json");

    QFile file(m_path);
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        const QJsonArray list = doc.object().value(QStringLiteral("presets")).toArray();
        for (const QJsonValue &item : list) {
            const QJsonObject o = item.toObject();
            const QString name = o.value(QStringLiteral("name")).toString().trimmed();
            if (name.isEmpty())
                continue;
            QVariantMap preset;
            preset.insert(QStringLiteral("name"), name);
            preset.insert(QStringLiteral("values"), onlyLook(o.value(QStringLiteral("values")).toObject().toVariantMap()));
            m_user << preset;
        }
    }
}

PresetStore::~PresetStore()
{
    if (s_instance == this)
        s_instance = nullptr;
}

PresetStore *PresetStore::instance()
{
    return s_instance;
}

QVariantList PresetStore::builtIn() const
{
    QVariantList out;
    for (const Look &l : looks()) {
        QVariantMap item;
        item.insert(QStringLiteral("label"), tr(l.label));
        item.insert(QStringLiteral("value"), QStringLiteral("builtin:") + QLatin1String(l.key));
        out << item;
    }
    return out;
}

QVariantList PresetStore::user() const
{
    QVariantList out;
    for (const QVariant &p : m_user) {
        const QString name = p.toMap().value(QStringLiteral("name")).toString();
        QVariantMap item;
        item.insert(QStringLiteral("name"), name);
        item.insert(QStringLiteral("id"), QStringLiteral("user:") + name);
        out << item;
    }
    return out;
}

QVariantMap PresetStore::values(const QString &id) const
{
    if (id.startsWith(QLatin1String("builtin:"))) {
        const QString key = id.mid(8);
        for (const Look &l : looks()) {
            if (key == QLatin1String(l.key))
                return l.values;
        }
    } else if (id.startsWith(QLatin1String("user:"))) {
        const QString name = id.mid(5);
        for (const QVariant &p : m_user) {
            const QVariantMap m = p.toMap();
            if (m.value(QStringLiteral("name")).toString() == name)
                return m.value(QStringLiteral("values")).toMap();
        }
    }
    return QVariantMap();
}

bool PresetStore::exists(const QString &name) const
{
    const QString n = name.trimmed();
    for (const QVariant &p : m_user) {
        if (p.toMap().value(QStringLiteral("name")).toString() == n)
            return true;
    }
    return false;
}

QString PresetStore::save(const QString &name, const QVariantMap &values)
{
    const QString n = name.trimmed().left(40);
    if (n.isEmpty())
        return QString();
    QVariantMap preset;
    preset.insert(QStringLiteral("name"), n);
    preset.insert(QStringLiteral("values"), onlyLook(values));

    bool replaced = false;
    for (int i = 0; i < m_user.size(); ++i) {
        if (m_user.at(i).toMap().value(QStringLiteral("name")).toString() == n) {
            m_user[i] = preset;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        m_user << preset;
    write();
    emit userChanged();
    return QStringLiteral("user:") + n;
}

void PresetStore::remove(const QString &id)
{
    if (!id.startsWith(QLatin1String("user:")))
        return;
    const QString name = id.mid(5);
    for (int i = 0; i < m_user.size(); ++i) {
        if (m_user.at(i).toMap().value(QStringLiteral("name")).toString() == name) {
            m_user.removeAt(i);
            write();
            emit userChanged();
            return;
        }
    }
}

void PresetStore::write() const
{
    QJsonArray list;
    for (const QVariant &p : m_user) {
        const QVariantMap m = p.toMap();
        QJsonObject o;
        o.insert(QStringLiteral("name"), m.value(QStringLiteral("name")).toString());
        o.insert(QStringLiteral("values"), QJsonObject::fromVariantMap(m.value(QStringLiteral("values")).toMap()));
        list << o;
    }
    QJsonObject root;
    root.insert(QStringLiteral("presets"), list);
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
}
