#include "developer.h"

#include "exporter.h"
#include "imagesource.h"
#include "pipeline.h"
#include "recipestore.h"

#include <algorithm>

namespace {

const int PreviewSource = 2048;
const int PreviewSize = 1200;

}

QVariantMap Developer::s_clipboard;

DevelopWorker::DevelopWorker(QObject *parent)
    : QObject(parent)
{
}

void DevelopWorker::load(const QVariantMap &source)
{
    m_source = source;
    m_full.clear();
    m_preview.clear();
    m_turns = 0;
    m_gain = 1.0f;

    const QString jpeg = source.value(QStringLiteral("jpeg")).toString();
    const QString raw = source.value(QStringLiteral("raw")).toString();
    QString message;
    m_raw = false;

    if (!raw.isEmpty()) {
        RawMeta meta;
        QString error;
        if (ImageSource::loadRaw(raw, source.value(QStringLiteral("json")).toString(),
                                 source.value(QStringLiteral("altJson")).toString(),
                                 true, m_preview, &meta, &error)) {
            m_raw = true;
            if (!jpeg.isEmpty())
                m_turns = ImageSource::matchRotation(m_preview, jpeg);
            else if (meta.orientation > 0)
                m_turns = meta.orientation / 90;
            m_preview = Imaging::rotated(m_preview, m_turns);
            if (!jpeg.isEmpty())
                m_gain = ImageSource::matchBrightness(m_preview, jpeg);
            Imaging::scale(m_preview, m_gain);
        } else if (!jpeg.isEmpty()) {
            message = tr("The RAW could not be read (%1). Showing the JPEG.").arg(error);
        } else {
            emit loaded(false, error, false, 1.0, QImage());
            return;
        }
    }

    if (!m_raw) {
        QString error;
        if (!ImageSource::loadJpeg(jpeg, PreviewSource, m_preview, &error)) {
            emit loaded(false, error.isEmpty() ? tr("The photo could not be read.") : error, false, 1.0, QImage());
            return;
        }
    }

    const Recipe neutral;
    const QSizeF frame = Pipeline::frameSize(m_preview.width, m_preview.height, neutral, false);
    const QImage original = Pipeline::render(m_preview, neutral, m_raw, false,
                                             Pipeline::fitted(frame, PreviewSize));
    emit loaded(true, message, m_raw, double(m_preview.width) / m_preview.height, original);
}

void DevelopWorker::render(const QVariantMap &recipe, bool uncropped, bool clipWarning, quint64 generation)
{
    const Recipe r = Recipe::fromMap(recipe);
    const QSizeF frame = Pipeline::frameSize(m_preview.width, m_preview.height, r, !uncropped);
    const QImage image = Pipeline::render(m_preview, r, m_raw, !uncropped,
                                          Pipeline::fitted(frame, PreviewSize),
                                          QRectF(0.0, 0.0, 1.0, 1.0), clipWarning);
    emit rendered(image, Pipeline::histogram(image), generation);
}

bool DevelopWorker::ensureFull(QString *error)
{
    if (!m_full.isNull())
        return true;
    if (m_raw) {
        if (!ImageSource::loadRaw(m_source.value(QStringLiteral("raw")).toString(),
                                  m_source.value(QStringLiteral("json")).toString(),
                                  m_source.value(QStringLiteral("altJson")).toString(),
                                  false, m_full, nullptr, error))
            return false;
        m_full = Imaging::rotated(m_full, m_turns);
        Imaging::scale(m_full, m_gain);
        return true;
    }
    return ImageSource::loadJpeg(m_source.value(QStringLiteral("jpeg")).toString(), 0, m_full, error);
}

void DevelopWorker::inspect(const QVariantMap &recipe, double u, double v, int width, int height)
{
    QString error;
    if (!ensureFull(&error)) {
        emit inspectFailed(error);
        return;
    }
    const Recipe r = Recipe::fromMap(recipe);
    const QSizeF frame = Pipeline::frameSize(m_full.width, m_full.height, r, true);
    const double ww = std::min(1.0, width / frame.width());
    const double wh = std::min(1.0, height / frame.height());
    const double x = std::max(0.0, std::min(1.0 - ww, u - ww / 2.0));
    const double y = std::max(0.0, std::min(1.0 - wh, v - wh / 2.0));
    const QSize size(std::max(1, int(ww * frame.width())), std::max(1, int(wh * frame.height())));
    const QImage image = Pipeline::render(m_full, r, m_raw, true, size, QRectF(x, y, ww, wh));
    emit inspected(image, int(frame.width()), int(frame.height()));
}

