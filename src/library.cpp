#include "library.h"

#include "recipestore.h"

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>

namespace {

struct Found {
    QString jpeg;
    QString raw;
    QString json;
    QString warmRaw;
    QString warmJson;
    QDateTime modified;
};

}

Library::Library(RecipeStore *store, QObject *parent)
    : QAbstractListModel(parent)
    , m_store(store)
    , m_filter(QStringLiteral("all"))
{
    connect(m_store, &RecipeStore::changed, this, &Library::onRecipeChanged);
    refresh();
}

int Library::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant Library::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size())
        return QVariant();
    const Entry &e = m_all.at(m_visible.at(index.row()));
    switch (role) {
    case KeyRole: return e.key;
    case NameRole: return e.name;
    case ShortNameRole: {
        QString s = e.name;
        if (s.startsWith(QLatin1String("RAWfish_")))
            s = s.mid(8);
        return s;
    }
    case JpegRole: return e.jpeg;
    case RawRole: return e.raw;
    case JsonRole: return e.json;
    case AltJsonRole: return e.altJson;
    case HasRawRole: return !e.raw.isEmpty();
    case EditedRole: return e.edited;
    case ThumbRole: return thumbSource(e.jpeg, e.raw, e.json, e.altJson);
    default: return QVariant();
    }
}

QHash<int, QByteArray> Library::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "key";
    roles[NameRole] = "name";
    roles[ShortNameRole] = "shortName";
    roles[JpegRole] = "jpegPath";
    roles[RawRole] = "rawPath";
    roles[JsonRole] = "jsonPath";
    roles[AltJsonRole] = "altJsonPath";
    roles[HasRawRole] = "hasRaw";
    roles[EditedRole] = "edited";
    roles[ThumbRole] = "thumb";
    return roles;
}

void Library::setFilter(const QString &filter)
{
    if (filter == m_filter)
        return;
    m_filter = filter;
    beginResetModel();
    applyFilter();
    endResetModel();
    emit filterChanged();
    emit countChanged();
}

void Library::refresh()
{
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QStringList dirs = QStringList()
            << pictures
            << pictures + QStringLiteral("/Camera")
            << pictures + QStringLiteral("/RAWfish");

    struct Suffix { const char *text; int kind; };
    static const Suffix suffixes[] = {
        { ".warm.raw16", 3 }, { ".warm.raw12", 3 }, { ".warm.raw10", 3 }, { ".warm.raw8", 3 },
        { ".warm.json", 4 },
        { ".raw16", 1 }, { ".raw12", 1 }, { ".raw10", 1 }, { ".raw8", 1 },
        { ".json", 2 }, { ".jpg", 0 }, { ".jpeg", 0 }
    };

    QMap<QString, Found> groups;
    for (const QString &dir : dirs) {
        const QFileInfoList files = QDir(dir).entryInfoList(QDir::Files, QDir::NoSort);
        for (const QFileInfo &info : files) {
            const QString name = info.fileName();
            const QString lower = name.toLower();
            for (const Suffix &s : suffixes) {
                const QString suffix = QLatin1String(s.text);
                if (!lower.endsWith(suffix))
                    continue;
                const QString base = name.left(name.length() - suffix.length());
                Found &f = groups[dir + QLatin1Char('/') + base];
                const QString path = info.absoluteFilePath();
                switch (s.kind) {
                case 0: f.jpeg = path; break;
                case 1: f.raw = path; break;
                case 2: f.json = path; break;
                case 3: f.warmRaw = path; break;
                default: f.warmJson = path; break;
                }
                if (!f.modified.isValid() || info.lastModified() > f.modified)
                    f.modified = info.lastModified();
                break;
            }
        }
    }

    QVector<Entry> entries;
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
        const Found &f = it.value();
        Entry e;
        e.key = it.key();
        e.name = QFileInfo(it.key()).fileName();
        e.jpeg = f.jpeg;
        e.modified = f.modified;
        if (!f.raw.isEmpty() && (!f.json.isEmpty() || !f.warmJson.isEmpty())) {
            e.raw = f.raw;
            e.json = f.json.isEmpty() ? f.warmJson : f.json;
            e.altJson = f.json.isEmpty() ? QString() : f.warmJson;
        } else if (!f.warmRaw.isEmpty() && (!f.warmJson.isEmpty() || !f.json.isEmpty())) {
            e.raw = f.warmRaw;
            e.json = f.warmJson.isEmpty() ? f.json : f.warmJson;
            e.altJson = f.warmJson.isEmpty() ? QString() : f.json;
        }
        if (e.jpeg.isEmpty() && e.raw.isEmpty())
            continue;
        e.edited = m_store->contains(e.key);
        entries << e;
    }
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        return a.modified > b.modified;
    });

    beginResetModel();
    m_all = entries;
    applyFilter();
    endResetModel();
    emit countChanged();
}

void Library::setCurrent(const QString &name, const QString &thumb)
{
    QVariantMap current;
    current.insert(QStringLiteral("name"), name);
    current.insert(QStringLiteral("thumb"), thumb);
    if (current == m_current)
        return;
    m_current = current;
    emit currentChanged();
}

QString Library::thumbSource(const QString &jpeg, const QString &raw,
                             const QString &json, const QString &altJson)
{
    const QString id = jpeg.isEmpty()
            ? QStringLiteral("raw\n") + raw + QLatin1Char('\n') + json + QLatin1Char('\n') + altJson
            : jpeg;
    return QStringLiteral("image://thumbs/") + QString::fromLatin1(QUrl::toPercentEncoding(id));
}

void Library::applyFilter()
{
    m_visible.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        const Entry &e = m_all.at(i);
        const bool keep = m_filter == QLatin1String("raw") ? !e.raw.isEmpty()
                : m_filter == QLatin1String("jpeg") ? e.raw.isEmpty()
                : m_filter == QLatin1String("edited") ? e.edited
                : true;
        if (keep)
            m_visible << i;
    }
}

void Library::onRecipeChanged(const QString &key)
{
    for (int i = 0; i < m_all.size(); ++i) {
        if (m_all.at(i).key != key)
            continue;
        const bool edited = m_store->contains(key);
        if (m_all.at(i).edited == edited)
            return;
        m_all[i].edited = edited;
        if (m_filter == QLatin1String("edited")) {
            beginResetModel();
            applyFilter();
            endResetModel();
            emit countChanged();
        } else {
            const int row = m_visible.indexOf(i);
            if (row >= 0)
                emit dataChanged(index(row), index(row), QVector<int>() << EditedRole);
        }
        return;
    }
}
