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

#include <KPushButton>
#include <KLocale>

#include <QTextCursor>
#include <QTextOption>
#include <QPointer>

PasteEditor::PasteEditor(QWidget* parent)
    : KDialog(parent), Ui::PasteEditor()
{
    setCaption(i18n("Edit Multiline Paste"));
    setModal(true);

    QWidget* widget = new QWidget(this);
    setupUi(widget);
    setMainWidget(widget);

    m_textEditor->enableFindReplace(true);
#if QT_VERSION >= 0x040500
    QTextOption options = m_textEditor->document()->defaultTextOption();
    options.setFlags(options.flags() | QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators);
    m_textEditor->document()->setDefaultTextOption(options);
#endif
    connect(m_removeNewlinesButton, SIGNAL(clicked()), this, SLOT(removeNewlines()));
    connect(m_addQuotesButton, SIGNAL(clicked()), this, SLOT(addQuotationIndicators()));

    setInitialSize(Preferences::self()->multilineEditSize());
}

PasteEditor::~PasteEditor()
{
    Preferences::self()->setMultilineEditSize(size());
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
    QTextCursor cursor(m_textEditor->document());
    cursor.beginEditBlock();

    while(cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor))
    {
        cursor.deletePreviousChar();

        if (!cursor.atBlockEnd())
        {
            bool moved = cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);

            if (moved)
            {
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);

                if (!cursor.selectedText().contains(' '))
                {
                    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);
                    cursor.insertText(" ");
                }
            }
        }
    }

    cursor.endEditBlock();
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

#include "pasteeditor.moc"
