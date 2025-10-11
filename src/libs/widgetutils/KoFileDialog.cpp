/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2013-2014 Yue Liu <yue.liu@mail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoFileDialog.h"
#include <QDebug>
#include <QFileDialog>
#include <QApplication>
#include <QImageReader>
#include <QClipboard>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QUrl>
#include <QMimeDatabase>
#include <QMimeType>

class Q_DECL_HIDDEN KoFileDialog::Private
{
public:
    Private(QWidget *parent_,
            KoFileDialog::DialogType dialogType_,
            const QString caption_,
            const QString defaultDir_,
            const QString dialogName_)
        : parent(parent_)
        , type(dialogType_)
        , dialogName(dialogName_)
        , caption(caption_)
        , defaultDirectory(defaultDir_)
        , filterList(QStringList())
        , defaultFilter(QString())
        , useStaticForNative(false)
        , hideDetails(false)
        , swapExtensionOrder(false)
    {
        // Force the native file dialogs on Windows. Except for KDE, the native file dialogs are only possible
        // using the static methods. The Qt documentation is wrong here, if it means what it says " By default,
        // the native file dialog is used unless you use a subclass of QFileDialog that contains the Q_OBJECT
        // macro."
#ifdef Q_OS_WIN
        useStaticForNative = true;
#endif
        // Non-static KDE file is broken when called with QFileDialog::AcceptSave:
        // then the directory above defaultdir is opened, and defaultdir is given as the default file name...
        //
        // So: in X11, use static methods inside KDE, which give working native dialogs, but non-static outside
        // KDE, which gives working Qt dialogs.
        //
        // Only show the GTK dialog in Gnome, where people deserve it
#ifdef HAVE_X11
        if (qgetenv("KDE_FULL_SESSION").size() > 0) {
            useStaticForNative = true;
        }
        if (qgetenv("XDG_CURRENT_DESKTOP") == "GNOME") {
            useStaticForNative = true;
            QClipboard *cb = QApplication::clipboard();
            cb->blockSignals(true);
            swapExtensionOrder = true;
        }

#endif
    }

    ~Private()
    {
        if (qgetenv("XDG_CURRENT_DESKTOP") == "GNOME") {
            useStaticForNative = true;
            QClipboard *cb = QApplication::clipboard();
            cb->blockSignals(false);
        }
    }

    QWidget *parent;
    KoFileDialog::DialogType type;
    QString dialogName;
    QString caption;
    QString defaultDirectory;
    QStringList filterList;
    QString defaultFilter;
    QScopedPointer<QFileDialog> fileDialog;
    QMimeType mimeType;
    bool useStaticForNative;
    bool hideDetails;
    bool swapExtensionOrder;
};

KoFileDialog::KoFileDialog(QWidget *parent,
                           KoFileDialog::DialogType type,
                           const QString &dialogName)
    : d(new Private(parent, type, QString(), getUsedDir(dialogName).toString(QUrl::FullyEncoded), dialogName))
{
}

KoFileDialog::~KoFileDialog()
{
    delete d;
}

void KoFileDialog::setCaption(const QString &caption)
{
    d->caption = caption;
}

void KoFileDialog::setDefaultDir(const QString &defaultDir, bool override)
{
    if (override || d->defaultDirectory.isEmpty()) {
        QFileInfo f(defaultDir);
        d->defaultDirectory = f.absoluteFilePath();
    } else {
        QFileInfo df(d->defaultDirectory);
        if (!df.isFile()) {
            QFileInfo f(defaultDir);
            if (df.exists()) {
                df.setFile(df.filePath(), f.fileName());
                d->defaultDirectory = df.absoluteFilePath();
            } else {
                QFileInfo f(defaultDir);
                d->defaultDirectory = f.absoluteFilePath();
            }
        }
    }
}

void KoFileDialog::setOverrideDir(const QString &overrideDir)
{
    d->defaultDirectory = overrideDir;
}

void KoFileDialog::setImageFilters()
{
    QStringList imageMimeTypes;
    const QList<QByteArray> types = QImageReader::supportedMimeTypes();
    for (const QByteArray &mimeType : types) {
        imageMimeTypes << QLatin1String(mimeType);
    }
    setMimeTypeFilters(imageMimeTypes);
}

void KoFileDialog::setNameFilter(const QString &filter)
{
    d->filterList.clear();
    if (d->type == KoFileDialog::SaveFile) {
        QStringList mimeList;
        d->filterList << splitNameFilter(filter, &mimeList);
        d->defaultFilter = d->filterList.first();
    }
    else {
        d->filterList << filter;
    }
}

void KoFileDialog::setNameFilters(const QStringList &filterList,
                                  QString defaultFilter)
{
    d->filterList.clear();

    if (d->type == KoFileDialog::SaveFile) {
        QStringList mimeList;
        for (const QString &filter : filterList) {
            d->filterList << splitNameFilter(filter, &mimeList);
        }

        if (!defaultFilter.isEmpty()) {
            mimeList.clear();
            QStringList defaultFilters = splitNameFilter(defaultFilter, &mimeList);
            if (defaultFilters.size() > 0) {
                defaultFilter = defaultFilters.first();
            }
        }
    }
    else {
        d->filterList = filterList;
    }
    d->defaultFilter = defaultFilter;

}

