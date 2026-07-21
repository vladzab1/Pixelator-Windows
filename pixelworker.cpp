#include "pixelworker.h"

#include <QFileInfo>
#include <QMetaObject>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <limits>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace {
using Color = cv::Vec3b; // OpenCV stores colour channels as BGR.

const std::vector<Color> &colorsFor(const QString &palette)
{
    static const std::vector<Color> gameBoy{{15, 56, 15}, {48, 98, 48}, {139, 172, 15}, {155, 188, 15}};
    static const std::vector<Color> nes{
        {124, 124, 124}, {252, 0, 0}, {188, 0, 0}, {168, 0, 68},
        {120, 0, 148}, {0, 0, 168}, {0, 0, 136}, {0, 40, 80},
        {0, 120, 0}, {0, 104, 0}, {0, 88, 0}, {0, 64, 0},
        {0, 84, 188}, {0, 120, 248}, {60, 188, 248}, {104, 136, 152},
        {152, 120, 248}, {216, 0, 216}, {228, 0, 168}, {248, 56, 0},
        {248, 120, 0}, {188, 188, 0}, {0, 184, 0}, {0, 168, 0},
        {0, 136, 0}, {0, 136, 136}, {0, 136, 248}, {60, 248, 248},
        {152, 216, 248}, {200, 200, 200}, {216, 168, 248}, {248, 120, 248},
        {248, 120, 216}, {248, 160, 104}, {248, 184, 0}, {248, 216, 120},
        {184, 248, 184}, {88, 216, 216}, {120, 216, 248}, {168, 228, 248},
        {248, 248, 248}, {252, 216, 248}, {248, 200, 248}, {248, 200, 216},
        {248, 216, 184}, {248, 248, 120}, {216, 248, 216}, {184, 248, 248}};
    static const std::vector<Color> pico8{
        {0, 0, 0}, {51, 29, 29}, {82, 17, 126}, {57, 87, 0},
        {83, 82, 171}, {114, 73, 95}, {110, 158, 41}, {169, 203, 212},
        {43, 129, 255}, {0, 76, 255}, {17, 37, 171}, {54, 44, 171},
        {29, 163, 0}, {0, 173, 255}, {66, 228, 255}, {204, 255, 255}};
    if (palette == u"GameBoy Original") return gameBoy;
    if (palette == u"NES / Famicom") return nes;
    return pico8;
}
} // namespace

PixelWorker::PixelWorker(QObject *parent) : QObject(parent) {}

PixelWorker::~PixelWorker()
{
    if (m_processingFuture.isRunning())
        m_processingFuture.waitForFinished();
}

QStringList PixelWorker::queue() const { return m_queue; }
bool PixelWorker::processing() const noexcept { return m_processing; }

void PixelWorker::addFiles(const QVariantList &urlsOrPaths)
{
    if (m_processing) return;
    bool changed = false;
    for (const QVariant &value : urlsOrPaths) {
        const QString path = localPath(value);
        if (isSupportedImage(path) && QFileInfo::exists(path) && !m_queue.contains(path)) {
            m_queue.append(path);
            changed = true;
        }
    }
    if (changed) {
        emit queueChanged();
        emit statusChanged(QString::number(m_queue.size()) + QStringLiteral(" file(s) queued"));
    }
}

void PixelWorker::removeAt(int index)
{
    if (!m_processing && index >= 0 && index < m_queue.size()) {
        m_queue.removeAt(index);
        emit queueChanged();
    }
}

void PixelWorker::clearQueue()
{
    if (!m_processing && !m_queue.isEmpty()) {
        m_queue.clear();
        emit queueChanged();
        emit statusChanged(QStringLiteral("Queue cleared"));
    }
}

