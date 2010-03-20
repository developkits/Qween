/*
  This file is part of Qween.
  Copyright (C) 2009-2010 NOSE Takafumi <ahya365@gmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  In addition, as a special exception, NOSE Takafumi
  gives permission to link the code of its release of Qween with the
  OpenSSL project's "OpenSSL" library (or with modified versions of it
  that use the same license as the "OpenSSL" library), and distribute
  the linked executables.  You must obey the GNU General Public License
  in all respects for all of the code used other than "OpenSSL".  If you
  modify this file, you may extend this exception to your version of the
  file, but you are not obligated to do so.  If you do not wish to do
  so, delete this exception statement from your version.
*/

#include "qweeninputbox.h"
#include "urishortensvc.h"
#include "const.h"
#include <QtGui>
QweenInputBox::QweenInputBox(QWidget *parent) :
    MultipleLineEdit(parent), m_pos(0), m_completer(new QCompleter(this)), m_shortenSvcName("bitly"),
    m_uriShortenSvc(getUriShortenService(m_shortenSvcName, this))
{
    connect(m_uriShortenSvc, SIGNAL(uriShortened(QString,QString)),
            this, SLOT(OnUriShortened(QString,QString)));
    connect(m_uriShortenSvc, SIGNAL(failed(QString,int)),
            this, SLOT(OnUriShorteningFailed(QString,int)));
}


void QweenInputBox::setCompleter(QCompleter *completer){
    if (m_completer)
        disconnect(m_completer, 0, this, 0);
    m_completer = completer;
    if (!m_completer)
        return;
    connect(m_completer, SIGNAL(activated(const QString&)),
                     this, SLOT(insertCompletion(const QString&)));
    m_completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setWrapAround(false);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setWidget(this);
}

QCompleter* QweenInputBox::completer() const { return m_completer; }

void QweenInputBox::setUriShortenSvc(const QString& name){
    if(m_uriShortenSvc) {
        disconnect(m_uriShortenSvc, SIGNAL(uriShortened(QString,QString)),
                this, SLOT(OnUriShortened(QString,QString)));
        disconnect(m_uriShortenSvc, SIGNAL(failed(QString,int)),
                this, SLOT(OnUriShorteningFailed(QString,int)));
        delete m_uriShortenSvc;
        m_uriShortenSvc = NULL;
    }
    m_shortenSvcName = name;
    m_uriShortenSvc = getUriShortenService(m_shortenSvcName, this);
    connect(m_uriShortenSvc, SIGNAL(uriShortened(QString,QString)),
            this, SLOT(OnUriShortened(QString,QString)));
    connect(m_uriShortenSvc, SIGNAL(failed(QString,int)),
            this, SLOT(OnUriShorteningFailed(QString,int)));
}

void QweenInputBox::insertCompletion(const QString& completion)
{
    if (m_completer->widget() != this)
        return;
    /*int tc = this->textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    this->insert(completion.right(extra));*/
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

QString QweenInputBox::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void QweenInputBox::keyPressEvent(QKeyEvent *event)
{
    if (m_completer && m_completer->popup()->isVisible()) {
       switch (event->key()) {
       case Qt::Key_Enter:
       case Qt::Key_Return:
       case Qt::Key_Escape:
       case Qt::Key_Tab:
       case Qt::Key_Backtab:
            event->ignore();
            return; // let the completer do default behavior
       case Qt::Key_Backspace:
       case Qt::Key_Left:
       {
            if(textUnderCursor().endsWith('@')){
                m_completer->popup()->hide();
            }
            break;
        }
       default:
            break;
       }
    }

    if(!event->modifiers().testFlag(Qt::ShiftModifier) &&
       !event->modifiers().testFlag(Qt::ControlModifier) &&
       !event->modifiers().testFlag(Qt::AltModifier)){
        if(event->key() == Qt::Key_Space){
            if(toPlainText() == " " || toPlainText() == "　"){
                setPlainText("");
                return;
                //TODO:JumpUnreadMenuItem_Click(Nothing, Nothing);
            }
        }else if(event->key() == Qt::Key_Return){
            if(requireCtrlOnEnter()){
                event->ignore();
                return;
            }
        }
    }

    if(!event->modifiers().testFlag(Qt::ShiftModifier) &&
        event->modifiers().testFlag(Qt::ControlModifier) &&
       !event->modifiers().testFlag(Qt::AltModifier)){
        if(event->key() == Qt::Key_Up || event->key() == Qt::Key_Down){
            //TODO:入力履歴を上下する
        }else if(event->key() == Qt::Key_PageUp){
            //TODO:右のタブを選択
        }else if(event->key() == Qt::Key_PageUp){
            //TODO:左のタブを選択
        }else if(event->key() == Qt::Key_Return){
            if(requireCtrlOnEnter()){
                emit returnPressed();
                event->ignore();
                return;
            }
        }
    }
    MultipleLineEdit::keyPressEvent(event);

    QString completionPrefix = textUnderCursor();
    int begin;
    begin = completionPrefix.lastIndexOf('@');
    completionPrefix = completionPrefix.mid(begin+1, completionPrefix.length());
    if (completionPrefix != m_completer->completionPrefix()) {
        m_completer->setCompletionPrefix(completionPrefix);
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    }

    if(!event->modifiers().testFlag(Qt::ShiftModifier) &&
       !event->modifiers().testFlag(Qt::ControlModifier) &&
       !event->modifiers().testFlag(Qt::AltModifier)){
        if(event->key() == Qt::Key_At){
            //Start AutoComplete
            if(m_completer){
                QRect cr = cursorRect();
                cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                            + m_completer->popup()->verticalScrollBar()->sizeHint().width());
                m_completer->complete(cr);
            }
        }
    }
}

void QweenInputBox::doShorten(){
    QRegExp rx(URLRXDATA);
    int begin;
    if(!m_uriShortenSvc) return;
    if((begin=rx.indexIn(toPlainText(),m_pos))!=-1){
        m_pos = begin;
        m_uriShortenSvc->shortenAsync(rx.capturedTexts().at(0));
    }else{
        m_pos = 0;
        setEnabled(true);
        emit uriShorteningFinished();
    }
}

void QweenInputBox::shortenUri(const QString& svcName){
    if(svcName.isEmpty())
        setUriShortenSvc("bitly");
    else
        setUriShortenSvc(svcName);
    this->setEnabled(false);
    doShorten();
}

void QweenInputBox::OnUriShortened(const QString& src, const QString& dest){
    QString newstr = toPlainText().mid(0,m_pos) + dest + toPlainText().mid(m_pos+src.length(),toPlainText().length()-m_pos+src.length());
    m_pos += dest.length();
    setPlainText(newstr);
    doShorten();
}

void QweenInputBox::OnUriShorteningFailed(const QString& src, int status){
    Q_UNUSED(src)
    Q_UNUSED(status)
    m_pos = 0;
    setEnabled(true);
    emit uriShorteningFinished();
}
