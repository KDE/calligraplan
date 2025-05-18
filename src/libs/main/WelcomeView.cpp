/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "WelcomeView.h"
#include "config.h"
#include "ExtraProperties.h"

#include "KoApplication.h"
#include "KoPart.h"
#include "MimeTypes.h"
#include <KoIcon.h>
#include <KoApplication.h>
#include <KoMainWindow.h>
#include <KoDocument.h>
#include <KoFileDialog.h>
#include <KoComponentData.h>
#include <KoDocumentInfo.h>

#include <KConfigGroup>
#include <KRecentFilesAction>
#include <KUrlCompletion>

#include <QAction>
#include <QStringList>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QUrl>
#include <QIcon>
#include <QStandardPaths>
#include <QEvent>
#include <QLoggingCategory>
#include <QToolBar>

const QLoggingCategory &KOWELCOME_LOG()
{
    static const QLoggingCategory category("calligra.plan.welcome");
    return category;
}

#define debugWelcome qCDebug(KOWELCOME_LOG)<<Q_FUNC_INFO
#define warnWelcome qCWarning(KOWELCOME_LOG)
#define errorWelcome qCCritical(KOWELCOME_LOG)

class RecentProjectsModel : public QStandardItemModel
{
public:
    RecentProjectsModel(QObject *parent = nullptr);
    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    void setRecentFiles(const KRecentFilesAction &recent);
    void populate(const QList<QAction*> actions);
};

RecentProjectsModel::RecentProjectsModel(QObject *parent)
: QStandardItemModel(parent)
{
}

Qt::ItemFlags RecentProjectsModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags f = (QStandardItemModel::flags(idx) & ~Qt::ItemIsEditable);
    return f;
}

QVariant RecentProjectsModel::data(const QModelIndex &idx, int role) const
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

void RecentProjectsModel::setRecentFiles(const KRecentFilesAction &recent)
{
    populate(recent.actions());
}

void RecentProjectsModel::populate(const QList<QAction*> actions)
{
    clear();
    setColumnCount(1);
    setRowCount(actions.count());
    QModelIndex idx = index(rowCount()-1, 0);
    for (const QAction *a : actions) {
        // KRecentFilesAction format: <name> [<file path>]
        // so we split it up and remove the []
        QString s = a->text();
        QString name = s.left(s.indexOf(QLatin1Char('['))).trimmed();
        QString file = s.mid(s.indexOf(QLatin1Char('['))+1);
        file = file.left(file.lastIndexOf(QLatin1Char(']')));
        QString t = QStringLiteral("%1\n%2").arg(name, file);
        setData(idx, t, Qt::EditRole);
        setData(idx, file, Qt::UserRole+1);
        idx = idx.sibling(idx.row()-1, idx.column());
    }
}