cv::Size PixelWorker::findBestPixelSize(const cv::Mat &image)
{
    const cv::Size source = image.size();
    const double aspect = static_cast<double>(source.width) / source.height;
    const int maxSide = std::min(std::max(source.width, source.height), 400);
    cv::Size best = source;
    double smallestDifference = std::numeric_limits<double>::infinity();

    for (int side = 8; side <= maxSide; ++side) {
        const cv::Size candidate = source.width >= source.height
            ? cv::Size{side, std::max(1, static_cast<int>(std::lround(side / aspect)))}
            : cv::Size{std::max(1, static_cast<int>(std::lround(side * aspect))), side};
        cv::Mat small, reconstructed, difference;
        cv::resize(image, small, candidate, 0.0, 0.0, cv::INTER_NEAREST);
        cv::resize(small, reconstructed, source, 0.0, 0.0, cv::INTER_NEAREST);
        cv::absdiff(image, reconstructed, difference);
        const cv::Scalar mean = cv::mean(difference);
        const double score = (mean[0] + mean[1] + mean[2]) / 3.0;
        if (score < smallestDifference) { smallestDifference = score; best = candidate; }
        if (score == 0.0) break;
    }
    return best;
}

cv::Size PixelWorker::targetSize(const cv::Size &source, const QString &mode)
{
    if (mode == u"Smart auto") return findBestPixelSize(cv::Mat(source, CV_8UC3, cv::Scalar{}));
    bool ok = false;
    const int maxSide = mode.section(' ', 0, 0).toInt(&ok);
    if (!ok) return source;
    const double aspect = static_cast<double>(source.width) / source.height;
    return source.width >= source.height
        ? cv::Size{std::min(source.width, maxSide), std::max(1, static_cast<int>(std::lround(std::min(source.width, maxSide) / aspect)))}
        : cv::Size{std::max(1, static_cast<int>(std::lround(std::min(source.height, maxSide) * aspect))), std::min(source.height, maxSide)};
}

cv::Mat PixelWorker::applyPalette(const cv::Mat &bgr, const QString &palette)
{
    if (palette == u"True Color") return bgr;
    if (palette == u"SEGA Mega Drive / Genesis") {
        cv::Mat result = bgr.clone();
        for (int y = 0; y < result.rows; ++y)
            for (auto *pixel = result.ptr<Color>(y); pixel != result.ptr<Color>(y) + result.cols; ++pixel)
                for (int channel = 0; channel < 3; ++channel)
                    (*pixel)[channel] = static_cast<unsigned char>(((*pixel)[channel] + 42) / 85 * 85);
        return result;
    }
    const auto &paletteColors = colorsFor(palette);
    cv::Mat result = bgr.clone();
    for (int y = 0; y < result.rows; ++y) {
        for (auto *pixel = result.ptr<Color>(y); pixel != result.ptr<Color>(y) + result.cols; ++pixel) {
            const auto nearest = std::min_element(paletteColors.begin(), paletteColors.end(), [pixel](const Color &a, const Color &b) {
                const auto distance = [pixel](const Color &color) {
                    const int db = (*pixel)[0] - color[0], dg = (*pixel)[1] - color[1], dr = (*pixel)[2] - color[2];
                    return db * db + dg * dg + dr * dr;
                };
                return distance(a) < distance(b);
            });
            *pixel = *nearest;
        }
    }
    return result;
}

bool PixelWorker::isSupportedImage(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == u"png" || suffix == u"jpg" || suffix == u"jpeg" || suffix == u"webp";
}

QString PixelWorker::localPath(const QVariant &urlOrPath)
{
    const QUrl url = urlOrPath.toUrl();
    return url.isLocalFile() ? url.toLocalFile() : urlOrPath.toString().remove("file://");
}

QString PixelWorker::sanitizePaletteSuffix(const QString &palette)
{
    QString result;
    result.reserve(palette.size());
    for (const QChar ch : palette) {
        if (ch.isLetterOrNumber())
            result.append(ch.toLower());
    }
    return result.isEmpty() ? QStringLiteral("default") : result;
}

