/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001-2007 OpenMFG LLC (info@openmfg.com)
 * SPDX-FileCopyrightText: 2007-2008 Adam Pigg (adam@piggz.co.uk)
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef PlanReportDesignerItemText_H
#define PlanReportDesignerItemText_H

#include "PlanReportItemText.h"

#include "KReportDesignerItemRectBase.h"

class KProperty;

class PlanReportDesignerItemText : public PlanReportItemText, public KReportDesignerItemRectBase
{
  Q_OBJECT
public:
    PlanReportDesignerItemText(KReportDesigner *, QGraphicsScene * scene, const QPointF &pos);
    PlanReportDesignerItemText(const QDomNode & element, KReportDesigner *, QGraphicsScene * scene);
    virtual ~PlanReportDesignerItemText();

    virtual void buildXML(QDomDocument *doc, QDomElement *parent);
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
    virtual PlanReportDesignerItemText* clone();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);

private:
    QRect getTextRect() const;
    void init(QGraphicsScene*, KReportDesigner*);

private Q_SLOTS:
    void slotPropertyChanged(KPropertySet &, KProperty &);
};

#endif
