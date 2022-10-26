#ifndef TASKSPLITDIALOG_H
#define TASKSPLITDIALOG_H

#include <ui_TaskSplitDialog.h>

#include <kundo2command.h>

#include <QDialog>

namespace KPlato
{
class Project;
class Task;

class TaskSplitDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TaskSplitDialog(Project *project, Task *task, QWidget *parent = nullptr);


    KUndo2Command *buildCommand() const;

private:
    Project *m_project;
    Task *m_task;
    Ui::TaskSplitDialog ui;
};

}
#endif // TASKSPLITDIALOG_H
