#pragma once

#include <QWidget>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDateTime>
#include <QImage>
#include <QCryptographicHash>
#include <QTextDocument>
#include <QBuffer>
#include <QList>
#include <QUrl>

struct ClipboardItemData
{
    QDateTime dateTime;
    QString text;
    QString html;
    QList<QUrl> urls;
    QImage image;
    QStringList formats; // 可用格式列表
    QMap<QString, QByteArray> rawData; // 其他格式的原始数据

    constexpr static const char* clipboardItemCopyFormat = "application/nice-clipboard-history-item-copy";
    constexpr static const char* clipboardItemCopyFormatData = "NiceClipboard";

    bool hasText() const { return !text.isEmpty(); }
    bool hasHtml() const { return !html.isEmpty(); }
    bool hasUrls() const { return !urls.isEmpty(); }
    bool hasImage() const { return !image.isNull(); }
    bool hasFormat(const QString& format) const { return rawData.contains(format); }
    void setMimeData(const QMimeData* mimeData, QCryptographicHash::Algorithm algorithm, const QString& imageFmt)
    {
        formats = mimeData->formats();
        for (const QString& format : formats)
        {
            //if (format.contains("EnterpriseDataProtectionId"))
            //    continue; // 跳过企业数据保护相关的格式
            rawData[format] = mimeData->data(format);
        }
        if (mimeData->hasText())
            text = mimeData->text();
        if (mimeData->hasHtml())
            html = mimeData->html();
        if (mimeData->hasUrls())
            urls = mimeData->urls();
        if (mimeData->hasImage())
        {
            image = qvariant_cast<QImage>(mimeData->imageData());
            checkImageReference(algorithm, imageFmt);
        }

    }

    QMimeData* toMimeData(QWidget* parent = nullptr) const
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setParent(parent);
        for (const QString& format : formats)
            mimeData->setData(format, rawData.value(format));
        if (hasText())
            mimeData->setText(text);
        if (hasHtml())
            mimeData->setHtml(html);
        if (hasUrls())
            mimeData->setUrls(urls);
        if (hasImage())
            mimeData->setImageData(image);
        return mimeData;
    }

    QDataStream& serialize(QDataStream& out) const {
        out << dateTime;
        out << text;
        out << html;
        out << urls;
        out << image;
        out << formats;
        out << rawData;
        return out;
    }

    QDataStream& deserialize(QDataStream& in) {
        in >> dateTime;
        in >> text;
        in >> html;
        in >> urls;
        in >> image;
        in >> formats;
        in >> rawData;
        return in;
    }


    const QDateTime& getDateTime() const { return dateTime; }
    const QString& getText() const { return text; }
    const QString& getHtml() const { return html; }
    const QList<QUrl>& getUrls() const { return urls; }
    const QImage& getImage() const { return image; }
    const QMap<QString, QByteArray>& getRawData() const { return rawData; }
    const QStringList& getFormats() const { return formats; }
    QTextDocument* getCachedHtmlTextDocument() const {
        if (!cachedHtmlTextDoc)
            cachedHtmlTextDoc = new QTextDocument();
        if (cachedHtmlTextDoc && cachedHtmlTextDoc->isEmpty() && !html.isEmpty())
            cachedHtmlTextDoc->setHtml(html);
        return cachedHtmlTextDoc;
    }
    QByteArray getCachedImageHash(QCryptographicHash::Algorithm algorithm, const QString& imageFmt) const {
        if ((cachedImageHash.isEmpty() || cachedImageHashAlgorithm != algorithm || cachedImageFormat != imageFmt)
            && !image.isNull())
        {
            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            if (image.save(&buffer, imageFmt.toUtf8()))
            {
                cachedImageHash = QCryptographicHash::hash(imageData, algorithm);
                cachedImageHashAlgorithm = algorithm;
                cachedImageFormat = imageFmt;
            }
            else
            {
                cachedImageHash.clear();
                cachedImageFormat = "";
                cachedImageHashAlgorithm = QCryptographicHash::NumAlgorithms;
            }
        }
        if (cachedImageHash.isEmpty())
            cachedImageHash.clear();
        return cachedImageHash;
    }

    QMimeData* copyToClipboard(QWidget* parent = nullptr) const {
        auto mimeData = this->toMimeData(parent);
        mimeData->setData(clipboardItemCopyFormat, clipboardItemCopyFormatData);
        QGuiApplication::clipboard()->setMimeData(mimeData);
        return mimeData;
    }

    QMimeData* copyToClipboard(QStringList formatsToCopy, QWidget* parent = nullptr) const {
        auto mimeData = new QMimeData();
        mimeData->setParent(parent);
        for (const auto& format : formatsToCopy)
            mimeData->setData(format, rawData.value(format));
        mimeData->setData(clipboardItemCopyFormat, clipboardItemCopyFormatData);
        QGuiApplication::clipboard()->setMimeData(mimeData);
        return mimeData;
    }

    static bool shouldHandleClipboardEvent(const QMimeData* mimeData) {
        return mimeData && mimeData->formats().size() && (!mimeData->hasFormat(clipboardItemCopyFormat) || mimeData->data(clipboardItemCopyFormat) != clipboardItemCopyFormatData);
    }
    bool shouldHandleClipboardEvent() const {
        return this->formats.size() && (!formats.contains(clipboardItemCopyFormat) || rawData.value(clipboardItemCopyFormat) != clipboardItemCopyFormatData);
    }
    void updateDateTime() {
        this->dateTime = QDateTime::currentDateTime();
    }
    ClipboardItemData(bool updateDateTime = false) {
        if (updateDateTime)
            this->dateTime = QDateTime::currentDateTime();
    }
    ClipboardItemData(const ClipboardItemData& other) = default;
    ClipboardItemData(ClipboardItemData&& other) noexcept = default;
    ClipboardItemData& operator=(const ClipboardItemData& other) = default;
    ~ClipboardItemData() {
        if (cachedHtmlTextDoc)
            delete cachedHtmlTextDoc;
    }


    void loadImageFile(QString filePath, QCryptographicHash::Algorithm algorithm, const QString& imageFmt) {
        if (imageReferenceHashMap.contains(filePath))
        {
            auto& ref = imageReferenceHashMap[filePath];
            image = ref.image;
            ++ref.counter;
            return;
        }
        image.load(filePath);
        checkImageReference(algorithm, imageFmt);
    }

