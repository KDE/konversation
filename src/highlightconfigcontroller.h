//
// C++ Interface: highlightconfigcontroller
//
// Description: 
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2003, 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KONVERSATIONHIGHLIGHTCONFIGCONTROLLER_H
#define KONVERSATIONHIGHLIGHTCONFIGCONTROLLER_H

#include <qobject.h>

class Highlight_Config;

/**
  @author Dario Abatianni
*/

class HighlightConfigController : public QObject
{
  Q_OBJECT
  
  public:
    HighlightConfigController(Highlight_Config* highlightPage,QObject *parent = 0, const char *name = 0);
    ~HighlightConfigController();

  public:
    void populateHighlightList();
    void saveSettings();

  signals:
    void modified();
  
  protected slots:
    void highlightSelected(QListViewItem* item);
    void highlightTextChanged(const QString& newPattern);
    void highlightTextEditButtonClicked();
    void highlightColorChanged(const QColor& newColor);
    void soundURLChanged(const QString& newURL);
    void autoTextChanged(const QString& newText);
    void addHighlight();
    void removeHighlight();
    void playSound();
    QPtrList<Highlight> getHighlightList();
  
  protected:
    bool newItemSelected;
    Highlight_Config* m_highlightPage;
};

#endif
