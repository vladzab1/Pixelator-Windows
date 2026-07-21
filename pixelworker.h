#pragma once

#include <QObject>
#include <QFuture>
#include <QStringList>
#include <QVariantList>

#include <opencv2/core.hpp>

class PixelWorker final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList queue READ queue NOTIFY queueChanged)
    Q_PROPERTY(bool processing READ processing NOTIFY processingChanged)

public:
    explicit PixelWorker(QObject *parent = nullptr);
    ~PixelWorker() override;

    [[nodiscard]] QStringList queue() const;
    [[nodiscard]] bool processing() const noexcept;

    Q_INVOKABLE void addFiles(const QVariantList &urlsOrPaths);
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE void clearQueue();
    Q_INVOKABLE void processAll(const QString &mode, const QString &palette);

    [[nodiscard]] static cv::Size findBestPixelSize(const cv::Mat &image);

signals:
    void queueChanged();
    void processingChanged();
    void progressChanged(int completed, int total, const QString &fileName);
    void statusChanged(const QString &message);
    void fileFinished(const QString &fileName, bool success, const QString &details);
    void processingFinished(int processed, int failed);

private:
    [[nodiscard]] static cv::Size targetSize(const cv::Size &source, const QString &mode);
    [[nodiscard]] static cv::Mat applyPalette(const cv::Mat &bgr, const QString &palette);
    [[nodiscard]] static bool isSupportedImage(const QString &path);
    [[nodiscard]] static QString localPath(const QVariant &urlOrPath);
    [[nodiscard]] static QString sanitizePaletteSuffix(const QString &palette);
    [[nodiscard]] static QString outputPathFor(const QString &inputPath, int targetWidth,
                                                const QString &palette);
    [[nodiscard]] static bool processOne(const QString &inputPath, const QString &mode,
                                         const QString &palette, QString *error);

    QStringList m_queue;
    bool m_processing = false;
    QFuture<void> m_processingFuture;
};
