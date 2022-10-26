#include "TaskSplitDialog.h"

#include <kptcommand.h>
#include <kptproject.h>
#include <kpttask.h>
#include <kptnode.h>
#include <kptrelation.h>
#include <kptresourcerequest.h>

using namespace KPlato;

TaskSplitDialog::TaskSplitDialog(Project *project, Task *task, QWidget *parent)
    : QDialog{parent}
    , m_project(project)
    , m_task(task)
{
    ui.setupUi(this);
}

KUndo2Command *TaskSplitDialog::buildCommand() const
{
    MacroCommand *m = new MacroCommand(kundo2_i18n("Split task"));
    const auto count = ui.parts->value();
    if (count > 1) {
        QList<Task*> tasks;
        for (int i = 0; i < count; ++i) {
            auto task = m_project->createTask(m_project->taskDefaults());
            task->setName(QStringLiteral("%1.%2").arg(m_task->name()).arg(i+1));
            if (ui.keep->isChecked()) {
                auto cmd = new SubtaskAddCmd(m_project, task, m_task);
                m->addCommand(cmd);
            } else {
                auto cmd = new TaskAddCmd(m_project, task, tasks.isEmpty() ? m_task : tasks.last());
                m->addCommand(cmd);
            }
            if (ui.migrateEstimate->isChecked()) {
                const auto estimate = m_task->estimate();
                auto e = task->estimate();
                *e = *estimate;
                e->setExpectedEstimate(estimate->expectedEstimate() / count);
                e->setOptimisticRatio(estimate->optimisticRatio());
                e->setPessimisticRatio(estimate->pessimisticRatio());
            }
            if (ui.migrateAllocations->isChecked()) {
                const auto requests = m_task->requests().resourceRequests(false);
                for (const auto request : requests) {
                    auto req = new ResourceRequest(*request);
                    req->setId(0); // new id will be generated on insert
                    m->addCommand(new AddResourceRequestCmd(&task->requests(), req));
                }
            }
            tasks << task;
        }
        if (ui.createDependencies->isChecked()) {
            // handle dependencies
            for (int i = 0; i < tasks.count(); ++i) {
                if (i == 0) {
                    // move parent relations from m_task to first task
                const auto relations = m_task->dependParentNodes();
                    for (const auto r : relations) {
                        auto rel = new Relation(r);
                        rel->setChild(tasks.first());
                        m->addCommand(new AddRelationCmd(*m_project, rel));
                        m->addCommand(new DeleteRelationCmd(*m_project, r));
                    }
                } else if (i == tasks.count() - 1) {
                    // move child relations from m_task to last task
                    const auto relations = m_task->dependChildNodes();
                    for (const auto r : relations) {
                        auto rel = new Relation(r);
                        rel->setParent(tasks.last());
                        m->addCommand(new AddRelationCmd(*m_project, rel));
                        m->addCommand(new DeleteRelationCmd(*m_project, r));
                    }
                }
                if (i > 0) {
                    // create dependency from prev task to task
                    auto rel = new Relation(tasks.at(i-1), tasks.at(i), Relation::FinishStart);
                    m->addCommand(new AddRelationCmd(*m_project, rel));
                }
            }
        }
        if (!ui.keep->isChecked()) {
            auto cmd = new NodeDeleteCmd(m_task);
            m->addCommand(cmd);
        }
    }
    if (m->isEmpty()) {
        delete m;
        m = nullptr;
    }
    return m;
}
