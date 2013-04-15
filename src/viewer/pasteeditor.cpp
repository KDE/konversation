// Copyright 2009 Peter Simonsson <peter.simonsson@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "pasteeditor.h"
#include "preferences.h"
#include "application.h"

#include <KPushButton>
#include <KLocale>
#include <KActionCollection>

#include <QTextCursor>
#include <QTextOption>
#include <QPointer>
#include <QAction>

PasteEditor::PasteEditor(QWidget* parent)
    : KDialog(parent), Ui::PasteEditor(), m_autoReplaceActionWasEnabled(true), m_autoReplaceAction(0)
{
    setCaption(i18n("Edit Multiline Paste"));
    setModal(true);

    setButtonText(KDialog::Ok, i18n("&Send"));

    QWidget* widget = new QWidget(this);
    setupUi(widget);
    setMainWidget(widget);

    m_textEditor->enableFindReplace(true);

    QTextOption options = m_textEditor->document()->defaultTextOption();
    options.setFlags(options.flags() | QTextOption::ShowTabsAndSpaces);
    m_textEditor->document()->setDefaultTextOption(options);
    m_textEditor->setFocus();

    // Get auto_replace action, check enabled state and make the connection
    m_autoReplaceAction = Application::instance()->getMainWindow()->actionCollection()->action("auto_replace");
    if (m_autoReplaceAction)
    {
        // Store action's original enabled state in ViewContainer
        m_autoReplaceActionWasEnabled = m_autoReplaceAction->isEnabled();
        m_autoReplaceAction->setEnabled(true);
        addAction(m_autoReplaceAction);
        connect(m_autoReplaceAction, SIGNAL(triggered(bool)), this, SLOT(doInlineAutoreplace()));
    }

    connect(m_removeNewlinesButton, SIGNAL(clicked()), this, SLOT(removeNewlines()));
    connect(m_addQuotesButton, SIGNAL(clicked()), this, SLOT(addQuotationIndicators()));

    setInitialSize(Preferences::self()->multilineEditSize());
}

PasteEditor::~PasteEditor()
{
    Preferences::self()->setMultilineEditSize(size());
    // Restore action's original state in ViewContainer
    if (m_autoReplaceAction)
        m_autoReplaceAction->setEnabled(m_autoReplaceActionWasEnabled);
}

void PasteEditor::addQuotationIndicators()
{
    QTextCursor cursor(m_textEditor->document());
    cursor.beginEditBlock();
    cursor.insertText("> ");

    while(cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor))
    {
        cursor.insertText("> ");
    }

    cursor.endEditBlock();
}

void PasteEditor::removeNewlines()
{
    QString text(m_textEditor->toPlainText());
    text.remove('\r');
    text.remove(QRegExp("^\n+|\n+$"));

    int i = 0;

    while (i < text.length())
    {
        if (text[i] == '\n')
        {
            if (text[i - 1].category() == QChar::Separator_Space)
            {
                text.remove(i, 1);
                continue;
            }
            else
                text[i] = ' ';
        }

        ++i;
    }

    QTextCursor cursor(m_textEditor->document());
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
}

void PasteEditor::setText(const QString& text)
{
    m_textEditor->setPlainText(text);
}

QString PasteEditor::text() const
{
    return m_textEditor->toPlainText();
}

QString PasteEditor::edit(QWidget* parent, const QString& text)
{
    QPointer<PasteEditor> dialog = new PasteEditor(parent);
    dialog->setText(text);

    if(dialog->exec())
    {
        const QString text = dialog->text();
        delete dialog;
        return text;
    }
    delete dialog;
    return QString();
}

void PasteEditor::doInlineAutoreplace()
{
    Application::instance()->doInlineAutoreplace(m_textEditor);
}

#include "pasteeditor.moc"
