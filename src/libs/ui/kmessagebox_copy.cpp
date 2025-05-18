/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Waldo Bastian (bastian@kde.org)

    SPDX-License-Identifier: LGPL-2.0-only
*/

// NOTE: This is copied from kmessagebox.cpp

// clazy:excludeall=qstring-arg
#include <QMessageBox>

#include <KIconLoader>

#include <KoIcon.h>

static QIcon themedMessageBoxIcon(QMessageBox::Icon icon)
{
    const char *icon_name = nullptr;

    switch (icon) {
    case QMessageBox::NoIcon:
        return QIcon();
        break;
    case QMessageBox::Information:
        icon_name = koIconNameCStr("dialog-information");
        break;
    case QMessageBox::Warning:
        icon_name = koIconNameCStr("dialog-warning");
        break;
    case QMessageBox::Critical:
        icon_name = koIconNameCStr("dialog-error");
        break;
    default:
        break;
    }

   QIcon ret = KIconLoader::global()->loadIcon(QLatin1String(icon_name), KIconLoader::NoGroup, KIconLoader::SizeHuge, KIconLoader::DefaultState, QStringList(), nullptr, true);

   if (ret.isNull()) {
        QStyle::StandardPixmap spIcon = QStyle::SP_MessageBoxInformation;
        switch (icon) {
        case QMessageBox::Information:
            spIcon = QStyle::SP_MessageBoxInformation;
            break;
        case QMessageBox::Warning:
            spIcon = QStyle::SP_MessageBoxWarning;
            break;
        case QMessageBox::Critical:
            spIcon = QStyle::SP_MessageBoxCritical;
            break;
        case QMessageBox::Question:
            spIcon = QStyle::SP_MessageBoxQuestion;
            break;
        default:
            return QIcon();
        }
        return qApp->style()->standardIcon(spIcon);
   } else {
       return ret;
   }
}

