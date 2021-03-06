/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTDURATIONSPINBOX_H
#define KPTDURATIONSPINBOX_H

#include "kptduration.h"
#include "planmodels_export.h"

#include <QDoubleSpinBox>

namespace KPlato
{

/**
 * The DurationSpinBox provides a spinbox and a line edit to display and edit durations.
 *
 * DurationSpinBox is a QDoubleSpinBox with the addition of adjustable units,
 * defined as Duration::Unit.
 * The unit can be Day, Hour, Minute, Second and Millisecond.
 * The user can select the unit the duration is displayed in by placing the cursor
 * on the unit and step up or -down.
 * Maximum- and minimum unit can be set with setMaximumUnit() and setMinimumUnit().
 * Defaults are: maximum unit Day, minimum unit Hour.
 * 
 */
class PLANMODELS_EXPORT DurationSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit DurationSpinBox(QWidget *parent = nullptr);

    /// Return the current unit
    Duration::Unit unit() const { return m_unit; }
    
    /// step the value steps step. If inside unit, steps unit +/- 1 step.
    void stepBy(int steps) override;
    /// Set maximum unit to @p unit.
    void setMaximumUnit(Duration::Unit unit);
    /// Set maximum unit to @p unit.
    void setMinimumUnit(Duration::Unit unit);
    
    double valueFromText(const QString & text) const override;
    QString textFromValue (double value) const override;
    QValidator::State validate (QString & input, int & pos) const override;

Q_SIGNALS:
    void unitChanged(int);

public Q_SLOTS:
    /// Set the current unit.
    /// If unit is outside minimum- or maximum unit, the limit is adjusted.
    void setUnit(Duration::Unit unit);

protected Q_SLOTS:
    void editorTextChanged(const QString &text);

protected:
    void keyPressEvent(QKeyEvent * event) override;
    StepEnabled stepEnabled () const override;

    void stepUnitUp();
    void stepUnitDown();

    QString extractUnit (const QString &text) const;
    QString extractValue (const QString &text) const;

    /// If unit is outside minimum- or maximum unit, the limit is used.
    void updateUnit(Duration::Unit unit);

    bool isOnUnit() const;

private:
    Duration::Unit m_unit;
    Duration::Unit m_minunit;
    Duration::Unit m_maxunit;
};

} //namespace KPlato

#endif