void KoFileDialog::setMimeTypeFilters(const QStringList &filterList,
                                      QString defaultFilter)
{
    d->filterList = getFilterStringListFromMime(filterList, true);

    if (!defaultFilter.isEmpty()) {
        QStringList defaultFilters = getFilterStringListFromMime(QStringList() << defaultFilter, false);
        if (defaultFilters.size() > 0) {
            defaultFilter = defaultFilters.first();
        }
    }
    d->defaultFilter = defaultFilter;
}

void KoFileDialog::setHideNameFilterDetailsOption()
{
    d->hideDetails = true;
}

QStringList KoFileDialog::nameFilters() const
{
    return d->filterList;
}

QString KoFileDialog::selectedNameFilter() const
{
    if (!d->useStaticForNative) {
        return d->fileDialog->selectedNameFilter();
    }
    else {
        return d->defaultFilter;
    }
}

QString KoFileDialog::selectedMimeType() const
{
    if (d->mimeType.isValid()) {
        return d->mimeType.name();
    }
    else {
        return QString();
    }
}

void KoFileDialog::createFileDialog()
{
    d->fileDialog.reset(new QFileDialog(d->parent, d->caption, directory().toString()));

    if (d->type == SaveFile) {
        d->fileDialog->setAcceptMode(QFileDialog::AcceptSave);
        d->fileDialog->setFileMode(QFileDialog::AnyFile);
    }
    else { // open / import

        d->fileDialog->setAcceptMode(QFileDialog::AcceptOpen);

        if (d->type == ImportDirectory
                || d->type == OpenDirectory)
        {
            d->fileDialog->setFileMode(QFileDialog::Directory);
            d->fileDialog->setOption(QFileDialog::ShowDirsOnly, true);
        }
        else { // open / import file(s)
            if (d->type == OpenFile
                    || d->type == ImportFile)
            {
                d->fileDialog->setFileMode(QFileDialog::ExistingFile);
            }
            else { // files
                d->fileDialog->setFileMode(QFileDialog::ExistingFiles);
            }
        }
    }

    d->fileDialog->setNameFilters(d->filterList);
    if (!d->defaultFilter.isEmpty()) {
        d->fileDialog->selectNameFilter(d->defaultFilter);
    }

    if (d->type == ImportDirectory ||
            d->type == ImportFile || d->type == ImportFiles ||
            d->type == SaveFile) {
        d->fileDialog->setWindowModality(Qt::WindowModal);
    }

    if (d->hideDetails) {
        d->fileDialog->setOption(QFileDialog::HideNameFilterDetails);
    }

    connect(d->fileDialog.data(), &QFileDialog::filterSelected, this, &KoFileDialog::filterSelected);
    connect(d->fileDialog.data(), &QDialog::finished, this, &KoFileDialog::finished);
}

QString KoFileDialog::filename()
{
    QUrl url;
    auto directory = this->directory();
    if (!d->useStaticForNative) {

        if (!d->fileDialog) {
            createFileDialog();
        }

        if (d->fileDialog->exec() == QDialog::Accepted) {
            url = d->fileDialog->selectedUrls().first();
        }
    }
    else {
        switch (d->type) {
        case OpenFile:
        {
            url = QFileDialog::getOpenFileUrl(d->parent,
                                               d->caption,
                                               directory,
                                               d->filterList.join(QStringLiteral(";;")),
                                               &d->defaultFilter);
            break;
        }
        case OpenDirectory:
        {
            url = QUrl::fromUserInput(QFileDialog::getExistingDirectory(d->parent,
                                                    d->caption,
                                                    directory.toString(),
                                                    QFileDialog::ShowDirsOnly));
            break;
        }
        case ImportFile:
        {
            url = QFileDialog::getOpenFileUrl(d->parent,
                                               d->caption,
                                               directory,
                                               d->filterList.join(QStringLiteral(";;")),
                                               &d->defaultFilter);
            break;
        }
        case ImportDirectory:
        {
            url = QUrl::fromUserInput(QFileDialog::getExistingDirectory(d->parent,
                                                    d->caption,
                                                    directory.toString(),
                                                    QFileDialog::ShowDirsOnly));
            break;
        }
        case SaveFile:
        {
            url = QFileDialog::getSaveFileUrl(d->parent,
                                               d->caption,
                                               directory,
                                               d->filterList.join(QStringLiteral(";;")),
                                               &d->defaultFilter);
            break;
        }
        default:
            ;
        }
    }

    if (url.isValid()) {
        QMimeDatabase db;
        d->mimeType = db.mimeTypeForUrl(url);
        saveUsedDir(url.toString(QUrl::FullyEncoded), d->dialogName);
    }
    return url.toString(QUrl::FullyEncoded);
}