QString PixelWorker::outputPathFor(const QString &inputPath, int targetWidth, const QString &palette)
{
    namespace fs = std::filesystem;
    const fs::path input(inputPath.toStdString());
    const fs::path dir = input.parent_path();
    const std::string stem = input.stem().string();
    const std::string extension = input.has_extension() ? input.extension().string().substr(1) : "png";

    const QString paletteSuffix = sanitizePaletteSuffix(palette);
    const QString fileName = QStringLiteral("%1_pixelated_%2px_%3.%4")
                                 .arg(QString::fromStdString(stem))
                                 .arg(targetWidth)
                                 .arg(paletteSuffix)
                                 .arg(QString::fromStdString(extension));

    fs::path output = dir / fileName.toStdString();

    const fs::path absoluteInput = fs::absolute(input);
    if (fs::absolute(output) == absoluteInput) {
        const QString safeName = QStringLiteral("%1_pixelated_%2px_%3_out.%4")
                                     .arg(QString::fromStdString(stem))
                                     .arg(targetWidth)
                                     .arg(paletteSuffix)
                                     .arg(QString::fromStdString(extension));
        output = dir / safeName.toStdString();
    }

    return QString::fromStdString(output.string());
}

bool PixelWorker::processOne(const QString &inputPath, const QString &mode, const QString &palette, QString *error)
{
    const cv::Mat input = cv::imread(inputPath.toStdString(), cv::IMREAD_UNCHANGED);
    if (input.empty()) { *error = QStringLiteral("OpenCV could not read the image"); return false; }
    cv::Mat bgr, alpha;
    if (input.channels() == 4) { cv::cvtColor(input, bgr, cv::COLOR_BGRA2BGR); cv::extractChannel(input, alpha, 3); }
    else if (input.channels() == 1) cv::cvtColor(input, bgr, cv::COLOR_GRAY2BGR);
    else bgr = input;
    const cv::Size size = mode == u"Smart auto" ? findBestPixelSize(bgr) : targetSize(bgr.size(), mode);
    cv::Mat small;
    cv::resize(bgr, small, size, 0.0, 0.0, cv::INTER_NEAREST);
    small = applyPalette(small, palette);
    if (!alpha.empty()) { cv::Mat scaledAlpha, output; cv::resize(alpha, scaledAlpha, size, 0.0, 0.0, cv::INTER_NEAREST); cv::cvtColor(small, output, cv::COLOR_BGR2BGRA); cv::insertChannel(scaledAlpha, output, 3); small = output; }
    const int targetWidth = std::max(size.width, size.height);
    const QString outputPath = outputPathFor(inputPath, targetWidth, palette);
    if (!cv::imwrite(outputPath.toStdString(), small)) { *error = QStringLiteral("OpenCV could not save the output"); return false; }
    return true;
}

void PixelWorker::processAll(const QString &mode, const QString &palette)
{
    if (m_processing || m_queue.isEmpty()) return;
    m_processing = true;
    emit processingChanged();
    const QStringList tasks = m_queue;
    m_processingFuture = QtConcurrent::run([this, tasks, mode, palette] {
        int succeeded = 0, failed = 0;
        for (int index = 0; index < tasks.size(); ++index) {
            QString error;
            const bool ok = processOne(tasks[index], mode, palette, &error);
            succeeded += ok; failed += !ok;
            const QString name = QFileInfo(tasks[index]).fileName();
            QMetaObject::invokeMethod(this, [this, index, total = tasks.size(), name, ok, error] {
                emit fileFinished(name, ok, ok ? QStringLiteral("Saved") : error);
                emit progressChanged(index + 1, total, name);
            }, Qt::QueuedConnection);
        }
        QMetaObject::invokeMethod(this, [this, succeeded, failed] {
            m_processing = false;
            emit processingChanged();
            emit processingFinished(succeeded, failed);
            emit statusChanged(QStringLiteral("Finished: %1 saved, %2 failed").arg(succeeded).arg(failed));
        }, Qt::QueuedConnection);
    });
}
