/* This file is part of the KDE project
  Copyright (C) 2017 Dag Andersen <danders@get2net.dk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

// clazy:excludeall=qstring-arg
#include "WelcomeView.h"

#include <kptconfigbase.h>
#include "kptcommand.h"
#include "kptdebug.h"
#include "Help.h"
#include "kptpart.h"
#include "kptmaindocument.h"

#include <KoIcon.h>
#include <KoApplication.h>
#include <KoMainWindow.h>
#include <KoDocument.h>
#include <KoFileDialog.h>

#include <QAction>
#include <QStringList>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QUrl>
#include <QIcon>
#include <QStandardPaths>
#include <QEvent>

const QLoggingCategory &PLANWELCOME_LOG()
{
    static const QLoggingCategory category("calligra.plan.welcome");
    return category;
}

#define debugWelcome qCDebug(PLANWELCOME_LOG)<<Q_FUNC_INFO
#define warnWelcome qCWarning(PLANWELCOME_LOG)
#define errorWelcome qCCritical(PLANWELCOME_LOG)

namespace KPlato {
class RecentFilesModel : public QStandardItemModel
{
public:
    RecentFilesModel(QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    void populate(const QList<QAction*> actions);
};
}

using namespace KPlato;

RecentFilesModel::RecentFilesModel(QObject *parent)
: QStandardItemModel(parent)
{
}

Qt::ItemFlags RecentFilesModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags f = (QStandardItemModel::flags(idx) & ~Qt::ItemIsEditable);
    return f;
}

QVariant RecentFilesModel::data(const QModelIndex &idx, int role) const
{
    switch(role) {
        case Qt::DecorationRole:
            return QIcon::fromTheme(QStringLiteral("document-open"));
        case Qt::FontRole: {
            break;
        }
        default: break;
    }
    return QStandardItemModel::data(idx, role);
}

void RecentFilesModel::populate(const QList<QAction*> actions)
{
    clear();
    setColumnCount(1);
    setRowCount(actions.count());
    QModelIndex idx = index(rowCount()-1, 0);
    for (const QAction *a : actions) {
        // KRecentFilesAction format: <name> [<file path>]
        // so we split it up and remove the []
        QString s = a->text();
        QString name = s.left(s.indexOf('['));
        QString file = s.mid(s.indexOf('[')+1);
        file = file.left(file.lastIndexOf(']'));
        QString t = QString("%1\n%2").arg(name, file);
        setData(idx, t, Qt::EditRole);
        setData(idx, file, Qt::UserRole+1);
        idx = idx.sibling(idx.row()-1, idx.column());
    }
}

//-----------------------------------
WelcomeView::WelcomeView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
    , m_projectdialog(0)
    , m_filedialog(0)
{
    ui.setupUi(this);
    ui.recentProjects->setBackgroundRole(QPalette::Midlight);
    ui.projectTemplates->setBackgroundRole(QPalette::Midlight);

    Help::add(ui.newProjectBtn,
                   xi18nc("@info:whatsthis",
                          "<title>Create a new project</title>"
                          "<para>"
                          "Creates a new project with default values defined in"
                          " <emphasis>Settings</emphasis>."
                          "<nl/>Opens the <emphasis>project dialog</emphasis>"
                          " so you can define project specific properties like"
                          " <resource>Project Name</resource>,"
                          " <resource>Target Start</resource>"
                          " and <resource>- End</resource> times."
                          "<nl/><link url='%1'>More...</link>"
                          "</para>", Help::page("Creating_a_Project")));

    Help::add(ui.createResourceFileBtn,
                   xi18nc("@info:whatsthis",
                          "<title>Shared resources</title>"
                          "<para>"
                          "Create a shared resources file."
                          "<nl/>This enables you to only create your resources once,"
                          " you just refer to your resources file when you create a new project."
                          "<nl/>These resources can then be shared between projects"
                          " to avoid overbooking resources across projects."
                          "<nl/>Shared resources must be defined in a separate file."
                          "<nl/><link url='%1'>More...</link>"
                          "</para>", Help::page("Managing_Resources")));

    Help::add(ui.recentProjects,
                   xi18nc("@info:whatsthis",
                          "<title>Recent Projects</title>"
                          "<para>"
                          "A list of the 10 most recent project files opened."
                          "</para><para>"
                          "<nl/>This enables you to quickly open projects you have worked on recently."
                          "</para>"));

    Help::add(ui.contextHelp,
                   xi18nc("@info:whatsthis",
                          "<title>Context help</title>"
                          "<para>"
                          "Help is available many places using <emphasis>What's This</emphasis>."
                          "<nl/>It is activated using the menu entry <interface>Help->What's this?</interface>"
                          " or the keyboard shortcut <shortcut>Shift+F1</shortcut>."
                          "</para><para>"
                          "In dialogs it is available via the <interface>?</interface> in the dialog title bar."
                          "</para><para>"
                          "If you see <link url='%1'>More...</link> in the text,"
                          " pressing it will display more information from online resources in your browser."
                          "</para>", Help::page("Context_Help")));

    m_model = new RecentFilesModel(this);
    ui.recentProjects->setModel(m_model);
    ui.recentProjects->setSelectionMode(QAbstractItemView::SingleSelection);

    setProjectTemplatesModel();
    Help::add(ui.projectTemplates, xi18nc("@info:whatsthis",
        "<title>Project Templates</title>"
        "<para>"
        "Select a template to create a new project based on the selected template."
        "</para><para>"
        "Plan searches for templates in the places you can define in the settings dialog: <interface>Settings->Configure Plan</interface>."
        "</para><para>"
        "You can create new templates from a project using the <interface>File->Create Project Template</interface> menu entry."
        "<nl/><link url='%1'>More...</link>"
        "</para>", Help::page("Creating_a_Project")));

    setupGui();

    connect(ui.newProjectBtn, &QAbstractButton::clicked, this, &WelcomeView::slotNewProject);
    connect(ui.createResourceFileBtn, &QAbstractButton::clicked, this, &WelcomeView::slotCreateResourceFile);
    connect(ui.openProjectBtn, &QAbstractButton::clicked, this, &WelcomeView::slotOpenProject);

    connect(ui.recentProjects, &QAbstractItemView::activated, this, &WelcomeView::slotRecentFileSelected);

    connect(ui.projectTemplates, &QAbstractItemView::activated, this, &WelcomeView::slotOpenProjectTemplate);

    connect(mainWindow(), &KoMainWindow::loadCompleted, this, &WelcomeView::finished);
}

WelcomeView::~WelcomeView()
{
    debugWelcome;
}

Project *WelcomeView::project() const
{
    return koDocument() ? koDocument()->project() : nullptr;
}

void WelcomeView::setRecentFiles(const QList<QAction*> &actions)
{
    m_model->populate(actions);
}

void WelcomeView::updateReadWrite(bool /*readwrite */)
{
}

