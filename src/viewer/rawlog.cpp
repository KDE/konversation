/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "rawlog.h"
#include "channel.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "server.h"
#include "application.h"


RawLog::RawLog(QWidget* parent) : ChatWindow(parent)
{
    setName(i18n("Raw Log"));
    setType(ChatWindow::RawLog);
    m_isTopLevelView = false;
    auto* ircBox = new IRCViewBox(this);
    setTextView(ircBox->ircView());               // Server will be set later in setServer()

    updateAppearance();
}

RawLog::~RawLog()
{
}

void RawLog::childAdjustFocus()
{
}

void RawLog::morphNotification()
{
    activateTabNotification(Konversation::tnfSystem);
}

bool RawLog::closeYourself()
{
    // make the server delete us so server can reset the pointer to us
    m_server->closeRawLog();
    return true;
}

bool RawLog::searchView() const { return true; }

bool RawLog::log() const { return false; }

// The QByteArray implementation of this uses the unintelligent q_strchr for its "exclude" list.
// Additionally, we need to escape certain values for the IrcView, so rather than letting it loop
// over dontEncode repeatedly, and then have QString do the replacements afterwards, we'll just
// reimplement and let QString do its thing as far as memory goes.

static const char tr16[] = "0123456789ABCDEF";

static QString toPercentEncoding(const QByteArray& input, QString output=QString()) // Do NOT 'fix' this for krazy, I want to see the EBN --argonel
{
    int len = input.size();
    const char *inputData = input.constData();

    // skip a few allocations - most of my raw log is more than 200 characters with "&nbsp;" replaced
    // qAllocMore will do 256-12 if you want more than 116 (128-12) bytes
    output.reserve(244);

    char hexbuf[4]="%00";
    QLatin1String l16(hexbuf); // QString does a deep copy on append

    for (int i = 0; i < len; ++i)
    {
        unsigned char c = *inputData++;
        if ((c >= 0x21 && c <= 0x25) || (c >= 0x27 && c <= 0x3B) || (c >= 0x3F && c<= 0x7E))
            output.append(QLatin1Char(c));
        else
        {
            switch (c)
            {
                case 0x20:
                    output.append(QLatin1String("&nbsp;"));
                break;
                case 0x26:
                    output.append(QLatin1String("&amp;"));
                break;
                case 0x3C:
                    output.append(QLatin1String("&lt;"));
                break;
                case 0x3D:
                    output.append(QLatin1Char('='));
                break;
                case 0x3E:
                    output.append(QLatin1String("&gt;"));
                break;

                default:
                    hexbuf[1] = tr16[((c & 0xF0) >> 4)];
                    hexbuf[2] = tr16[c & 0x0F];
                    output.append(l16);
            }
        }
    }
    return output;
}

void RawLog::appendRaw(RawLog::MessageDirection dir, const QByteArray& message)
{
    if (!getTextView() || message.isEmpty())
        return;

    static const QLatin1String in("&gt;&gt; ");
    static const QLatin1String out("&lt;&lt; ");
    QString output = toPercentEncoding(message, (dir == RawLog::Inbound ? in : out));

    // Whatever the original line inbound ending was is too much effort to conserve, but its nice
    // to see the actual line endings, so we'll fake it here
    output.append(QLatin1String("%0A"));

    appendRaw(output, dir == RawLog::Outbound);
}

#include "moc_rawlog.cpp"