void DevelopWorker::exportImage(const QVariantMap &recipe, const QVariantMap &options)
{
    QString error;
    if (!ensureFull(&error)) {
        emit exportFailed(error);
        return;
    }
    const Recipe r = Recipe::fromMap(recipe);
    const bool half = options.value(QStringLiteral("half")).toBool();
    const QSizeF frame = Pipeline::frameSize(m_full.width, m_full.height, r, true);
    const QSize size = Pipeline::fitted(frame * (half ? 0.5 : 1.0), 0);
    const QImage image = Pipeline::render(m_full, r, m_raw, true, size);
    if (image.isNull()) {
        emit exportFailed(tr("Nothing to export."));
        return;
    }

    const QByteArray data = options.value(QStringLiteral("cameraData")).toBool()
            ? Exporter::cameraData(m_source.value(QStringLiteral("jpeg")).toString())
            : QByteArray();
    const QString path = Exporter::uniquePath(m_source.value(QStringLiteral("name")).toString());
    const int quality = options.value(QStringLiteral("quality"), 92).toInt();
    if (!Exporter::writeJpeg(image, path, quality, data, &error)) {
        emit exportFailed(error);
        return;
    }
    emit exported(path);
}

Developer::Developer(QObject *parent)
    : QObject(parent)
    , m_worker(new DevelopWorker)
{
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(this, &Developer::requestLoad, m_worker, &DevelopWorker::load);
    connect(this, &Developer::requestRender, m_worker, &DevelopWorker::render);
    connect(this, &Developer::requestInspect, m_worker, &DevelopWorker::inspect);
    connect(this, &Developer::requestExport, m_worker, &DevelopWorker::exportImage);

    connect(m_worker, &DevelopWorker::loaded, this, &Developer::onLoaded);
    connect(m_worker, &DevelopWorker::rendered, this, &Developer::onRendered);
    connect(m_worker, &DevelopWorker::inspected, this, &Developer::onInspected);
    connect(m_worker, &DevelopWorker::inspectFailed, this, &Developer::onInspectFailed);
    connect(m_worker, &DevelopWorker::exported, this, &Developer::onExported);
    connect(m_worker, &DevelopWorker::exportFailed, this, &Developer::onExportFailed);

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(800);
    connect(&m_saveTimer, &QTimer::timeout, this, &Developer::save);

    m_thread.start();
}

Developer::~Developer()
{
    save();
    m_thread.quit();
    m_thread.wait();
}

void Developer::setUncropped(bool uncropped)
{
    if (uncropped == m_uncropped)
        return;
    m_uncropped = uncropped;
    emit uncroppedChanged();
    render();
}

void Developer::setShowClipping(bool show)
{
    if (show == m_showClipping)
        return;
    m_showClipping = show;
    emit showClippingChanged();
    render();
}

void Developer::setLoupe(bool loupe)
{
    if (loupe == m_loupe)
        return;
    m_loupe = loupe;
    emit loupeChanged();
}

void Developer::open(const QVariantMap &source)
{
    m_key = source.value(QStringLiteral("key")).toString();
    m_loaded = false;
    emit loadedChanged();
    RecipeStore *store = RecipeStore::instance();
    m_recipe = store ? Recipe::fromMap(store->get(m_key)) : Recipe();
    emit recipeChanged();
    setMessage(QString());
    setBusy(true);
    emit requestLoad(source);
}

void Developer::set(const QString &name, const QVariant &value)
{
    QVariantMap map = m_recipe.toMap();
    map.insert(name, value);
    if (name == QLatin1String("rotation")) {
        map.insert(QStringLiteral("cropX"), 0.0);
        map.insert(QStringLiteral("cropY"), 0.0);
        map.insert(QStringLiteral("cropW"), 1.0);
        map.insert(QStringLiteral("cropH"), 1.0);
        map.insert(QStringLiteral("aspect"), QStringLiteral("original"));
    }
    apply(Recipe::fromMap(map));
}

void Developer::setCrop(double x, double y, double w, double h)
{
    QVariantMap map = m_recipe.toMap();
    map.insert(QStringLiteral("cropX"), x);
    map.insert(QStringLiteral("cropY"), y);
    map.insert(QStringLiteral("cropW"), w);
    map.insert(QStringLiteral("cropH"), h);
    apply(Recipe::fromMap(map));
}