void WelcomeView::setGuiActive(bool activate)
{
    debugPlan<<activate;
}

void WelcomeView::slotRecentFileSelected(const QModelIndex &idx)
{
    if (idx.isValid()) {
        QString file = idx.data(Qt::UserRole+1).toString();
        int start = file.indexOf('[');
        int end = file.indexOf(']');
        file = file.left(end).mid(start+1);
        QUrl url = QUrl::fromUserInput(file);
        debugWelcome<<file<<url;
        KoPart *part = koDocument()->isEmpty() ? koDocument()->documentPart() : nullptr;
        if (url.isValid()) {
            emit recentProject(url, part);
        }
    }
}

void WelcomeView::slotContextMenuRequested(const QModelIndex &/*index*/, const QPoint& /*pos */)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
}

void WelcomeView::slotEnableActions(bool on)
{
    updateActionsEnabled(on);
}

void WelcomeView::updateActionsEnabled(bool /*on */)
{
}

void WelcomeView::setupGui()
{
    // Add the context menu actions for the view options
}

KoPrintJob *WelcomeView::createPrintJob()
{
    return 0;
}

void WelcomeView::slotNewProject()
{
    Project *p = project();
    if (p) {
        p->config().setDefaultValues(*p);
        if (!m_projectdialog) {
            m_projectdialog =  new MainProjectDialog(*p, this, false /*edit*/);
            connect(m_projectdialog.data(), &MainProjectDialog::dialogFinished, this, &WelcomeView::slotProjectEditFinished);
            connect(m_projectdialog.data(), &MainProjectDialog::sigLoadSharedResources, this, &WelcomeView::slotLoadSharedResources);
        }
        m_projectdialog->open();
    }
}

void WelcomeView::slotProjectEditFinished(int result)
{
    MainProjectDialog *dia = qobject_cast<MainProjectDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        MacroCommand *cmd = dia->buildCommand();
        if (cmd) {
            cmd->execute();
            delete cmd;
            koDocument()->setModified(true);
        }
        emit projectCreated();
        emit selectDefaultView();
        emit finished();
    }
    dia->deleteLater();
}

