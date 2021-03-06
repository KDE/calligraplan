/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/


// clazy:excludeall=qstring-arg
#include <kptdurationspinbox.h>

#include "kptnode.h"

#include <QLineEdit>
#include <QDoubleValidator>
#include <QKeyEvent>
#include <QLocale>

#include <math.h>
#include <limits.h>

namespace KPlato
{

DurationSpinBox::DurationSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent),
    m_unit(Duration::Unit_d),
    m_minunit(Duration::Unit_h),
    m_maxunit(Duration::Unit_Y)
{
    setUnit(Duration::Unit_h);
    setMaximum(140737488355328.0); //Hmmmm

    connect(lineEdit(), &QLineEdit::textChanged, this, &DurationSpinBox::editorTextChanged);

}

void DurationSpinBox::setUnit(Duration::Unit unit)
{
    if (unit < m_maxunit) {
        m_maxunit = unit;
    } else if (unit > m_minunit) {
        m_minunit = unit;
    }
    m_unit = unit;
    setValue(value());
}

void DurationSpinBox::setMaximumUnit(Duration::Unit unit)
{
    //NOTE Year = 0, Milliseconds = 7 !!!
    m_maxunit = unit;
    if (m_minunit < unit) {
        m_minunit = unit;
    }
    if (m_unit < unit) {
        setUnit(unit);
        Q_EMIT unitChanged(m_unit);
    }
}

void DurationSpinBox::setMinimumUnit(Duration::Unit unit)
{
    //NOTE Year = 0, Milliseconds = 7 !!!
    m_minunit = unit;
    if (m_maxunit > unit) {
        m_maxunit = unit;
    }
    if (m_unit > unit) {
        setUnit(unit);
        Q_EMIT unitChanged(m_unit);
    }
}

void DurationSpinBox::stepUnitUp()
{
    //debugPlan<<m_unit<<">"<<m_maxunit;
    if (m_unit > m_maxunit) {
        setUnit(static_cast<Duration::Unit>(m_unit - 1));
        // line may change length, make sure cursor stays within unit
        lineEdit()->setCursorPosition(lineEdit()->displayText().length() - suffix().length());
        Q_EMIT unitChanged(m_unit);
    }
}

void DurationSpinBox::stepUnitDown()
{
    //debugPlan<<m_unit<<"<"<<m_minunit;
    if (m_unit < m_minunit) {
        setUnit(static_cast<Duration::Unit>(m_unit + 1));
        // line may change length, make sure cursor stays within unit
        lineEdit()->setCursorPosition(lineEdit()->displayText().length() - suffix().length());
        Q_EMIT unitChanged(m_unit);
    }
}

void DurationSpinBox::stepBy(int steps)
{
    //debugPlan<<steps;
    int cpos = lineEdit()->cursorPosition();
    if (isOnUnit()) {
        // we are in unit
        if (steps > 0) {
            stepUnitUp();
        } else if (steps < 0) {
            stepUnitDown();
        }
        lineEdit()->setCursorPosition(cpos);
        return;
    }
    QDoubleSpinBox::stepBy(steps);
    // QDoubleSpinBox selects the whole text and the cursor might end up at the end (in the unit field)
    lineEdit()->setCursorPosition(cpos); // also deselects
}

QAbstractSpinBox::StepEnabled DurationSpinBox::stepEnabled () const
{
    if (isOnUnit()) {
        if (m_unit >= m_minunit) {
            //debugPlan<<"inside unit, up"<<m_unit<<m_minunit<<m_maxunit;
            return QAbstractSpinBox::StepUpEnabled;
        }
        if (m_unit <= m_maxunit) {
            //debugPlan<<"inside unit, down"<<m_unit<<m_minunit<<m_maxunit;
            return QAbstractSpinBox::StepDownEnabled;
        }
        //debugPlan<<"inside unit, up|down"<<m_unit<<m_minunit<<m_maxunit;
        return QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    }
    return QDoubleSpinBox::stepEnabled();
}

bool DurationSpinBox::isOnUnit() const
{
    int pos = lineEdit()->cursorPosition();
    return (pos <= text().size() - suffix().size()) &&
           (pos > text().size() - suffix().size() - Duration::unitToString(m_unit, true).size());
}

void DurationSpinBox::keyPressEvent(QKeyEvent * event)
{
    //debugPlan<<lineEdit()->cursorPosition()<<","<<(text().size() - Duration::unitToString(m_unit, true).size())<<""<<event->text().isEmpty();
    if (isOnUnit()) {
        // we are in unit
        switch (event->key()) {
        case Qt::Key_Up:
            event->accept();
            stepBy(1);
            return;
        case Qt::Key_Down:
            event->accept();
            stepBy(-1);
            return;
        default:
            break;
        }
    }
    QDoubleSpinBox::keyPressEvent(event);
}

// handle unit, QDoubleSpinBox handles value, signals etc
void DurationSpinBox::editorTextChanged(const QString &text) {
    //debugPlan<<text;
    QString s = text;
    int pos = lineEdit()->cursorPosition();
    if (validate(s, pos) == QValidator::Acceptable) {
        s = extractUnit(s);
        if (! s.isEmpty()) {
            updateUnit((Duration::Unit)Duration::unitList(true).indexOf(s));
        }
    }
}

double DurationSpinBox::valueFromText(const QString & text) const
{
    QString s = extractValue(text);
    bool ok = false;
    double v = QLocale().toDouble(s, &ok);
    if (! ok) {
        v = QDoubleSpinBox::valueFromText(s);
    }
    return v;
}

QString DurationSpinBox::textFromValue (double value) const
{
    QString s = QLocale().toString(qMin(qMax(minimum(), value), maximum()), 'f', decimals());
    s += Duration::unitToString(m_unit, true);
    //debugPlan<<2<<value<<s;
    return s;
}

QValidator::State DurationSpinBox::validate (QString & input, int & pos) const
{
    //debugPlan<<input;
    QDoubleValidator validator(minimum(), maximum(), decimals(), nullptr);
    if (input.isEmpty()) {
        return validator.validate (input, pos);
    }
    QString s = extractUnit(input);
    if (s.isEmpty()) {
        return validator.validate (input, pos);
    }
    int idx = Duration::unitList(true).indexOf(s); 
    if (idx < m_maxunit || idx > m_minunit) {
        return QValidator::Invalid;
    }
    s = extractValue(input);
    int p = 0;
    return validator.validate (s, p); // pos doesn't matter
}

QString DurationSpinBox::extractUnit (const QString &text) const
{
    //debugPlan<<text;
    QString s;
    for (int i = text.length() - 1; i >= 0; --i) {
        QChar c = text[ i ];
        if (! c.isLetter()) {
            break;
        }
        s.prepend(c);
    }
    if (Duration::unitList(true).contains(s)) {
        return s;
    }
    return QString();
}

QString DurationSpinBox::extractValue (const QString &text) const
{
    //debugPlan<<text;
    QString s = extractUnit(text);
    if (Duration::unitList(true).contains(s)) {
        return text.left(text.length() - s.length());
    }
    return text;
}

void DurationSpinBox::updateUnit(Duration::Unit unit)
{
    if (unit < m_maxunit) {
        m_unit = m_maxunit;
    } else if (unit > m_minunit) {
        m_unit = m_minunit;
    }
    if (m_unit != unit) {
        m_unit = unit;
        Q_EMIT unitChanged(unit);
    }
}


} //namespace KPlato
