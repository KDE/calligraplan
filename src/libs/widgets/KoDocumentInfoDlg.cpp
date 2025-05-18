/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
                 2006 Martin Pfeiffer <hubipete@gmx.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoDocumentInfoDlg.h"
#include "WidgetsDebug.h"

#include "ui_koDocumentInfoAboutWidget.h"
#include "ui_koDocumentInfoAuthorWidget.h"

#include "KoDocumentInfo.h"
#include "KoDocumentBase.h"
#include "KoGlobal.h"
#ifdef QCA2
#include <KoEncryptionChecker.h>
#endif
#include "KoPageWidgetItem.h"
//#include <KoDocumentRdfBase.h>
#include <KoIcon.h>

#include <KLocalizedString>
#include <KIconLoader>

#include <KMessageBox>
#include <KMainWindow>
#include <KoDialog.h>

#include <QUrl>
#include <QLineEdit>
#include <QDateTime>
#include <QMimeDatabase>
#include <QMimeType>


// see KoIcon.h
#define koSmallPixmap(name) KIconLoader::global()->loadIcon(QStringLiteral(name), KIconLoader::Small)

class KoPageWidgetItemAdapter : public KPageWidgetItem
{
Q_OBJECT
public:
    KoPageWidgetItemAdapter(KoPageWidgetItem *item)
      : KPageWidgetItem(item->widget(), item->name())
      , m_item(item)
    {
        setHeader(item->name());
        setIcon(QIcon::fromTheme(item->iconName()));
    }
    ~KoPageWidgetItemAdapter() override { delete m_item; }

    bool shouldDialogCloseBeVetoed() { return m_item->shouldDialogCloseBeVetoed(); }
    void apply() { m_item->apply(); }

private:
    KoPageWidgetItem * const m_item;
};


class KoDocumentInfoDlg::KoDocumentInfoDlgPrivate
{
public:
    KoDocumentInfoDlgPrivate() :
        toggleEncryption(false),
        applyToggleEncryption(false),
        documentSaved(false) {}
    ~KoDocumentInfoDlgPrivate() {}

    KoDocumentInfo* info;
    QList<KPageWidgetItem*> pages;
    Ui::KoDocumentInfoAboutWidget* aboutUi;
    Ui::KoDocumentInfoAuthorWidget* authorUi;

    bool toggleEncryption;
    bool applyToggleEncryption;
    bool documentSaved;
};


KoDocumentInfoDlg::KoDocumentInfoDlg(QWidget* parent, KoDocumentInfo* docInfo)
    : KPageDialog(parent)
    , d(new KoDocumentInfoDlgPrivate)
{
    d->info = docInfo;

    setWindowTitle(i18n("Document Information"));
//    setInitialSize(QSize(500, 500));
    setFaceType(KPageDialog::List);
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    button(QDialogButtonBox::Ok)->setDefault(true);

    d->aboutUi = new Ui::KoDocumentInfoAboutWidget();
    QWidget *infodlg = new QWidget();
    d->aboutUi->setupUi(infodlg);
#ifdef QCA2
    if (!KoEncryptionChecker::isEncryptionSupported()) {
#endif
        d->aboutUi->lblEncryptedDesc->setVisible(false);
        d->aboutUi->lblEncrypted->setVisible(false);
        d->aboutUi->pbEncrypt->setVisible(false);
        d->aboutUi->lblEncryptedPic->setVisible(false);
#ifdef QCA2
    }
#endif
    d->aboutUi->cbLanguage->addItems(KoGlobal::listOfLanguages());
    d->aboutUi->cbLanguage->setCurrentIndex(-1);

    KPageWidgetItem *page = new KPageWidgetItem(infodlg, i18n("General"));
    page->setHeader(i18n("General"));

    // Ugly hack, the mimetype should be a parameter, instead
    KoDocumentBase* doc = dynamic_cast< KoDocumentBase* >(d->info->parent());
    if (doc) {
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForName(QString::fromLatin1(doc->mimeType()));
        if (mime.isValid()) {
            page->setIcon(QIcon::fromTheme(mime.iconName()));
        }
    } else {
        // hide all entries not used in pages for KoDocumentInfoPropsPage
        d->aboutUi->filePathInfoLabel->setVisible(false);
        d->aboutUi->filePathLabel->setVisible(false);
        d->aboutUi->filePathSeparatorLine->setVisible(false);
        d->aboutUi->lblTypeDesc->setVisible(false);
        d->aboutUi->lblType->setVisible(false);
    }
    addPage(page);
    d->pages.append(page);

    initAboutTab();

    d->authorUi = new Ui::KoDocumentInfoAuthorWidget();
    QWidget *authordlg = new QWidget();
    d->authorUi->setupUi(authordlg);
    page = new KPageWidgetItem(authordlg, i18n("Author"));
    page->setHeader(i18n("Last saved by"));
    page->setIcon(koIcon("user-identity"));
    addPage(page);
    d->pages.append(page);

    initAuthorTab();
}