void WelcomeView::slotCreateResourceFile()
{
    QString file = QStandardPaths::locate(QStandardPaths::AppDataLocation, "templates/.source/SharedResources.plant");
    emit openTemplate(QUrl::fromUserInput(file));
    emit finished();
}

void WelcomeView::slotOpenProject()
{
    Project *p = project();
    if (p) {
        KoFileDialog filedialog(this, KoFileDialog::OpenFile, "OpenDocument");
        filedialog.setCaption(i18n("Open Document"));
        filedialog.setDefaultDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        filedialog.setMimeTypeFilters(koApp->mimeFilter(KoFilterManager::Import));
        filedialog.setHideNameFilterDetailsOption();
        KoPart *part = koDocument()->isEmpty() ? koDocument()->documentPart() : nullptr;
        QUrl url = QUrl::fromUserInput(filedialog.filename());
        if (!url.isEmpty()) {
            mainWindow()->openDocument(part, url);
        }
    }
}

void WelcomeView::slotOpenProjectTemplate(const QModelIndex &idx)
{
    if (idx.isValid()) {
        QString file = idx.data(Qt::UserRole+1).toString();
        QUrl url = QUrl::fromUserInput(file);
        debugWelcome<<file<<url<<koDocument();
        Q_ASSERT(koDocument());
        KoPart *part = koDocument()->documentPart();
        bool ok = false;
        if (part && url.isValid()) {
            ok = part->openProjectTemplate(url);
        }
        if (ok) {
            Project *p = part->document()->project();
            Q_ASSERT(p);
            MainProjectDialog dlg(*p, nullptr, false /*edit*/);
            if (dlg.exec()) {
                MacroCommand *cmd = dlg.buildCommand();
                if (cmd) {
                    cmd->execute();
                    delete cmd;
                    part->document()->setModified(true);
                    if (part->document()->url().isEmpty() && !p->name().isEmpty()) {
                        part->document()->setUrl(QUrl(p->name() + ".plan"));
                    }
                    emit finished();
                }
            } else {
                warnWelcome<<"cancelled dialog";
                koDocument()->initEmpty();
            }
        } else {
            warnWelcome<<"Failed to load template:"<<url;
        }
    }
}

void WelcomeView::slotOpenFileFinished(int result)
{
    KoFileDialog *dia = qobject_cast<KoFileDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        QUrl url = QUrl::fromUserInput(dia->filename());
        KoPart *part = koDocument()->isEmpty() ? nullptr : koDocument()->documentPart();
        if (!url.isEmpty() && mainWindow()->openDocument(part, url)) {
            emit finished();
        }
    }
    dia->deleteLater();
}

void WelcomeView::slotLoadSharedResources(const QString &file, const QUrl &projects, bool loadProjectsAtStartup)
{
    QUrl url(file);
    if (url.scheme().isEmpty()) {
        url.setScheme("file");
    }
    if (url.isValid()) {
        emit loadSharedResources(url, loadProjectsAtStartup ? projects :QUrl());
    }
}

void WelcomeView::setProjectTemplatesModel()
{
    QStandardItemModel *m = new QStandardItemModel(ui.projectTemplates);
    const ConfigBase &config = koDocument()->project()->config();
    const QStringList dirs = config.projectTemplatePaths();
    bool addgroups = dirs.count() > 1;
    ui.projectTemplates->setRootIsDecorated(addgroups);
    for (const QString &path : dirs) {
        QStandardItem *parent = nullptr;
        if (addgroups) {
            QString p = path;
            if (p.endsWith('/')) {
                p.remove(p.length()-1, 1);
            }
            p = p.mid(p.lastIndexOf('/')+1);
            parent = new QStandardItem(p);
            m->appendRow(parent);
        }
        QDir dir(path, "*.plant");
        for (const QString &file : dir.entryList(QDir::Files)) {
            QStandardItem *item = new QStandardItem(file.left(file.lastIndexOf(".plant")));
            item->setData(QString(path + '/' + file));
            item->setToolTip(item->data().toString());
            item->setIcon(koIcon("document-new-from-template"));
            if (parent) {
                parent->appendRow(item);
            } else {
                m->appendRow(item);
            }
        }
    }
    delete ui.projectTemplates->model();
    ui.projectTemplates->setModel(m);
    ui.projectTemplates->expandAll();
}
