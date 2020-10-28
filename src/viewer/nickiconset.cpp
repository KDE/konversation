/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005-2006 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2020 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#include "nickiconset.h"

// Qt
#include <QFileInfo>
#include <QIconEngine>
#include <QPainter>

class OverlayIconEngine : public QIconEngine
{
public:
    OverlayIconEngine(const QIcon &icon, const QIcon &overlayIccon);
    ~OverlayIconEngine() override = default;

public: // QIconEngine API
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine *clone() const override;

    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;

    void addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state) override;
    void addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state) override;

private:
    QIcon m_baseIcon;
    QIcon m_overlayIcon;

    Q_DISABLE_COPY(OverlayIconEngine)
};

OverlayIconEngine::OverlayIconEngine(const QIcon &icon, const QIcon &overlayIcon)
    : QIconEngine()
    , m_baseIcon(icon)
    , m_overlayIcon(overlayIcon)
{
}

QIconEngine *OverlayIconEngine::clone() const
{
    return new OverlayIconEngine(m_baseIcon, m_overlayIcon);
}

QSize OverlayIconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return m_baseIcon.actualSize(size, mode, state);
}

QPixmap OverlayIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);

    paint(&p, pixmap.rect(), mode, state);

    return pixmap;
}

void OverlayIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state)
{
    m_baseIcon.addPixmap(pixmap, mode, state);
}

void OverlayIconEngine::addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    m_baseIcon.addFile(fileName, size, mode, state);
}

void OverlayIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    // paint the base icon as ground
    m_baseIcon.paint(painter, rect, Qt::AlignCenter, mode, state);

    m_overlayIcon.paint(painter, rect, Qt::AlignCenter, mode, state);
}



class AbstractIconElementSet
{
public:
    enum ElementIndex
    {
        Normal,
        Voice,
        HalfOp,
        Op,
        Owner,
        Admin,
        Away,
        AwayStacked,
        _ElementIndex_COUNT
    };

    AbstractIconElementSet() = default;
    virtual ~AbstractIconElementSet() = default;
    virtual bool load(const QString &baseDir) = 0;
    virtual void generateIcon(QIcon* icons, ElementIndex index, bool isOverlay) const = 0;
    virtual const QIcon & awayElement() const = 0;
    virtual int defaultIconSize() const = 0;

protected:
    static const struct LoadData
    {
        QLatin1String elementName;
        bool required;
    }
    m_loadData[_ElementIndex_COUNT];

private:
    Q_DISABLE_COPY(AbstractIconElementSet)
};

const AbstractIconElementSet::LoadData AbstractIconElementSet::m_loadData[_ElementIndex_COUNT] = {
    {QLatin1String("irc_normal"),       true},
    // all the overlays or substitutes for privileges
    {QLatin1String("irc_voice"),        true},
    {QLatin1String("irc_halfop"),       true},
    {QLatin1String("irc_op"),           true},
    {QLatin1String("irc_owner"),        true},
    {QLatin1String("irc_admin"),        true},
    // away overlays
    {QLatin1String("irc_away"),         true},
    {QLatin1String("irc_away_stacked"), false},
};

class SvgIconElementSet : public AbstractIconElementSet
{
public:
    SvgIconElementSet() = default;
    ~SvgIconElementSet() override = default;

    bool load(const QString &baseDir) override;

    void generateIcon(QIcon* icons, ElementIndex index, bool isOverlay) const override;
    const QIcon & awayElement() const override { return m_element[Away]; }
    int defaultIconSize() const override { return 16; }

private:
    QIcon m_element[_ElementIndex_COUNT];

    Q_DISABLE_COPY(SvgIconElementSet)
};

bool SvgIconElementSet::load(const QString &baseDir)
{
    // load element icons
    for (int i = 0; i < _ElementIndex_COUNT; ++i) {
        const LoadData& d = m_loadData[i];
        const QString path = baseDir + QLatin1Char('/') + d.elementName + QLatin1String(".svg");
        // try to load file
        if (!QFile::exists(path)) {
            if (d.required) {
                return false;
            }
            continue;
        }
        m_element[i] = QIcon(path);
        if (m_element[i].isNull()) {
            return false;
        }
    }

    return true;
}

void SvgIconElementSet::generateIcon(QIcon* icons, ElementIndex index, bool isOverlay) const
{
    const QIcon& awayOverlay = (index == Normal) || (!isOverlay) || m_element[AwayStacked].isNull() ? m_element[Away] : m_element[AwayStacked];

    QIcon presentIcon = isOverlay ?
            QIcon(new OverlayIconEngine(m_element[Normal], m_element[index])) : m_element[index];

    icons[NickIconSet::UserPresent] = presentIcon;
    icons[NickIconSet::UserAway] = QIcon(new OverlayIconEngine(presentIcon, awayOverlay));
}