KoDocumentInfoDlg::~KoDocumentInfoDlg()
{
    delete d->authorUi;
    delete d->aboutUi;
    delete d;
}

void KoDocumentInfoDlg::accept()
{
    // check if any pages veto the close
    for (KPageWidgetItem* item : std::as_const(d->pages)) {
        KoPageWidgetItemAdapter *page = dynamic_cast<KoPageWidgetItemAdapter*>(item);
        if (page) {
            if (page->shouldDialogCloseBeVetoed()) {
                return;
            }
        }
    }

    // all fine, go and apply
    saveAboutData();
    for (KPageWidgetItem* item : std::as_const(d->pages)) {
        KoPageWidgetItemAdapter *page = dynamic_cast<KoPageWidgetItemAdapter*>(item);
        if (page) {
            page->apply();
        }
    }

    KPageDialog::accept();
}

bool KoDocumentInfoDlg::isDocumentSaved()
{
    return d->documentSaved;
}

void KoDocumentInfoDlg::initAboutTab()
{
    KoDocumentBase* doc = dynamic_cast< KoDocumentBase* >(d->info->parent());

    if (doc) {
        d->aboutUi->filePathLabel->setText(doc->localFilePath());
    }

    d->aboutUi->leTitle->setText(d->info->aboutInfo("title"));
    d->aboutUi->leSubject->setText(d->info->aboutInfo("subject"));
    QString language = KoGlobal::languageFromTag(d->info->aboutInfo("language"));
    d->aboutUi->cbLanguage->setCurrentIndex(d->aboutUi->cbLanguage->findText(language));

    d->aboutUi->leKeywords->setToolTip(i18n("Use ';' (Example: Office;KDE;Calligra)"));
    if (!d->info->aboutInfo("keyword").isEmpty())
        d->aboutUi->leKeywords->setText(d->info->aboutInfo("keyword"));

    d->aboutUi->meComments->setPlainText(d->info->aboutInfo("description"));
    if (doc && !doc->mimeType().isEmpty()) {
        QMimeDatabase db;
        QMimeType docmime = db.mimeTypeForName(QLatin1String(doc->mimeType()));
        if (docmime.isValid())
            d->aboutUi->lblType->setText(docmime.comment());
    }
    if (!d->info->aboutInfo("creation-date").isEmpty()) {
        QDateTime t = QDateTime::fromString(d->info->aboutInfo("creation-date"),
                                            Qt::ISODate);
        QString s = QLocale().toString(t);
        d->aboutUi->lblCreated->setText(s + QStringLiteral(", ") +
                                        d->info->aboutInfo("initial-creator"));
    }

    if (!d->info->aboutInfo("date").isEmpty()) {
        QDateTime t = QDateTime::fromString(d->info->aboutInfo("date"), Qt::ISODate);
        QString s = QLocale().toString(t);
        d->aboutUi->lblModified->setText(s + QStringLiteral(", ") + d->info->authorInfo("creator"));
    }

    d->aboutUi->lblRevision->setText(d->info->aboutInfo("editing-cycles"));

    if (doc && (doc->supportedSpecialFormats() & KoDocumentBase::SaveEncrypted)) {
        if (doc->specialOutputFlag() == KoDocumentBase::SaveEncrypted) {
            if (d->toggleEncryption) {
                d->aboutUi->lblEncrypted->setText(i18n("This document will be decrypted"));
                d->aboutUi->lblEncryptedPic->setPixmap(koSmallPixmap("object-unlocked"));
                d->aboutUi->pbEncrypt->setText(i18n("Do not decrypt"));
            } else {
                d->aboutUi->lblEncrypted->setText(i18n("This document is encrypted"));
                d->aboutUi->lblEncryptedPic->setPixmap(koSmallPixmap("object-locked"));
                d->aboutUi->pbEncrypt->setText(i18n("D&ecrypt"));
            }
        } else {
            if (d->toggleEncryption) {
                d->aboutUi->lblEncrypted->setText(i18n("This document will be encrypted."));
                d->aboutUi->lblEncryptedPic->setPixmap(koSmallPixmap("object-locked"));
                d->aboutUi->pbEncrypt->setText(i18n("Do not encrypt"));
            } else {
                d->aboutUi->lblEncrypted->setText(i18n("This document is not encrypted"));
                d->aboutUi->lblEncryptedPic->setPixmap(koSmallPixmap("object-unlocked"));
                d->aboutUi->pbEncrypt->setText(i18n("&Encrypt"));
            }
        }
    } else {
        d->aboutUi->lblEncrypted->setText(i18n("This document does not support encryption"));
        d->aboutUi->pbEncrypt->setEnabled(false);
    }
    connect(d->aboutUi->pbReset, &QAbstractButton::clicked,
            this, &KoDocumentInfoDlg::slotResetMetaData);
    connect(d->aboutUi->pbEncrypt, &QAbstractButton::clicked,
            this, &KoDocumentInfoDlg::slotToggleEncryption);
}

