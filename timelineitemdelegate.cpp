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
*/

#include "timelineitemdelegate.h"
#include "timelinemodel.h"
#include "twitter.h"
#include "iconmanager.h"
#include "qweensettings.h"
#include <QtGui>

TimelineItemDelegate::TimelineItemDelegate(IconManager *iconMgr, QWidget *parent) :
    QStyledItemDelegate(parent), m_iconMgr(iconMgr)
{
}

void TimelineItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Twitter::TwitterItem *item = (Twitter::TwitterItem *)index.internalPointer();

    QStyleOptionViewItemV4 myOption = option;
    initStyleOption(&myOption, index);

    if(!item->read())
        myOption.font.setBold(true);

    if(index.row() == item->parent()->baseIndex()){
        //myOption.palette.setColor(QPalette::Highlight, QColor(255,255,0));
    }
    /*int size = QweenSettings::globalSettings()->iconSize()*8+8;
    myOption.decorationSize.setHeight(size);
    myOption.decorationSize.setWidth(size);
    myOption.rect.setHeight(qMax(myOption.rect.height(),size));*/
    QStyledItemDelegate::paint(painter, myOption, index);
}

//TODO: 色の設定を実装
