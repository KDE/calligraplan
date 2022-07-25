/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_RESOURCEUSAGEVIEW_H
#define PLANPORTFOLIO_RESOURCEUSAGEVIEW_H

#include "planportfolio_export.h"

#include "ui_ResourceUsageView.h"
#include "ResourceUsageModel.h"

#include <KoView.h>

#include <QSpinBox>

#include <KChartLineDiagram>
#include <KChartCartesianCoordinatePlane>
class KoDocument;
class KoPrintJob;
class QMenu;

namespace KChart {
    class Chart;
    class LineDiagram;
}

class ResourceUsageModel;

class LineDiagram : public KChart::LineDiagram
{
    Q_OBJECT
public:
    LineDiagram(QWidget *parent = nullptr, KChart::CartesianCoordinatePlane* plane = nullptr);

    void resize(const QSizeF &size) override;

Q_SIGNALS:
    void sizeChanged(const QSizeF &size);
};

class PLANPORTFOLIO_EXPORT ResourceUsageView : public KoView
{
    Q_OBJECT

public:
    explicit ResourceUsageView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~ResourceUsageView() override;

    QMenu *popupMenu(const QString& name);

    KoPrintJob *createPrintJob() override;

    void guiActivateEvent(bool activated) override;

public Q_SLOTS:
    void setNumDays(int days);

protected Q_SLOTS:
    void slotCurrentIndexChanged(const QModelIndex &current, const QModelIndex &previous);
    void slotUpdateNumDays();
    void slotNumDaysChanged(int value);

    void updateMarker();

protected:
    void updateReadWrite(bool readwrite) override;

private:
    void setupGui();
    void setOverrideCursor();

private Q_SLOTS:
    void slotRestoreOverrideCursor();

private:
    bool m_readWrite;
    Ui::ResourceUsageView ui;
    ResourceUsageModel m_resourceUsageModel;
    QSpinBox *m_numDays;
    LineDiagram *m_available;
    bool m_overrideCursorSet = false;
};

#endif