QList<QUrl> KoFileDialog::filenames()
{
    QList<QUrl> urls;

    if (!d->useStaticForNative) {
        if (!d->fileDialog) {
            createFileDialog();
        }
        if (d->fileDialog->exec() == QDialog::Accepted) {
            urls = d->fileDialog->selectedUrls();
        }
    }
    else {
        switch (d->type) {
        case OpenFiles:
        case ImportFiles:
        {
            urls = QFileDialog::getOpenFileUrls(d->parent,
                                                 d->caption,
                                                 directory(),
                                                 d->filterList.join(QStringLiteral(";;")),
                                                 &d->defaultFilter);
            break;
        }
        default:
            ;
        }
    }
    if (!urls.isEmpty()) {
        saveUsedDir(urls.first().toString(QUrl::FullyEncoded), d->dialogName);
    }
    return urls;
}

void KoFileDialog::filterSelected(const QString &filter)
{
    // "Windows BMP image (*.bmp)";
    int start = filter.lastIndexOf(QStringLiteral("*.")) + 2;
    int end = filter.lastIndexOf(QLatin1Char(')'));
    int n = end - start;
    QString extension = filter.mid(start, n);
    d->defaultFilter = filter;
    d->fileDialog->setDefaultSuffix(extension);
}

QStringList KoFileDialog::splitNameFilter(const QString &nameFilter, QStringList *mimeList)
{
    Q_ASSERT(mimeList);

    QStringList filters;
    QString description;

    if (nameFilter.contains(QLatin1Char('('))) {
        description = nameFilter.left(nameFilter.indexOf(QLatin1Char('(')) -1).trimmed();
    }

    const QStringList entries = nameFilter.mid(nameFilter.indexOf(QLatin1Char('(')) + 1).split(QLatin1Char(' '),Qt::SkipEmptyParts);

    for (QString entry : entries) {

        entry = entry.remove(QLatin1Char('*'));
        entry = entry.remove(QLatin1Char(')'));

        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForName(QStringLiteral("bla") + entry);
        if (mime.name() != QStringLiteral("application/octet-stream")) {
            if (!mimeList->contains(mime.name())) {
                mimeList->append(mime.name());
                filters.append(mime.comment() + QStringLiteral(" (*") + entry + QLatin1Char(')'));
            }
        }
        else {
            filters.append(entry.remove(QLatin1Char('.')).toUpper() + QLatin1Char(' ') + description + QStringLiteral(" (*.") + entry + QLatin1Char(')'));
        }
    }

    return filters;
}

const QStringList KoFileDialog::getFilterStringListFromMime(const QStringList &mimeList,
                                                            bool withAllSupportedEntry)
{
    QStringList mimeSeen;

    QStringList ret;
    if (withAllSupportedEntry) {
        ret << QString();
    }

    for (QStringList::ConstIterator
         it = mimeList.begin(); it != mimeList.end(); ++it) {
        QMimeDatabase db;
        QMimeType mimeType = db.mimeTypeForName(*it);
        if (!mimeType.isValid()) {
            continue;
        }
        if (!mimeSeen.contains(mimeType.name())) {
            QString oneFilter;
            QStringList patterns = mimeType.globPatterns();
            QStringList::ConstIterator jt;
            for (jt = patterns.constBegin(); jt != patterns.constEnd(); ++jt) {
                if (d->swapExtensionOrder) {
                    oneFilter.prepend(*jt + QLatin1Char(' '));
                    if (withAllSupportedEntry) {
                        ret[0].prepend(*jt + QLatin1Char(' '));
                    }
                }
                else {
                    oneFilter.append(*jt + QLatin1Char(' '));
                    if (withAllSupportedEntry) {
                        ret[0].append(*jt + QLatin1Char(' '));
                    }
                }
            }
            oneFilter = mimeType.comment() + QStringLiteral(" (") + oneFilter + QLatin1Char(')');
            ret << oneFilter;
            mimeSeen << mimeType.name();
        }
    }

    if (withAllSupportedEntry) {
        ret[0] = i18n("All supported formats") + QStringLiteral(" (") + ret[0] + QLatin1Char(')');
    }
    return ret;
}

QUrl KoFileDialog::directory()
{
    auto url = getUsedDir(d->dialogName);
    if (!url.isValid()) {
        url = QUrl::fromUserInput(d->defaultDirectory);
    }
    return url;
}

QUrl KoFileDialog::getUsedDir(const QString &dialogName)
{
    if (dialogName.isEmpty()) return QUrl();

    KConfigGroup group =  KSharedConfig::openConfig()->group(QStringLiteral("File Dialogs"));
    QUrl dir = QUrl::fromUserInput(group.readEntry(dialogName));

    return dir;
}

void KoFileDialog::saveUsedDir(const QString &fileName,
                               const QString &dialogName)
{
    if (dialogName.isEmpty()) {
        return;
    }
    const auto url = QUrl::fromUserInput(fileName);
    if (url.isValid()) {
        KConfigGroup group = KSharedConfig::openConfig()->group(QStringLiteral("File Dialogs"));
        group.writeEntry(dialogName, url.adjusted(QUrl::RemoveFilename).toString(QUrl::FullyEncoded));
    }
}

void KoFileDialog::setVisible(bool value)
{
    d->fileDialog.data()->setVisible(value);
    if (value) {
        d->fileDialog.data()->raise();
        d->fileDialog.data()->activateWindow();
    }
}