class PixmapIconElementSet : public AbstractIconElementSet
{
public:
    PixmapIconElementSet() = default;
    ~PixmapIconElementSet() override = default;

    bool load(const QString &baseDir) override;

    void generateIcon(QIcon* icons, ElementIndex index, bool isOverlay) const override;
    const QIcon & awayElement() const override { return m_awayIcon; }
    int defaultIconSize() const override { return m_element[Normal].height(); }

private:
    static QPixmap generateOverlayedPixmap(const QPixmap &base, const QPixmap &overlay);

private:
    QPixmap m_element[_ElementIndex_COUNT];
    QIcon m_awayIcon;

    Q_DISABLE_COPY(PixmapIconElementSet)
};

bool PixmapIconElementSet::load(const QString &baseDir)
{
    // load element icons
    for (int i = 0; i < _ElementIndex_COUNT; ++i) {
        const LoadData& d = m_loadData[i];
        const QString path = baseDir + QLatin1Char('/') + d.elementName + QLatin1String(".png");
        // try to load file
        if (!QFile::exists(path)) {
            if (d.required) {
                return false;
            }
            continue;
        }
        m_element[i] = QPixmap(path);
        if (m_element[i].isNull()) {
            return false;
        }
    }
    m_awayIcon = QIcon(m_element[Away]);

    return true;
}

QPixmap PixmapIconElementSet::generateOverlayedPixmap(const QPixmap &base, const QPixmap &overlay)
{
    QPixmap result(base);
    QPainter painter(&result);
    painter.drawPixmap(QPoint(0,0), overlay);
    return result;
}

void PixmapIconElementSet::generateIcon(QIcon* icons, ElementIndex index, bool isOverlay) const
{
    const QPixmap& awayOverlay = (index == Normal) || (!isOverlay) || m_element[AwayStacked].isNull() ? m_element[Away] : m_element[AwayStacked];

    const QPixmap presentPixmap = isOverlay ?
            generateOverlayedPixmap(m_element[Normal], m_element[index]) : m_element[index];

    icons[NickIconSet::UserPresent] = QIcon(presentPixmap);
    icons[NickIconSet::UserAway] = QIcon(generateOverlayedPixmap(presentPixmap, awayOverlay));
}


bool NickIconSet::isNull() const
{
    return m_nickIcons[Images::Normal][UserPresent].isNull();
}

QIcon NickIconSet::nickIconAwayOverlay() const
{
    return m_nickIconAwayOverlay;
}

int NickIconSet::defaultIconSize() const
{
    return m_defaultIconSize;
}

QIcon NickIconSet::nickIcon(Images::NickPrivilege privilege, NickIconSet::UserPresence presence) const
{
    return m_nickIcons[privilege][presence];
}

void NickIconSet::clear()
{
    for (int i = 0; i < Images::_NickPrivilege_COUNT; ++i) {
        QIcon* icons = m_nickIcons[i];
        icons[UserPresent] = icons[UserAway] = QIcon();
    }
    m_nickIconAwayOverlay = QIcon();

    m_defaultIconSize = 0;
}

bool NickIconSet::load(const QString &baseDir)
{
    const bool isDefaultTheme = baseDir.endsWith(QLatin1String("/default")) || baseDir.endsWith(QLatin1String("/default-dark"));

    QScopedPointer<AbstractIconElementSet> elements;

    // detect file format by testing existance of normal user element, prefer SVG
    const QString suffixLessNormalIconPath = baseDir + QLatin1String("/irc_normal");

    if (QFileInfo::exists(suffixLessNormalIconPath + QLatin1String(".svg"))) {
        elements.reset(new SvgIconElementSet);
    } else if (QFileInfo::exists(suffixLessNormalIconPath + QLatin1String(".png"))) {
        elements.reset(new PixmapIconElementSet);
    }

    if (!(elements && elements->load(baseDir))) {
        clear();
        return false;
    }

    const bool isOwnerOverlay = !isDefaultTheme;
    const bool isAdminOverlay = !isDefaultTheme;

    // compose final icons
    m_nickIconAwayOverlay = elements->awayElement();

    elements->generateIcon(m_nickIcons[Images::Normal], AbstractIconElementSet::Normal, false);
    elements->generateIcon(m_nickIcons[Images::Voice],  AbstractIconElementSet::Voice,  true);
    elements->generateIcon(m_nickIcons[Images::HalfOp], AbstractIconElementSet::HalfOp, true);
    elements->generateIcon(m_nickIcons[Images::Op],     AbstractIconElementSet::Op,     true);
    elements->generateIcon(m_nickIcons[Images::Owner],  AbstractIconElementSet::Owner,  isOwnerOverlay);
    elements->generateIcon(m_nickIcons[Images::Admin],  AbstractIconElementSet::Admin,  isAdminOverlay);

    m_defaultIconSize = elements->defaultIconSize();

    return true;
}
