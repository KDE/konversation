/*
  Copyright (C) 2017 by Eike Hein <hein@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

#include <KLocalizedString>

#include <KPackage/Package>
#include <KPackage/PackageStructure>

class ThemeStructure : public KPackage::PackageStructure
{
    Q_OBJECT

public:
    explicit ThemeStructure(QObject *parent = 0, const QVariantList &args = QVariantList())
        : KPackage::PackageStructure(parent, args) {};

    void initPackage(KPackage::Package *package) Q_DECL_OVERRIDE
    {
        package->setDefaultPackageRoot(QStringLiteral("konversation/uipackages/"));

        package->addFileDefinition("window", QStringLiteral("main.qml"), i18n("Konversation main window content"));
        package->setRequired("main", true);
    }
};

K_EXPORT_KPACKAGE_PACKAGE_WITH_JSON(ThemeStructure, "konversation-packagestructure-uipackage.json")

#include "themestructure.moc"