void KoDocumentInfoDlg::initAuthorTab()
{
    d->authorUi->fullName->setText(d->info->authorInfo("creator"));
    d->authorUi->initials->setText(d->info->authorInfo("initial"));
    d->authorUi->title->setText(d->info->authorInfo("author-title"));
    d->authorUi->company->setText(d->info->authorInfo("company"));
    d->authorUi->email->setText(d->info->authorInfo("email"));
    d->authorUi->phoneWork->setText(d->info->authorInfo("telephone-work"));
    d->authorUi->phoneHome->setText(d->info->authorInfo("telephone"));
    d->authorUi->fax->setText(d->info->authorInfo("fax"));
    d->authorUi->country->setText(d->info->authorInfo("country"));
    d->authorUi->postal->setText(d->info->authorInfo("postal-code"));
    d->authorUi->city->setText(d->info->authorInfo("city"));
    d->authorUi->street->setText(d->info->authorInfo("street"));
    d->authorUi->position->setText(d->info->authorInfo("position"));
}

void KoDocumentInfoDlg::saveAboutData()
{
    d->info->setAboutInfo("keyword", d->aboutUi->leKeywords->text());
    d->info->setAboutInfo("title", d->aboutUi->leTitle->text());
    d->info->setAboutInfo("subject", d->aboutUi->leSubject->text());
    d->info->setAboutInfo("description", d->aboutUi->meComments->toPlainText());
    d->info->setAboutInfo("language", KoGlobal::tagOfLanguage(d->aboutUi->cbLanguage->currentText()));
    d->applyToggleEncryption = d->toggleEncryption;
}

void KoDocumentInfoDlg::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event);

    // Saving encryption implies saving the document, this is done after closing the dialog
    // TODO: shouldn't this be skipped if cancel is pressed?
    saveEncryption();
}

void KoDocumentInfoDlg::slotResetMetaData()
{
    d->info->resetMetaData();

    if (!d->info->aboutInfo("creation-date").isEmpty()) {
        QDateTime t = QDateTime::fromString(d->info->aboutInfo("creation-date"),
                                            Qt::ISODate);
        QString s = QLocale().toString(t);
        d->aboutUi->lblCreated->setText(s + QStringLiteral(", ") +
                                        d->info->aboutInfo("initial-creator"));
    }

    if (!d->info->aboutInfo("date").isEmpty()) {
        QDateTime t = QDateTime::fromString(d->info->aboutInfo("date"), Qt::ISODate);
        QString s = QLocale().toString(t);
        d->aboutUi->lblModified->setText(s + QStringLiteral(", ") + d->info->authorInfo("creator"));
    }

    d->aboutUi->lblRevision->setText(d->info->aboutInfo("editing-cycles"));
}

void KoDocumentInfoDlg::slotToggleEncryption()
{
    KoDocumentBase* doc = dynamic_cast< KoDocumentBase* >(d->info->parent());
    if (!doc)
        return;

    d->toggleEncryption = !d->toggleEncryption;

    if (doc->specialOutputFlag() == KoDocumentBase::SaveEncrypted) {
        if (d->toggleEncryption) {
            d->aboutUi->lblEncrypted->setText(i18n("This document will be decrypted"));
            d->aboutUi->lblEncryptedPic->setPixmap(koSmallPixmap("object-unlocked"));
            d->aboutUi->pbEncrypt->setText(i18n("Do not decrypt"));
        } else {
            d->aboutUi->lblEncrypted->setText(i18n("This document is encrypted"));
            d->aboutUi->lblEncryptedPic->setPixmap(koSmallPixmap("object-locked"));
            d->aboutUi->pbEncrypt->setText(i18n("D&ecrypt"));
        }
    } else {
        if (d->toggleEncryption) {
            d->aboutUi->lblEncrypted->setText(i18n("This document will be encrypted."));
            d->aboutUi->lblEncryptedPic->setPixmap(koSmallPixmap("object-locked"));
            d->aboutUi->pbEncrypt->setText(i18n("Do not encrypt"));
        } else {
            d->aboutUi->lblEncrypted->setText(i18n("This document is not encrypted"));
            d->aboutUi->lblEncryptedPic->setPixmap(koSmallPixmap("object-unlocked"));
            d->aboutUi->pbEncrypt->setText(i18n("&Encrypt"));
        }
    }
}