private:
    friend QDataStream& operator<<(QDataStream& out, const ClipboardItemData& item);
    friend QDataStream& operator>>(QDataStream& in, ClipboardItemData& item);
    mutable QTextDocument* cachedHtmlTextDoc{ nullptr };
    mutable QByteArray cachedImageHash;
    mutable QCryptographicHash::Algorithm cachedImageHashAlgorithm{ QCryptographicHash::NumAlgorithms };
    mutable QString cachedImageFormat;

    struct HashImageReference {
        //QByteArray imageData;
        QImage image;
        size_t counter = 0;
    };
    static QHash<QString, HashImageReference> imageReferenceHashMap; // image hex hash -> HashImageReference

    void checkImageReference(QCryptographicHash::Algorithm algorithm, const QString& imageFmt) {
        if (image.isNull())
            return;
        auto hash = getCachedImageHash(algorithm, imageFmt);
        QString hexHashText = hash.toHex();
        if (imageReferenceHashMap.contains(hexHashText))
        {
            auto& ref = imageReferenceHashMap[hexHashText];
            image = ref.image;
            ++ref.counter;
            return;
        }
        auto& ref = imageReferenceHashMap[hexHashText];
        ref.image = image;
        ref.counter = 1;
    }
};


inline QHash<QString, ClipboardItemData::HashImageReference> ClipboardItemData::imageReferenceHashMap;

Q_DECLARE_METATYPE(ClipboardItemData)

// 序列化
inline QDataStream& operator<<(QDataStream& out, const ClipboardItemData& item)
{
    return item.serialize(out);
}
// 反序列化
inline QDataStream& operator>>(QDataStream& in, ClipboardItemData& item)
{
    return item.deserialize(in);
}

