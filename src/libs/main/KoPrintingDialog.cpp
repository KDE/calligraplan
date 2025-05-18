/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007, 2009 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2011 Boudewijn Rempt <boud@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
// clazy:excludeall=qstring-arg
#include "KoPrintingDialog.h"
#include "KoPrintingDialog_p.h"
#include "KoProgressUpdater.h"

//#include <KoZoomHandler.h>
//#include <KoShapeManager.h>
//#include <KoShape.h>
#include <KoProgressBar.h>

#include <QApplication>
#include <MainDebug.h>
#include <KLocalizedString>
#include <QPainter>
#include <QPrinter>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QDialog>
#include <QThread>

class PrintDialog : public QDialog {
public:
    PrintDialog(KoPrintingDialogPrivate *d, QWidget *parent)
        : QDialog(parent)
    {
        setModal(true);
        QGridLayout *grid = new QGridLayout(this);
        setLayout(grid);

        d->pageNumber = new QLabel(this);
        d->pageNumber->setMinimumWidth(200);
        grid->addWidget(d->pageNumber, 0, 0, 1, 2);
        KoProgressBar *bar = new KoProgressBar(this);
        d->progress = new KoProgressUpdater(bar);
        grid->addWidget(bar, 1, 0, 1, 2);
        d->button = new QPushButton(i18n("Stop"), this);
        grid->addWidget(d->button, 2, 1);
        grid->setColumnStretch(0, 1);
    }
};


KoPrintingDialog::KoPrintingDialog(QWidget *parent, QPrinter::PrinterMode mode)
    : KoPrintJob(parent),
      d(new KoPrintingDialogPrivate(this, mode))
{
    d->dialog = new PrintDialog(d, parent);

    connect(d->button, SIGNAL(released()), this, SLOT(stopPressed())); // clazy:exclude=old-style-connect
}

KoPrintingDialog::~KoPrintingDialog()
{
    d->stopPressed();
    delete d;
}

/*
void KoPrintingDialog::setShapeManager(KoShapeManager *sm)
{
    d->shapeManager = sm;
}

KoShapeManager *KoPrintingDialog::shapeManager() const
{
    return d->shapeManager;
}
*/
void KoPrintingDialog::setPageRange(const QList<int> &pages)
{
    if (d->index == 0) // can't change after we started
        d->pageRange = pages;
}

QPainter & KoPrintingDialog::painter() const
{
    if (d->painter == nullptr) {
        d->painter = new QPainter(d->printer);
        d->painter->save(); // state before page preparation (3)
    }
    return *d->painter;
}

bool KoPrintingDialog::isStopped() const
{
    return d->stop;
}

// Define a printer fri'\n'y palette
#define VeryLightGray   "#f8f8f8"
#define LightLightGray  "#f0f0f0"
#define DarkDarkGray    "#b3b3b3"
#define VeryDarkGray    "#838383"
class PrintPalette {
public:
    PrintPalette() {
        orig = QApplication::palette();
        QPalette palette = orig;
        // define a palette that works when printing on white paper
        palette.setColor(QPalette::Window, Qt::white);
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::AlternateBase, VeryLightGray); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, Qt::lightGray);
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::BrightText, Qt::white);
        palette.setColor(QPalette::Link, Qt::blue);
        palette.setColor(QPalette::Highlight, Qt::blue);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Light, QColor(VeryLightGray)); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::Midlight, QColor(LightLightGray)); // used for freeDays in gantt chart // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::Dark, QColor(DarkDarkGray)); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::Mid, QColor(VeryDarkGray)); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::Shadow, Qt::black);
        QApplication::setPalette(palette);
    }
    ~PrintPalette() {
        QApplication::setPalette(orig);
    }
    QPalette orig;
};


void KoPrintingDialog::startPrinting(RemovePolicy removePolicy)
{
    d->removePolicy = removePolicy;
    d->pages = d->pageRange;
    if (d->pages.isEmpty()) { // auto-fill from min/max
        switch (d->printer->printRange()) {
        case QPrinter::AllPages:
            for (int i=documentFirstPage(); i <= documentLastPage(); i++)
                d->pages.append(i);
            break;
        case QPrinter::PageRange:
            for (int i=d->printer->fromPage(); i <= d->printer->toPage(); i++)
                d->pages.append(i);
            break;
        case QPrinter::CurrentPage:
            d->pages.append(documentCurrentPage());
            break;
        default:
            return;
        }
    }
    if (d->pages.isEmpty()) {
        qWarning(/*30004*/) << "KoPrintingDialog::startPrinting: No pages to print, did you forget to call setPageRange()?";
        return;
    }

    const bool blocking = property("blocking").toBool();
    const bool noprogressdialog = property("noprogressdialog").toBool();
    if (d->index == 0 && d->pages.count() > 0 && d->printer) {
        if (!blocking && !noprogressdialog)
            d->dialog->show();
        d->stop = false;
        delete d->painter;
        d->painter = nullptr;
//        d->zoomer.setZoom(1.0);
//        d->zoomer.setDpi(d->printer->resolution(), d->printer->resolution());

        if (blocking) {
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        }
        d->progress->start(100, i18n("Printing"));

        if (d->printer->copyCount() > 1) {
            const QList<int> oldPages = d->pages;
            if (d->printer->collateCopies()) { // means we print whole doc at once
                for (int count = 1; count < d->printer->copyCount(); ++count)
                    d->pages.append(oldPages);
            } else {
                d->pages.clear();
                for (int page : oldPages) {
                    for (int count = 1; count < d->printer->copyCount(); ++count)
                        d->pages.append(page);
                }
            }
        }
        if (d->printer->pageOrder() == QPrinter::LastPageFirst) {
            const QList<int> pages = d->pages;
            d->pages.clear();
            QList<int>::ConstIterator iter = pages.end();
            do {
                --iter;
                d->pages << *iter;
            } while (iter != pages.begin());
        }

        PrintPalette p;

        d->resetValues();
        for (int page : std::as_const(d->pages)) {
            d->index++;
            d->updaters.append(d->progress->startSubtask()); // one per page
            d->preparePage(page);
            d->printPage(page);
            if (!blocking) {
                qApp->processEvents();
            }
            
        }
        if (blocking) {
            printingDone();
        }
        else {
            d->printingDone();
        }
        d->stop = true;
        d->resetValues();
        if (blocking) {
            QGuiApplication::restoreOverrideCursor();
        }
    }
}

QPrinter &KoPrintingDialog::printer()
{
    return *d->printer;
}

void KoPrintingDialog::printPage(int, QPainter &)
{
}

QRectF KoPrintingDialog::preparePage(int)
{
    return QRectF();
}

// have to include this because of Q_PRIVATE_SLOT
#include <moc_KoPrintingDialog.cpp>