void KoDocumentInfoDlg::saveEncryption()
{
    if (!d->applyToggleEncryption)
        return;

    KoDocumentBase* doc = dynamic_cast< KoDocumentBase* >(d->info->parent());
    if (!doc)
        return;

    KMainWindow* mainWindow = dynamic_cast< KMainWindow* >(parent());
    bool saveas = false;
    if (doc->specialOutputFlag() == KoDocumentBase::SaveEncrypted) {
        // Decrypt
        if (KMessageBox::warningContinueCancel(
                    this,
                    i18n("<qt>Decrypting the document will remove the password protection from it."
                         "<p>Do you still want to decrypt the file?</qt>"),
                    i18n("Confirm Decrypt"),
                    KGuiItem(i18n("Decrypt")),
                    KStandardGuiItem::cancel(),
                    QStringLiteral("DecryptConfirmation")
                    ) != KMessageBox::Continue) {
            return;
        }
        bool modified = doc->isModified();
        doc->setOutputMimeType(doc->outputMimeType(), doc->specialOutputFlag() & ~KoDocumentBase::SaveEncrypted);
        if (!mainWindow) {
            KMessageBox::information(
                        this,
                        i18n("<qt>Your document could not be saved automatically."
                             "<p>To complete the decryption, please save the document.</qt>"),
                        i18n("Save Document"),
                        QStringLiteral("DecryptSaveMessage"));
            return;
        }
        if (modified && KMessageBox::questionTwoActions(
                    this,
                    i18n("<qt>The document has been changed since it was opened. To complete the decryption the document needs to be saved."
                         "<p>Do you want to save the document now?</qt>"),
                    i18n("Save Document"),
                    KStandardGuiItem::save(),
                    KStandardGuiItem::dontSave(),
                    QStringLiteral("DecryptSaveConfirmation")
                    ) != KMessageBox::PrimaryAction) {
            return;
        }
    } else if (doc->mimeType().startsWith("application/vnd.oasis.opendocument.")) {
        // Encrypt oasis document
        bool modified = doc->isModified();
        if (!doc->url().isEmpty() && doc->specialOutputFlag() == 0) {
            QMimeDatabase db;
            QMimeType mime = db.mimeTypeForName(QString::fromLatin1(doc->mimeType()));
            QString comment = mime.isValid() ? mime.comment() : i18n("%1 (unknown file type)", QString::fromLatin1(doc->mimeType()));
            if (KMessageBox::warningContinueCancel(
                        this,
                        i18n("<qt>The document is currently saved as %1. The document needs to be changed to <b>OASIS OpenDocument</b> to be encrypted."
                             "<p>Do you want to change the file to OASIS OpenDocument?</qt>", QStringLiteral("<b>%1</b>").arg(comment)),
                        i18n("Change Filetype"),
                        KGuiItem(i18n("Change")),
                        KStandardGuiItem::cancel(),
                        QStringLiteral("EncryptChangeFiletypeConfirmation")
                        ) != KMessageBox::Continue) {
                return;
            }
            doc->resetURL();
        }
        doc->setMimeType(doc->nativeOasisMimeType());
        doc->setOutputMimeType(doc->nativeOasisMimeType(), KoDocumentBase::SaveEncrypted);
        if (!mainWindow) {
            KMessageBox::information(
                        this,
                        i18n("<qt>Your document could not be saved automatically."
                             "<p>To complete the encryption, please save the document.</qt>"),
                        i18n("Save Document"),
                        QStringLiteral("EncryptSaveMessage"));
            return;
        }
        if (modified && KMessageBox::questionTwoActions(
                    this,
                    i18n("<qt>The document has been changed since it was opened. To complete the encryption the document needs to be saved."
                         "<p>Do you want to save the document now?</qt>"),
                    i18n("Save Document"),
                    KStandardGuiItem::save(),
                    KStandardGuiItem::dontSave(),
                    QStringLiteral("EncryptSaveConfirmation")
                    ) != KMessageBox::PrimaryAction) {
            return;
        }
    } else {
        // Encrypt non-oasis document
        bool modified = doc->isModified();
        saveas = doc->outputMimeType().isEmpty();
        if (!doc->url().isEmpty() && doc->specialOutputFlag() == 0) {
            QMimeDatabase db;
            QMimeType mime = db.mimeTypeForName(QString::fromLatin1(doc->mimeType()));
            if (!doc->mimeType().isEmpty() && doc->mimeType() != doc->nativeFormatMimeType()) {
                QString comment = mime.isValid() ? mime.comment() : i18n("%1 (unknown file type)", QString::fromLatin1(doc->mimeType()));
                QString native = db.mimeTypeForName(QString::fromLatin1(doc->nativeFormatMimeType())).comment();
                if (KMessageBox::warningContinueCancel(
                            this,
                            i18n("<qt>The document is currently saved as %1. The document needs to be changed to <b>%2</b> to be encrypted."
                                "<p>Do you want to change the file to %2?</qt>", QStringLiteral("<b>%1</b>").arg(comment), native),
                            i18n("Change Filetype"),
                            KGuiItem(i18n("Change")),
                            KStandardGuiItem::cancel(),
                            QStringLiteral("EncryptChangeFiletypeConfirmation")
                            ) != KMessageBox::Continue) {
                    return;
                }
                // Replace the current extension with our native one.
                // If the current extension is an unknown mimetype,
                // we just add our native extension. Maybe not ideal
                // but should not happen in normal cases,
                // and the user can always correct it in the save dialog.
                auto patterns = mime.globPatterns();
                auto url = doc->url().url();
                for (auto p : patterns) {
                    p.remove(QLatin1Char('*'));
                    if (url.endsWith(p)) {
                        url = url.left(url.lastIndexOf(p));
                    }
                }
                if (!url.isEmpty()) {
                    const auto nativePattern = db.mimeTypeForName(QString::fromLatin1(doc->nativeFormatMimeType())).globPatterns().value(0).remove(QLatin1Char('*'));
                    url.append(nativePattern);
                }
                doc->setUrl(QUrl(url));
                saveas = true;
                debugWidgets<<"document mimetype changed:"<<doc->url()<<patterns;
            }
        }
        doc->setMimeType(doc->nativeFormatMimeType());
        doc->setOutputMimeType(doc->nativeFormatMimeType(), KoDocumentBase::SaveEncrypted);
        if (!mainWindow) {
            KMessageBox::information(
                        this,
                        i18n("<qt>Your document could not be saved automatically."
                            "<p>To complete the encryption, please save the document.</qt>"),
                        i18n("Save Document"),
                        QStringLiteral("EncryptSaveMessage"));
            return;
        }
        if (modified && KMessageBox::questionTwoActions(
                    this,
                    i18n("<qt>The document has been changed since it was opened. To complete the encryption the document needs to be saved."
                        "<p>Do you want to save the document now?</qt>"),
                    i18n("Save Document"),
                    KStandardGuiItem::save(),
                    KStandardGuiItem::dontSave(),
                    QStringLiteral("EncryptSaveConfirmation")
                    ) != KMessageBox::PrimaryAction) {
            return;
        }
    }
    // Let KoMainWindow handle save
    Q_EMIT saveRequested(saveas, false, doc->specialOutputFlag());
    d->toggleEncryption = false;
    d->applyToggleEncryption = false;
    // Detects when the user cancelled saving of an unsaved document
    // FIXME: Should this be tracked in the document?
    d->documentSaved = !doc->url().scheme().isEmpty();
}

QList<KPageWidgetItem*> KoDocumentInfoDlg::pages() const
{
    return d->pages;
}

void KoDocumentInfoDlg::setReadOnly(bool ro)
{
    d->aboutUi->meComments->setReadOnly(ro);

    for (KPageWidgetItem* page : std::as_const(d->pages)) {
        const QList<QLineEdit*> lineEdits = page->widget()->findChildren<QLineEdit *>();
        for (QLineEdit* le : lineEdits) {
            le->setReadOnly(ro);
        }
        const QList<QPushButton*> buttons = page->widget()->findChildren<QPushButton *>();
        for (QPushButton* le : buttons ) {
            le->setDisabled(ro);
        }
    }
}

void KoDocumentInfoDlg::addPageItem(KoPageWidgetItem *item)
{
    KPageWidgetItem * page = new KoPageWidgetItemAdapter(item);

    addPage(page);
    d->pages.append(page);
}
#include "KoDocumentInfoDlg.moc"
