/*  This file is part of the KDE Libraries
 *  SPDX-FileCopyrightText: 1998 Thomas Tanghus (tanghus@earthling.net)
 *  Additions 1999-2000 by Espen Sand (espen@kde.org)
 *                      by Holger Freyther <freyther@kde.org>
 *            2005-2009 by Olivier Goffart (ogoffart at kde.org)
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "KoDialog.h"
#include "KoDialog_p.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QPointer>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QDebug>
#include <QPushButton>

#include <KConfig>
#include <KLocalizedString>

#include <KSeparator>
#include <KStandardGuiItem>
#include <KHelpClient>
#include <KUrlLabel>
#include <KWindowConfig>

void KoDialogPrivate::setupLayout()
{
    Q_Q(KoDialog);
    if (!dirty) {
        QMetaObject::invokeMethod(q, "queuedLayoutUpdate", Qt::QueuedConnection);
        dirty = true;
    }
}

void KoDialogPrivate::queuedLayoutUpdate()
{
    if (!dirty) {
        return;
    }

    dirty = false;

    Q_Q(KoDialog);

    // Don't lose the focus widget when re-creating the layout.
    // Testcase: KOrganizer's "Select Categories" dialog
    QPointer<QWidget> focusWidget = mMainWidget ? mMainWidget->focusWidget() : nullptr;

    if (q->layout() && q->layout() != mTopLayout) {
        qWarning() << q->metaObject()->className() << "created with a layout; don't do that, KoDialog takes care of it, use mainWidget or setMainWidget instead";
        delete q->layout();
    }

    delete mTopLayout;

    if (mButtonOrientation == Qt::Horizontal) {
        mTopLayout = new QVBoxLayout(q);
    } else {
        mTopLayout = new QHBoxLayout(q);
    }

    if (mUrlHelp) {
        mTopLayout->addWidget(mUrlHelp, 0, Qt::AlignRight);
    }

    if (mMainWidget) {
        mTopLayout->addWidget(mMainWidget, 10);
    }

    if (mDetailsWidget) {
        mTopLayout->addWidget(mDetailsWidget);
    }

    if (mActionSeparator) {
        mTopLayout->addWidget(mActionSeparator);
    }

    if (mButtonBox) {
        mButtonBox->setOrientation(mButtonOrientation);
        mTopLayout->addWidget(mButtonBox);
    }

    if (focusWidget) {
        focusWidget->setFocus();
    }
}

void KoDialogPrivate::appendButton(KoDialog::ButtonCode key, const KGuiItem &item)
{
    Q_Q(KoDialog);

    QDialogButtonBox::ButtonRole role = QDialogButtonBox::InvalidRole;
    switch (key) {
    case KoDialog::Help:
    case KoDialog::Details:
        role = QDialogButtonBox::HelpRole;
        break;
    case KoDialog::Default:
    case KoDialog::Reset:
        role = QDialogButtonBox::ResetRole;
        break;
    case KoDialog::Ok:
        role = QDialogButtonBox::AcceptRole;
        break;
    case KoDialog::Apply:
        role = QDialogButtonBox::ApplyRole;
        break;
    case KoDialog::Try:
    case KoDialog::Close:
    case KoDialog::Cancel:
        role = QDialogButtonBox::RejectRole;
        break;
    case KoDialog::User1:
    case KoDialog::User2:
    case KoDialog::User3:
        role = QDialogButtonBox::ActionRole;
        break;
    default:
        role = QDialogButtonBox::InvalidRole;
        break;
    }

    if (role == QDialogButtonBox::InvalidRole) {
        return;
    }

    QPushButton *button = new QPushButton;
    KGuiItem::assign(button, item);
    mButtonBox->addButton(button, role);

    mButtonList.insert(key, button);
    mButtonSignalMapper.setMapping(button, key);

    QObject::connect(button, SIGNAL(clicked()),
                     &mButtonSignalMapper, SLOT(map()));

    if (key == mDefaultButton) {
        // Now that it exists, set it as default
        q->setDefaultButton(mDefaultButton);
    }
}

void KoDialogPrivate::init(KoDialog *q)
{
    q_ptr = q;

    dirty = false;

    q->setButtons(KoDialog::Ok | KoDialog::Cancel);
    q->setDefaultButton(KoDialog::Ok);

    q->connect(&mButtonSignalMapper, SIGNAL(mappedInt(int)), q, SLOT(slotButtonClicked(int)));

    q->setPlainCaption(qApp->applicationDisplayName()); // set appropriate initial window title for case it gets not set later
}

void KoDialogPrivate::helpLinkClicked()
{
    q_ptr->slotButtonClicked(KoDialog::Help);
}

KoDialog::KoDialog(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , d_ptr(new KoDialogPrivate)
{
    d_ptr->init(this);
}

KoDialog::KoDialog(KoDialogPrivate &dd, QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , d_ptr(&dd)
{
    d_ptr->init(this);
}

KoDialog::~KoDialog()
{
    delete d_ptr;
}

void KoDialog::setButtons(ButtonCodes buttonMask)
{
    Q_D(KoDialog);
    if (d->mButtonBox) {
        d->mButtonList.clear();

        delete d->mButtonBox;
        d->mButtonBox = nullptr;
    }

    if (buttonMask & Cancel) {
        buttonMask &= ~Close;
    }

    if (buttonMask & Apply) {
        buttonMask &= ~Try;
    }

    if (buttonMask & Details) {
        buttonMask &= ~Default;
    }

    if (buttonMask == None) {
        d->setupLayout();
        return; // When we want no button box
    }

    d->mEscapeButton = (buttonMask & Cancel) ? Cancel : Close;
    d->mButtonBox = new QDialogButtonBox(this);

    if (buttonMask & Help) {
        d->appendButton(Help, KStandardGuiItem::help());
    }
    if (buttonMask & Default) {
        d->appendButton(Default, KStandardGuiItem::defaults());
    }
    if (buttonMask & Reset) {
        d->appendButton(Reset, KStandardGuiItem::reset());
    }
    if (buttonMask & User3) {
        d->appendButton(User3, KGuiItem());
    }
    if (buttonMask & User2) {
        d->appendButton(User2, KGuiItem());
    }
    if (buttonMask & User1) {
        d->appendButton(User1, KGuiItem());
    }
    if (buttonMask & Ok) {
        d->appendButton(Ok, KStandardGuiItem::ok());
    }
    if (buttonMask & Apply) {
        d->appendButton(Apply, KStandardGuiItem::apply());
    }
    if (buttonMask & Try) {
        d->appendButton(Try, KGuiItem(i18n("&Try")));
    }
    if (buttonMask & Cancel) {
        d->appendButton(Cancel, KStandardGuiItem::cancel());
    }
    if (buttonMask & Close) {
        d->appendButton(Close, KStandardGuiItem::close());
    }
    if (buttonMask & Details) {
        d->appendButton(Details, KGuiItem(QString(), QStringLiteral("help-about")));
        setDetailsWidgetVisible(false);
    }

    d->setupLayout();
}

void KoDialog::setButtonsOrientation(Qt::Orientation orientation)
{
    Q_D(KoDialog);
    if (d->mButtonOrientation != orientation) {
        d->mButtonOrientation = orientation;

        if (d->mActionSeparator) {
            d->mActionSeparator->setOrientation(d->mButtonOrientation);
        }

        if (d->mButtonOrientation == Qt::Vertical) {
            enableLinkedHelp(false);    // 2000-06-18 Espen: No support for this yet.
        }
    }
}

void KoDialog::setEscapeButton(ButtonCode id)
{
    d_func()->mEscapeButton = id;
}

void KoDialog::setDefaultButton(ButtonCode newDefaultButton)
{
    Q_D(KoDialog);

    if (newDefaultButton == None) {
        newDefaultButton = NoDefault;    // #148969
    }

    const KoDialog::ButtonCode oldDefault = defaultButton();

    bool oldDefaultHadFocus = false;

    if (oldDefault != NoDefault) {
        QPushButton *old = button(oldDefault);
        if (old) {
            oldDefaultHadFocus = (focusWidget() == old);
            old->setDefault(false);
        }
    }

    if (newDefaultButton != NoDefault) {
        QPushButton *b = button(newDefaultButton);
        if (b) {
            b->setDefault(true);
            if (focusWidget() == nullptr || oldDefaultHadFocus) {
                // No widget had focus yet, or the old default button had
                // -> ok to give focus to the new default button, so that it's
                // really default (Enter triggers it).
                // But we don't do this if the kdialog user gave focus to a
                // specific widget in the dialog.
                b->setFocus();
            }
        }
    }
    d->mDefaultButton = newDefaultButton;
    Q_ASSERT(defaultButton() == newDefaultButton);
}

KoDialog::ButtonCode KoDialog::defaultButton() const
{
    Q_D(const KoDialog);
    QHashIterator<int, QPushButton *> it(d->mButtonList);
    while (it.hasNext()) {
        it.next();
        if (it.value()->isDefault()) {
            return (ButtonCode)it.key();
        }
    }

    return d->mDefaultButton;
}

void KoDialog::setMainWidget(QWidget *widget)
{
    Q_D(KoDialog);
    if (d->mMainWidget == widget) {
        return;
    }
    d->mMainWidget = widget;
    if (d->mMainWidget && d->mMainWidget->layout()) {
        // Avoid double-margin problem
        d->mMainWidget->layout()->setContentsMargins(0, 0, 0, 0);
    }
    d->setupLayout();
}

QWidget *KoDialog::mainWidget()
{
    Q_D(KoDialog);
    if (!d->mMainWidget) {
        setMainWidget(new QWidget(this));
    }
    return d->mMainWidget;
}

QSize KoDialog::sizeHint() const
{
    Q_D(const KoDialog);

    if (!d->mMinSize.isEmpty()) {
        return d->mMinSize.expandedTo(minimumSizeHint()) + d->mIncSize;
    } else {
        if (d->dirty) {
            const_cast<KoDialogPrivate *>(d)->queuedLayoutUpdate();
        }
        return QDialog::sizeHint() + d->mIncSize;
    }
}

QSize KoDialog::minimumSizeHint() const
{
    Q_D(const KoDialog);

    if (d->dirty) {
        const_cast<KoDialogPrivate *>(d)->queuedLayoutUpdate();
    }
    return QDialog::minimumSizeHint() + d->mIncSize;
}

//
// Grab QDialogs keypresses if non-modal.
//
void KoDialog::keyPressEvent(QKeyEvent *event)
{
    Q_D(KoDialog);
    if (event->modifiers() == 0) {
        if (event->key() == Qt::Key_F1) {
            QPushButton *button = this->button(Help);

            if (button) {
                button->animateClick();
                event->accept();
                return;
            }
        }

        if (event->key() == Qt::Key_Escape) {
            QPushButton *button = this->button(d->mEscapeButton);

            if (button) {
                button->animateClick();
                event->accept();
                return;
            }

        }
    } else if (event->key() == Qt::Key_F1 && event->modifiers() == Qt::ShiftModifier) {
        QWhatsThis::enterWhatsThisMode();
        event->accept();
        return;
    } else if (event->modifiers() == Qt::ControlModifier &&
               (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
        // accept the dialog when Ctrl-Return is pressed
        QPushButton *button = this->button(Ok);

        if (button) {
            button->animateClick();
            event->accept();
            return;
        }
    }

    QDialog::keyPressEvent(event);
}

QString KoDialog::makeStandardCaption(const QString &userCaption,
                                     QWidget *window,
                                     CaptionFlags flags)
{
    Q_UNUSED(window);
    QString caption = qApp->applicationDisplayName();
    QString captionString = userCaption.isEmpty() ? caption : userCaption;

    // If the document is modified, add '[modified]'.
    if (flags & ModifiedCaption) {
        captionString += QString::fromUtf8(" [") + i18n("modified") + QString::fromUtf8("]");
    }

    if (!userCaption.isEmpty()) {
        // Add the application name if:
        // User asked for it, it's not a duplication  and the app name (caption()) is not empty
        if (flags & AppNameCaption &&
                !caption.isEmpty() &&
                !userCaption.endsWith(caption)) {
            // TODO: check to see if this is a transient/secondary window before trying to add the app name
            //       on platforms that need this
            captionString += i18nc("Document/application separator in titlebar", " – ") + caption;
        }
    }

    return captionString;
}

void KoDialog::setCaption(const QString &_caption)
{
    const QString caption = makeStandardCaption(_caption, this);
    setPlainCaption(caption);
}

void KoDialog::setCaption(const QString &caption, bool modified)
{
    CaptionFlags flags = HIGCompliantCaption;

    // ### Qt5 TODO: port to [*], see QWidget::setWindowFilePath
    if (modified) {
        flags |= ModifiedCaption;
    }

    setPlainCaption(makeStandardCaption(caption, this, flags));
}

void KoDialog::setPlainCaption(const QString &caption)
{
    if (QWidget *win = window()) {
        win->setWindowTitle(caption);
    }
}

void KoDialog::resizeLayout(QWidget *widget, int margin, int spacing)   //static
{
    if (widget->layout()) {
        resizeLayout(widget->layout(), margin, spacing);
    }

    if (widget->children().count() > 0) {
        const QList<QObject *> list = widget->children();
        for (QObject *object : list) {
            if (object->isWidgetType()) {
                resizeLayout((QWidget *)object, margin, spacing);
            }
        }
    }
}

void KoDialog::resizeLayout(QLayout *layout, int margin, int spacing)   //static
{
    QLayoutItem *child;
    int pos = 0;

    while ((child = layout->itemAt(pos))) {
        if (child->layout()) {
            resizeLayout(child->layout(), margin, spacing);
        }

        ++pos;
    }

    if (layout->layout()) {
        layout->layout()->setContentsMargins(margin, margin, margin, margin);
        layout->layout()->setSpacing(spacing);
    }
}

void KoDialog::showButtonSeparator(bool state)
{
    Q_D(KoDialog);
    if ((d->mActionSeparator != nullptr) == state) {
        return;
    }
    if (state) {
        if (d->mActionSeparator) {
            return;
        }

        d->mActionSeparator = new KSeparator(this);
        d->mActionSeparator->setOrientation(d->mButtonOrientation);
    } else {
        delete d->mActionSeparator;
        d->mActionSeparator = nullptr;
    }

    d->setupLayout();
}

void KoDialog::setInitialSize(const QSize &size)
{
    d_func()->mMinSize = size;
    adjustSize();
}

void KoDialog::incrementInitialSize(const QSize &size)
{
    d_func()->mIncSize = size;
    adjustSize();
}

QPushButton *KoDialog::button(ButtonCode id) const
{
    Q_D(const KoDialog);
    return d->mButtonList.value(id, nullptr);
}

void KoDialog::enableButton(ButtonCode id, bool state)
{
    QPushButton *button = this->button(id);
    if (button) {
        button->setEnabled(state);
    }
}

bool KoDialog::isButtonEnabled(ButtonCode id) const
{
    QPushButton *button = this->button(id);
    if (button) {
        return button->isEnabled();
    }

    return false;
}

void KoDialog::enableButtonOk(bool state)
{
    enableButton(Ok, state);
}

void KoDialog::enableButtonApply(bool state)
{
    enableButton(Apply, state);
}

void KoDialog::enableButtonCancel(bool state)
{
    enableButton(Cancel, state);
}

void KoDialog::showButton(ButtonCode id, bool state)
{
    QPushButton *button = this->button(id);
    if (button) {
        state ? button->show() : button->hide();
    }
}

void KoDialog::setButtonGuiItem(ButtonCode id, const KGuiItem &item)
{
    QPushButton *button = this->button(id);
    if (!button) {
        return;
    }

    KGuiItem::assign(button, item);
}

void KoDialog::setButtonText(ButtonCode id, const QString &text)
{
    Q_D(KoDialog);
    if (!d->mSettingDetails && (id == Details)) {
        d->mDetailsButtonText = text;
        setDetailsWidgetVisible(d->mDetailsVisible);
        return;
    }

    QPushButton *button = this->button(id);
    if (button) {
        button->setText(text);
    }
}

QString KoDialog::buttonText(ButtonCode id) const
{
    QPushButton *button = this->button(id);
    if (button) {
        return button->text();
    } else {
        return QString();
    }
}

void KoDialog::setButtonIcon(ButtonCode id, const QIcon &icon)
{
    QPushButton *button = this->button(id);
    if (button) {
        button->setIcon(icon);
    }
}

QIcon KoDialog::buttonIcon(ButtonCode id) const
{
    QPushButton *button = this->button(id);
    if (button) {
        return button->icon();
    } else {
        return QIcon();
    }
}

void KoDialog::setButtonToolTip(ButtonCode id, const QString &text)
{
    QPushButton *button = this->button(id);
    if (button) {
        if (text.isEmpty()) {
            button->setToolTip(QString());
        } else {
            button->setToolTip(text);
        }
    }
}

QString KoDialog::buttonToolTip(ButtonCode id) const
{
    QPushButton *button = this->button(id);
    if (button) {
        return button->toolTip();
    } else {
        return QString();
    }
}

void KoDialog::setButtonWhatsThis(ButtonCode id, const QString &text)
{
    QPushButton *button = this->button(id);
    if (button) {
        if (text.isEmpty()) {
            button->setWhatsThis(QString());
        } else {
            button->setWhatsThis(text);
        }
    }
}

QString KoDialog::buttonWhatsThis(ButtonCode id) const
{
    QPushButton *button = this->button(id);
    if (button) {
        return button->whatsThis();
    } else {
        return QString();
    }
}

void KoDialog::setButtonFocus(ButtonCode id)
{
    QPushButton *button = this->button(id);
    if (button) {
        button->setFocus();
    }
}

void KoDialog::setDetailsWidget(QWidget *detailsWidget)
{
    Q_D(KoDialog);
    if (d->mDetailsWidget == detailsWidget) {
        return;
    }
    delete d->mDetailsWidget;
    d->mDetailsWidget = detailsWidget;

    if (d->mDetailsWidget->parentWidget() != this) {
        d->mDetailsWidget->setParent(this);
    }

    d->mDetailsWidget->hide();
    d->setupLayout();

    if (!d->mSettingDetails) {
        setDetailsWidgetVisible(d->mDetailsVisible);
    }
}

bool KoDialog::isDetailsWidgetVisible() const
{
    return d_func()->mDetailsVisible;
}

void KoDialog::setDetailsWidgetVisible(bool visible)
{
    Q_D(KoDialog);
    if (d->mDetailsButtonText.isEmpty()) {
        d->mDetailsButtonText = i18n("&Details");
    }

    d->mSettingDetails = true;
    d->mDetailsVisible = visible;
    if (d->mDetailsVisible) {
        Q_EMIT aboutToShowDetails();
        setButtonText(Details, d->mDetailsButtonText + QStringLiteral(" <<"));
        if (d->mDetailsWidget) {
            if (layout()) {
                layout()->setEnabled(false);
            }

            d->mDetailsWidget->show();

            adjustSize();

            if (layout()) {
                layout()->activate();
                layout()->setEnabled(true);
            }
        }
    } else {
        setButtonText(Details, d->mDetailsButtonText + QStringLiteral(" >>"));
        if (d->mDetailsWidget) {
            d->mDetailsWidget->hide();
        }

        if (layout()) {
            layout()->activate();
            adjustSize();
        }

    }

    d->mSettingDetails = false;
}

void KoDialog::delayedDestruct()
{
    if (isVisible()) {
        hide();
    }

    deleteLater();
}

void KoDialog::slotButtonClicked(int button)
{
    Q_D(KoDialog);
    Q_EMIT buttonClicked(static_cast<KoDialog::ButtonCode>(button));

    switch (button) {
    case Ok:
        Q_EMIT okClicked();
        accept();
        break;
    case Apply:
        Q_EMIT applyClicked();
        break;
    case Try:
        Q_EMIT tryClicked();
        break;
    case User3:
        Q_EMIT user3Clicked();
        break;
    case User2:
        Q_EMIT user2Clicked();
        break;
    case User1:
        Q_EMIT user1Clicked();
        break;
    case Cancel:
        Q_EMIT cancelClicked();
        reject();
        break;
    case Close:
        Q_EMIT closeClicked();
        done(Close); // KDE5: call reject() instead; more QDialog-like.
        break;
    case Help:
        Q_EMIT helpClicked();
        if (!d->mAnchor.isEmpty() || !d->mHelpApp.isEmpty()) {
            KHelpClient::invokeHelp(d->mAnchor, d->mHelpApp);
        }
        break;
    case Default:
        Q_EMIT defaultClicked();
        break;
    case Reset:
        Q_EMIT resetClicked();
        break;
    case Details:
        setDetailsWidgetVisible(!d->mDetailsVisible);
        break;
    }

    // If we're here from the closeEvent, and auto-delete is on, well, auto-delete now.
    if (d->mDeferredDelete) {
        d->mDeferredDelete = false;
        delayedDestruct();
    }
}

void KoDialog::enableLinkedHelp(bool state)
{
    Q_D(KoDialog);
    if ((d->mUrlHelp != nullptr) == state) {
        return;
    }
    if (state) {
        if (d->mUrlHelp) {
            return;
        }

        d->mUrlHelp = new KUrlLabel(this);
        d->mUrlHelp->setText(helpLinkText());
        d->mUrlHelp->setFloatEnabled(true);
        d->mUrlHelp->setUnderline(true);
        d->mUrlHelp->setMinimumHeight(fontMetrics().height() + QApplication::style()->pixelMetric(QStyle::PM_LayoutTopMargin));
        connect(d->mUrlHelp, SIGNAL(leftClickedUrl()), SLOT(helpLinkClicked()));

        d->mUrlHelp->show();
    } else {
        delete d->mUrlHelp;
        d->mUrlHelp = nullptr;
    }

    d->setupLayout();
}

void KoDialog::setHelp(const QString &anchor, const QString &appname)
{
    Q_D(KoDialog);
    d->mAnchor  = anchor;
    d->mHelpApp = appname;
}

void KoDialog::setHelpLinkText(const QString &text)
{
    Q_D(KoDialog);
    d->mHelpLinkText = text;
    if (d->mUrlHelp) {
        d->mUrlHelp->setText(helpLinkText());
    }
}

QString KoDialog::helpLinkText() const
{
    Q_D(const KoDialog);
    return (d->mHelpLinkText.isEmpty() ? i18n("Get help...") : d->mHelpLinkText);
}

void KoDialog::updateGeometry()
{
}

void KoDialog::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    Q_EMIT hidden();
}

void KoDialog::closeEvent(QCloseEvent *event)
{
    Q_D(KoDialog);
    QPushButton *button = this->button(d->mEscapeButton);
    if (button && !isHidden()) {
        button->animateClick();

        if (testAttribute(Qt::WA_DeleteOnClose)) {
            // Don't let QWidget::close do a deferred delete just yet, wait for the click first
            d->mDeferredDelete = true;
            setAttribute(Qt::WA_DeleteOnClose, false);
        }
    } else {
        QDialog::closeEvent(event);
    }
}

#include "moc_KoDialog.cpp"