void Developer::resetAll()
{
    apply(Recipe());
}

void Developer::copySettings()
{
    QVariantMap map = m_recipe.toMap();
    for (const QString &key : Recipe::geometryKeys())
        map.remove(key);
    s_clipboard = map;
    emit clipboardChanged();
}

void Developer::pasteSettings()
{
    if (s_clipboard.isEmpty())
        return;
    QVariantMap map = m_recipe.toMap();
    for (auto it = s_clipboard.constBegin(); it != s_clipboard.constEnd(); ++it)
        map.insert(it.key(), it.value());
    apply(Recipe::fromMap(map));
}

void Developer::inspect(double u, double v, int width, int height)
{
    m_inspectU = u;
    m_inspectV = v;
    m_inspectWidth = width;
    m_inspectHeight = height;
    m_hasInspect = true;
    renderInspect();
}

void Developer::exportImage(bool half, int quality, bool keepCameraData)
{
    if (!m_loaded || m_exporting)
        return;
    save();
    m_exporting = true;
    emit exportingChanged();
    QVariantMap options;
    options.insert(QStringLiteral("half"), half);
    options.insert(QStringLiteral("quality"), quality);
    options.insert(QStringLiteral("cameraData"), keepCameraData);
    emit requestExport(m_recipe.toMap(), options);
}

void Developer::save()
{
    m_saveTimer.stop();
    RecipeStore *store = RecipeStore::instance();
    if (!store || m_key.isEmpty() || !m_loaded)
        return;
    if (m_recipe.isDefault())
        store->remove(m_key);
    else
        store->put(m_key, m_recipe.toMap());
}

void Developer::onLoaded(bool ok, const QString &message, bool raw, double frameAspect, const QImage &original)
{
    setBusy(false);
    setMessage(message);
    if (!ok)
        return;
    m_raw = raw;
    m_frameAspect = frameAspect;
    m_original = original;
    m_loaded = true;
    emit loadedChanged();
    emit originalChanged();
    render();
    if (m_loupe)
        renderInspect();
}

void Developer::onRendered(const QImage &image, const QVariantList &histogram, quint64 generation)
{
    Q_UNUSED(generation)
    m_inFlight = false;
    m_preview = image;
    m_histogram = histogram;
    emit previewChanged();
    emit histogramChanged();
    if (m_dirty) {
        m_dirty = false;
        render();
    }
}

void Developer::onInspected(const QImage &image, int frameWidth, int frameHeight)
{
    m_inspect = image;
    m_fullWidth = frameWidth;
    m_fullHeight = frameHeight;
    m_inspectBusy = false;
    emit inspectChanged();
    if (m_inspectDirty) {
        m_inspectDirty = false;
        renderInspect();
    }
    if (!m_inspectBusy)
        emit inspectBusyChanged();
}

void Developer::onInspectFailed(const QString &message)
{
    m_inspectBusy = false;
    m_inspectDirty = false;
    emit inspectBusyChanged();
    setMessage(message);
}

void Developer::onExported(const QString &path)
{
    m_exporting = false;
    emit exportingChanged();
    emit exportFinished(path);
}

void Developer::onExportFailed(const QString &message)
{
    m_exporting = false;
    emit exportingChanged();
    emit exportFailed(message);
}

void Developer::apply(const Recipe &recipe)
{
    if (recipe.toMap() == m_recipe.toMap())
        return;
    m_recipe = recipe;
    emit recipeChanged();
    render();
    if (m_loupe)
        renderInspect();
    m_saveTimer.start();
}

void Developer::render()
{
    if (!m_loaded)
        return;
    if (m_inFlight) {
        m_dirty = true;
        return;
    }
    m_inFlight = true;
    emit requestRender(m_recipe.toMap(), m_uncropped, m_showClipping, ++m_generation);
}

void Developer::renderInspect()
{
    if (!m_loaded || !m_hasInspect)
        return;
    if (m_inspectBusy) {
        m_inspectDirty = true;
        return;
    }
    m_inspectBusy = true;
    emit inspectBusyChanged();
    emit requestInspect(m_recipe.toMap(), m_inspectU, m_inspectV, m_inspectWidth, m_inspectHeight);
}

void Developer::setBusy(bool busy)
{
    if (busy == m_busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void Developer::setMessage(const QString &message)
{
    if (message == m_message)
        return;
    m_message = message;
    emit messageChanged();
}