//-----------------------------------
WelcomeView::WelcomeView(KoMainWindow *parent)
    : QWidget(parent)
    , m_filedialog(nullptr)
{
    ui.setupUi(this);

    ui.recentProjects->setBackgroundRole(QPalette::Midlight);
    ui.projectTemplates->setBackgroundRole(QPalette::Midlight);

    ui.contextHelp->setText(
        xi18nc("@info",
               "<subtitle>Context Help</subtitle>"
               "<para>"
               "Many functions have help and hints that can be displayed with <emphasis>What's this</emphasis>."
               "</para><para>"
               "Activate it by selecting the menu item <b><interface>Help|What's this</interface></b> or by pressing <shortcut>Shift+F1</shortcut>."
               "</para><para>"
               "Try it on this text!"
               "</para>"));

    ui.onlineLabel->setText(xi18nc("@info", "<subtitle>Online Resources</subtitle>"));

    ui.newProjectBtn->setWhatsThis(
                   xi18nc("@info",
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
                          "</para>", QStringLiteral("plan:creating-a-project#creating-a-project-section")));

    ui.createResourceFileBtn->setWhatsThis(
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
                          "</para>", QStringLiteral("plan:managing-resources")));

    ui.recentProjects->setWhatsThis(
                   xi18nc("@info:whatsthis",
                          "<title>Recent Projects</title>"
                          "<para>"
                          "A list of the 10 most recent project files opened."
                          "</para><para>"
                          "<nl/>This enables you to quickly open projects you have worked on recently."
                          "</para>"));

    ui.contextHelp->setWhatsThis(
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
                          " pressing it will display more information from the documentation."
                          "</para>", QStringLiteral("plan:context-help")));

    m_recentProjects = new RecentProjectsModel(this);
    ui.recentProjects->setModel(m_recentProjects);

    setProjectTemplatesModel();

    ui.projectTemplates->setWhatsThis(xi18nc("@info:whatsthis",
        "<title>Project Templates</title>"
        "<para>"
        "Select a template to create a new project based on the selected template."
        "</para><para>"
        "Plan searches for templates in the places you can define in the settings dialog: <interface>Settings->Configure Plan</interface>."
        "</para><para>"
        "You can create new templates from a project using the <interface>File->Create Project Template</interface> menu entry."
        "<nl/><link url='%1'>More...</link>"
        "</para>", QStringLiteral("plan:creating-a-project#creating-a-project-section")));

    setupGui();

    connect(ui.newProjectBtn, &QAbstractButton::clicked, this, &WelcomeView::slotNewProject);
    connect(ui.createResourceFileBtn, &QAbstractButton::clicked, this, &WelcomeView::slotCreateResourceFile);
    connect(ui.openProjectBtn, &QAbstractButton::clicked, this, &WelcomeView::slotOpenProject);

    connect(ui.recentProjects, &QAbstractItemView::activated, this, &WelcomeView::slotRecentFileSelected);

    connect(ui.projectTemplates, &QAbstractItemView::activated, this, &WelcomeView::slotOpenProjectTemplate);

    connect(mainWindow(), &KoMainWindow::loadCompleted, this, &WelcomeView::finished);

    parent->reloadRecentFileList();
    m_recentProjects->setRecentFiles(*parent->recentAction());
}

WelcomeView::~WelcomeView()
{
    debugWelcome;
}

KoMainWindow * WelcomeView::mainWindow() const
{
    return qobject_cast<KoMainWindow*>(parent());
}


void WelcomeView::slotRecentFileSelected(const QModelIndex &idx)
{
    if (idx.isValid()) {
        QString file = idx.data(Qt::UserRole+1).toString();
        KUrlCompletion k;
        QUrl url = QUrl::fromUserInput(k.replacedPath(file));
        debugWelcome<<file<<url;
        if (url.isValid()) {
            KoPart *part = this->part(QStringLiteral("calligraplan"), PLAN_MIME_TYPE);
            mainWindow()->openDocument(part, url);
        }
    }
}

void WelcomeView::slotContextMenuRequested(const QModelIndex &/*index*/, const QPoint& /*pos */)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
}

void WelcomeView::slotEnableActions(bool on)
{
    Q_UNUSED(on)
}

void WelcomeView::setupGui()
{
    // Add the context menu actions for the view options
}

void WelcomeView::slotNewProject()
{
    KoPart *part = this->part(QStringLiteral("calligraplan"), PLAN_MIME_TYPE);
    part->addMainWindow(mainWindow());
    if (!part->editProject()) {
        part->removeMainWindow(mainWindow());
        delete part;
    }
}

void WelcomeView::slotCreateResourceFile()
{
    QString file = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("templates/.source/SharedResources.plant"));
    KoPart *part = this->part(QStringLiteral("calligraplan"), PLAN_MIME_TYPE);
    part->addMainWindow(mainWindow());
    if (!part->openTemplate(QUrl::fromUserInput(file))) {
        part->removeMainWindow(mainWindow());
        delete part;
    }
}

