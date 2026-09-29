#ifndef DEVELOPER_H
#define DEVELOPER_H

#include <QImage>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "imaging.h"
#include "recipe.h"

class DevelopWorker : public QObject
{
    Q_OBJECT
public:
    explicit DevelopWorker(QObject *parent = nullptr);

public slots:
    void load(const QVariantMap &source);
    void render(const QVariantMap &recipe, bool uncropped, bool clipWarning, quint64 generation);
    void inspect(const QVariantMap &recipe, double u, double v, int width, int height);
    void exportImage(const QVariantMap &recipe, const QVariantMap &options);

signals:
    void loaded(bool ok, const QString &message, bool raw, double frameAspect, const QImage &original);
    void rendered(const QImage &image, const QVariantList &histogram, quint64 generation);
    void inspected(const QImage &image, int frameWidth, int frameHeight);
    void inspectFailed(const QString &message);
    void exported(const QString &path);
    void exportFailed(const QString &message);

private:
    bool ensureFull(QString *error);

    QVariantMap m_source;
    bool m_raw = false;
    int m_turns = 0;
    float m_gain = 1.0f;
    ImageBuffer m_preview;
    ImageBuffer m_full;
};

class Developer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool isRaw READ isRaw NOTIFY loadedChanged)
    Q_PROPERTY(double frameAspect READ frameAspect NOTIFY loadedChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QVariantMap recipe READ recipe NOTIFY recipeChanged)
    Q_PROPERTY(QVariantList histogram READ histogram NOTIFY histogramChanged)
    Q_PROPERTY(bool uncropped READ uncropped WRITE setUncropped NOTIFY uncroppedChanged)
    Q_PROPERTY(bool showClipping READ showClipping WRITE setShowClipping NOTIFY showClippingChanged)
    Q_PROPERTY(bool loupe READ loupe WRITE setLoupe NOTIFY loupeChanged)
    Q_PROPERTY(bool hasClipboard READ hasClipboard NOTIFY clipboardChanged)
    Q_PROPERTY(bool inspectBusy READ inspectBusy NOTIFY inspectBusyChanged)
    Q_PROPERTY(int fullWidth READ fullWidth NOTIFY inspectChanged)
    Q_PROPERTY(int fullHeight READ fullHeight NOTIFY inspectChanged)
    Q_PROPERTY(bool exporting READ exporting NOTIFY exportingChanged)

public:
    explicit Developer(QObject *parent = nullptr);
    ~Developer();

    bool loaded() const { return m_loaded; }
    bool busy() const { return m_busy; }
    bool isRaw() const { return m_raw; }
    double frameAspect() const { return m_frameAspect; }
    QString message() const { return m_message; }
    QVariantMap recipe() const { return m_recipe.toMap(); }
    QVariantList histogram() const { return m_histogram; }
    bool uncropped() const { return m_uncropped; }
    void setUncropped(bool uncropped);
    bool showClipping() const { return m_showClipping; }
    void setShowClipping(bool show);
    bool loupe() const { return m_loupe; }
    void setLoupe(bool loupe);
    bool hasClipboard() const { return !s_clipboard.isEmpty(); }
    bool inspectBusy() const { return m_inspectBusy; }
    int fullWidth() const { return m_fullWidth; }
    int fullHeight() const { return m_fullHeight; }
    bool exporting() const { return m_exporting; }

    QImage preview() const { return m_preview; }
    QImage original() const { return m_original; }
    QImage inspectImage() const { return m_inspect; }

    Q_INVOKABLE void open(const QVariantMap &source);
    Q_INVOKABLE void set(const QString &name, const QVariant &value);
    Q_INVOKABLE void setCrop(double x, double y, double w, double h);
    Q_INVOKABLE void resetAll();
    Q_INVOKABLE void copySettings();
    Q_INVOKABLE void pasteSettings();
    Q_INVOKABLE void applyLook(const QString &id, double strength);
    Q_INVOKABLE QVariantMap lookSettings() const;
    Q_INVOKABLE void inspect(double u, double v, int width, int height);
    Q_INVOKABLE void exportImage(bool half, int quality, bool keepCameraData);
    Q_INVOKABLE void save();

signals:
    void loadedChanged();
    void busyChanged();
    void messageChanged();
    void recipeChanged();
    void histogramChanged();
    void uncroppedChanged();
    void showClippingChanged();
    void loupeChanged();
    void clipboardChanged();
    void inspectBusyChanged();
    void exportingChanged();
    void previewChanged();
    void originalChanged();
    void inspectChanged();
    void exportFinished(const QString &path);
    void exportFailed(const QString &message);

    void requestLoad(const QVariantMap &source);
    void requestRender(const QVariantMap &recipe, bool uncropped, bool clipWarning, quint64 generation);
    void requestInspect(const QVariantMap &recipe, double u, double v, int width, int height);
    void requestExport(const QVariantMap &recipe, const QVariantMap &options);

private:
    void onLoaded(bool ok, const QString &message, bool raw, double frameAspect, const QImage &original);
    void onRendered(const QImage &image, const QVariantList &histogram, quint64 generation);
    void onInspected(const QImage &image, int frameWidth, int frameHeight);
    void onInspectFailed(const QString &message);
    void onExported(const QString &path);
    void onExportFailed(const QString &message);

    void apply(const Recipe &recipe);
    void render();
    void renderInspect();
    void setBusy(bool busy);
    void setMessage(const QString &message);

    QThread m_thread;
    DevelopWorker *m_worker;
    QTimer m_saveTimer;

    QString m_key;
    Recipe m_recipe;
    bool m_loaded = false;
    bool m_busy = false;
    bool m_raw = false;
    double m_frameAspect = 1.0;
    QString m_message;
    QVariantList m_histogram;
    bool m_uncropped = false;
    bool m_showClipping = false;
    bool m_loupe = false;
    bool m_inFlight = false;
    bool m_dirty = false;
    quint64 m_generation = 0;
    bool m_inspectBusy = false;
    bool m_inspectDirty = false;
    bool m_hasInspect = false;
    double m_inspectU = 0.5;
    double m_inspectV = 0.5;
    int m_inspectWidth = 0;
    int m_inspectHeight = 0;
    int m_fullWidth = 0;
    int m_fullHeight = 0;
    bool m_exporting = false;

    QImage m_preview;
    QImage m_original;
    QImage m_inspect;

    static QVariantMap s_clipboard;
};

#endif
