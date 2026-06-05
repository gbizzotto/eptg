
#include <cmath>

#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QMimeDatabase>
#include <QMatrix4x4>

#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QEventLoop>
#include <QTimer>
#include <QImage>
#include <QUrl>

#include <QPdfDocument>
#include <QImage>

#include "qexifimageheader.h"
#include "eptg/helpers.hpp"
#include "eptg/constants.hpp"

int get_column(const QTableWidgetItem *item) { return item->column(); }
int get_column(const QModelIndex      &item) { return item .column(); }
QVariant get_data(const QTableWidgetItem *item) { return item->data(Qt::DisplayRole); }
QVariant get_data(const QModelIndex      &item) { return item .data(Qt::DisplayRole); }

std::set<QString> names_from_list(const QListWidget * list)
{
    std::set<QString> result;
    for (int i=0 ; i<list->count() ; i++)
        if (list->item(i)->data(Qt::DisplayRole).toString().size() > 0)
            result.insert(list->item(i)->data(Qt::DisplayRole).toString());
    return result;
}

bool images_close(const QImage & left, const QImage & right, int allowed_difference)
{
    int diff = 0;
    for (int x=0 ; x<8 ; x++)
        for (int y=0 ; y<8 ; y++)
        {
            QRgb  left_pixel =  left.pixel(x, y);
            QRgb right_pixel = right.pixel(x, y);
            diff += (qRed  (left_pixel) - qRed  (right_pixel)) * (qRed  (left_pixel) - qRed  (right_pixel))
                  + (qGreen(left_pixel) - qGreen(right_pixel)) * (qGreen(left_pixel) - qGreen(right_pixel))
                  + (qBlue (left_pixel) - qBlue (right_pixel)) * (qBlue (left_pixel) - qBlue (right_pixel))
                  ;
            if (diff > allowed_difference)
                return false;
        }
    return true;
}

bool has_exif(const QString & full_path)
{
	QFile file(full_path);
	QExifImageHeader exif_header;
	file.open(QIODevice::ReadOnly);
	bool result = exif_header.loadFromJpeg(&file);
	file.close();
	return result;
}
bool has_exif_orientation(const QString & full_path)
{
	QFile file(full_path);
	QExifImageHeader exif_header;
	file.open(QIODevice::ReadOnly);
	bool result = exif_header.loadFromJpeg(&file) && exif_header.contains(QExifImageHeader::ImageTag::Orientation);
	file.close();
	return result;
}
unsigned int get_exif_orientation(const QString & full_path)
{
	QFile file(full_path);
	QExifImageHeader exif_header;
	if (   file.open(QIODevice::ReadOnly)
		&& exif_header.loadFromJpeg(&file)
		&& exif_header.contains(QExifImageHeader::ImageTag::Orientation))
	{
		return exif_header.value(QExifImageHeader::ImageTag::Orientation).toLong();
	}
	return 0;
}

std::tuple<QPixmap,QSize,int> make_image(const QString & full_path, const QSize & initial_size, const QSize & thumb_size)
{
    QSize orig_size;
    QFile file(full_path);
    int file_size = (int)file.size();

    QMimeDatabase db;
    QString mime_type = db.mimeTypeForFile(full_path, QMimeDatabase::MatchContent).name().toLower();

    QImage image;

    if (mime_type.startsWith("video/"))
    {
        // load middle frame
        image = extractMiddleFrame(full_path);
    }
    else if(mime_type.startsWith("application/pdf"))
    {
        image = renderPdfPage(full_path, 0, 3.0);
    }
    else
    {
        image = QImage(full_path);
    }

    if (image.isNull())
    {
        // load image with text
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            auto qb = file.read(1024);
            image = QImage(initial_size, QImage::Format_ARGB32_Premultiplied);
            QPainter painter(&image);
            painter.setFont(QFont("Courier New", 20));
            painter.fillRect(image.rect(), Qt::white);
            painter.drawText(image.rect(), Qt::AlignTop | Qt::AlignLeft, QString(qb).toHtmlEscaped());
        }
    }

	unsigned int exif_orientation = get_exif_orientation(full_path);
	if (exif_orientation > 1)
	{
		QPixmap pixmap;
		pixmap.convertFromImage(image);
        QMatrix4x4 rm;
		switch(exif_orientation)
		{
			case 2: rm.scale(-1,1); break;
            case 3: rm.rotate(180, 0, 0, 1); break;
			case 4: rm.scale(1,-1); break;
            case 5: rm.scale(-1,1); rm.rotate(270, 0, 0, 1); break;
            case 6: rm.rotate(90, 0, 0, 1); break;
            case 7: rm.scale(-1,1); rm.rotate(90, 0, 0, 1); break;
            case 8: rm.rotate(270, 0, 0, 1); break;
			default: break;
		}
        image = pixmap.transformed(rm.toTransform()).toImage();
	}
    orig_size = image.size();

	if (   thumb_size.isValid()
		&& (image.width() > thumb_size.width() || image.height() > thumb_size.height()))
	{
		image = image.scaled(thumb_size, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
	}

	return std::make_tuple(QPixmap::fromImage(image), orig_size, file_size);
}