void WelcomeView::slotOpenProject()
{
    KoFileDialog filedialog(this, KoFileDialog::OpenFile, QStringLiteral("OpenDocument"));
    filedialog.setCaption(i18n("Open Document"));
    filedialog.setDefaultDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    filedialog.setMimeTypeFilters(koApp->mimeFilter(KoFilterManager::Import));
    filedialog.setHideNameFilterDetailsOption();
    QUrl url = QUrl::fromUserInput(filedialog.filename());
    if (!url.isEmpty()) {
        KoPart *part = this->part(QStringLiteral("calligraplan"), PLAN_MIME_TYPE);
        part->addMainWindow(mainWindow());
        if (!mainWindow()->openDocument(part, url)) {
            part->removeMainWindow(mainWindow());
            delete part;
        }
    }
}

void WelcomeView::slotOpenProjectTemplate(const QModelIndex &idx)
{
    if (idx.isValid()) {
        QString file = idx.data(Qt::UserRole+1).toString();
        QUrl url = QUrl::fromUserInput(file);
        debugWelcome<<file<<url;
        KoPart *part = this->part(QStringLiteral("calligraplan"), PLAN_MIME_TYPE);
        bool ok = false;
        if (part && url.isValid()) {
            ok = part->openProjectTemplate(url);
        }
        if (ok) {
            part->addMainWindow(mainWindow());
            if (!part->editProject()) {
                part->removeMainWindow(mainWindow());
                delete part;
            }
        } else {
            warnWelcome<<"Failed to load template:"<<url;
            delete part;
        }
    }
}

void WelcomeView::slotLoadSharedResources(const QString &file, const QUrl &projects, bool loadProjectsAtStartup)
{
    QUrl url(file);
    if (url.scheme().isEmpty()) {
        url.setScheme(QStringLiteral("file"));
    }
    if (url.isValid()) {
        Q_EMIT loadSharedResources(url, loadProjectsAtStartup ? projects :QUrl());
    }
}

void WelcomeView::setProjectTemplatesModel()
{
    QStandardItemModel *m = new QStandardItemModel(ui.projectTemplates);
    KSharedConfigPtr configPtr = mainWindow()->componentData().config();
    const QStringList dirs = configPtr->group("Project Templates").readEntry("ProjectTemplatePaths", QStringList());
    bool addgroups = dirs.count() > 1;
    ui.projectTemplates->setRootIsDecorated(addgroups);
    for (const QString &path : dirs) {
        QStandardItem *parent = nullptr;
        if (addgroups) {
            QString p = path;
            if (p.endsWith(QLatin1Char('/'))) {
                p.remove(p.length()-1, 1);
            }
            p = p.mid(p.lastIndexOf(QLatin1Char('/'))+1);
            parent = new QStandardItem(p);
            m->appendRow(parent);
        }
        QDir dir(path, QStringLiteral("*.plant"));
        const auto entryList = dir.entryList(QDir::Files);
        for (const QString &file : entryList) {
            QStandardItem *item = new QStandardItem(file.left(file.lastIndexOf(QString::fromLatin1(".plant"))));
            item->setData(QString(path + QStringLiteral("/") + file));
            item->setIcon(koIcon("document-new-from-template"));
            setTemplateToolTip(item);
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

void WelcomeView::setTemplateToolTip(QStandardItem *item)
{
    QString filename = item->data().toString();
    item->setToolTip(filename);
    auto part = this->part(QApplication::applicationName(), PLAN_MIME_TYPE);
    if (part) {
        auto doc = part->createDocument(part);
        doc ->setCheckAutoSaveFile(false);
        doc->setProperty(NOUI, true);
        doc->setProperty(SKIPCOMPLETELOADING, true);
        doc->setProperty(SKIPLOADMAINDOC, true);
        if (doc->loadNativeFormat(item->data().toString())) {
            auto description = doc->documentInfo()->aboutInfo("description");
            if (!description.isEmpty()) {
                item->setToolTip(description);
            }
        }
    }
}

void WelcomeView::slotTemplateDocumentLoaded()
{

}

KoPart *WelcomeView::part(const QString &appName, const QString &mimeType) const
{
    return KoApplication::koApplication()->getPart(appName, mimeType);
}