QPixmap make_preview(const QString & base_path, const std::set<QString> & selected_items_text, const QSize & total_size)
{
	QPixmap image_result(total_size);
	image_result.fill(Qt::transparent);
	int cols = int(std::ceil(std::sqrt(selected_items_text.size())));
	int rows = int(std::ceil(selected_items_text.size() * 1.0 / cols));
	int w = image_result.width() / cols;
	int h = image_result.height() / rows;
	auto it = selected_items_text.begin();
	QSize thumb_size(w-IMG_BORDER, h-IMG_BORDER);
	for (int r = 0 ; r<rows ; ++r)
		for (int c = 0 ; c<cols && it!=selected_items_text.end() ; ++c,++it)
		{
			auto full_path = path::append(base_path, *it);
			QSize orig_size;
			QPixmap image;
			int file_size;
            std::tie(image, orig_size, file_size) = make_image(full_path, total_size, thumb_size);
			QRectF targetRect(w*c + (w-image.width())/2, h*r + (h-image.height())/2, image.width(), image.height());
			QRectF sourceRect(0, 0, image.width(), image.height());
			QPainter painter(&image_result);
			painter.drawPixmap(targetRect, image, sourceRect);
		}
	return image_result;
}


QString checksum_4k(const QString &fileName)
{
	QFile f(fileName);
	QCryptographicHash hash(QCryptographicHash::Sha256);
	if (f.open(QFile::ReadOnly))
	{
		QByteArray a = f.read(4096);
		if (a.length() > 0)
		{
			hash.addData(a);
			auto result = hash.result();
			return result.toHex();
		}
	}
	return "";
}

QImage extractMiddleFrame(const QString &videoFile)
{
    QMediaPlayer player;
    QVideoSink sink;

    player.setVideoSink(&sink);

    QImage result;

    QEventLoop loop;

    QObject::connect(
        &sink,
        &QVideoSink::videoFrameChanged,
        [&](const QVideoFrame &frame)
        {
            if (!frame.isValid())
                return;

            result = frame.toImage();

            if (!result.isNull())
                loop.quit();
        });

    QObject::connect(
        &player,
        &QMediaPlayer::durationChanged,
        [&](qint64 duration)
        {
            if (duration <= 0)
                return;

            player.setPosition(duration / 2);
            player.play();
        });

    QObject::connect(
        &player,
        &QMediaPlayer::errorOccurred,
        [&](QMediaPlayer::Error)
        {
            loop.quit();
        });

    QTimer timeout;
    timeout.setSingleShot(true);

    QObject::connect(
        &timeout,
        &QTimer::timeout,
        [&]()
        {
            loop.quit();
        });

    timeout.start(10000);

    player.setSource(QUrl::fromLocalFile(videoFile));

    loop.exec();

    player.stop();

    return result;
}

/*
QImage extractMiddleFrame(const QString &videoFile)
{
    // Create temporary directory
    QTemporaryDir tempDir;

    if (!tempDir.isValid())
        return {};

    QString output = tempDir.path() + "/frame.jpg";

    // First get duration with ffprobe
    QProcess probe;

    probe.start(
        "ffprobe",
        {
            "-v", "error",
            "-show_entries", "format=duration",
            "-of", "default=noprint_wrappers=1:nokey=1",
            videoFile
        });

    if (!probe.waitForFinished())
        return {};

    double duration = probe.readAllStandardOutput().trimmed().toDouble();
    if (duration <= 0.0)
        return {};

    double middle = duration / 2.0;

    // Extract one frame
    QProcess ffmpeg;

    ffmpeg.start(
        "ffmpeg",
        {
            "-y",
            "-ss", QString::number(middle),
            "-i", videoFile,
            "-frames:v", "1",
            output
        });

    if (!ffmpeg.waitForFinished())
        return {};

    if (!QFileInfo::exists(output))
        return {};

    return QImage(output);
}
*/

QImage renderPdfPage(const QString& pdfPath, int pageNumber, double scale)
{
    QPdfDocument doc;

    doc.load(pdfPath);

    if (doc.status() != QPdfDocument::Status::Ready)
        return {};

    if (pageNumber < 0 || pageNumber >= doc.pageCount())
        return {};

    QSizeF pageSize = doc.pagePointSize(pageNumber);
    QSize imageSize = (pageSize * scale).toSize();

    //QImage image(imageSize, QImage::Format_ARGB32);
    //image.fill(Qt::white);

    return doc.render(pageNumber, imageSize);
}
