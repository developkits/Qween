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

#include "timelinemodel.h"
#include "iconmanager.h"
#include "qweensettings.h"

TimelineModel::TimelineModel(IconManager *iconMgr, QObject *parent)
     : QAbstractItemModel(parent), m_iconMgr(iconMgr), m_textDoc(new QTextDocument(this)), m_settings(QweenSettings::globalSettings()), m_baseIndex(-1), m_myId(0), m_newestId(0),
     m_unreadCount(0)
 {
    connect(m_iconMgr, SIGNAL(iconDownloaded(quint64,QIcon)),
            this, SLOT(OnIconDownloaded(quint64,QIcon)));
 }

int TimelineModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_itemList.size();
}

int TimelineModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 5;
}

QVariant TimelineModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_itemList.size() || index.row() < 0)
        return QVariant();

    Twitter::TwitterItem *item = m_itemList.at(index.row());
    if (role == Qt::DisplayRole) {

        switch(index.column()){
        case 0:
            return item->userName();
            break;
        case 1:
            return item->plainStatus();
            break;
        case 2:
            return item->createdAt().toLocalTime().toString(QweenSettings::globalSettings()->dateTimeFormat());
            break;
        case 3:
            if(item->favorited()){
                return tr("★");
            }else{
                return "";
            }
            break;
        case 4:
            return item->screenName();
            break;
        case 5:
            return item->id();
            break;
        default:
            return QVariant();
        }
        }else if(role == Qt::DecorationRole){
        switch(index.column()){
        case 0:
            if(m_iconMgr->isIconAvailable(item->userId())){
                return m_iconMgr->getIcon(item->userId());
            }else{
                m_iconMgr->fetchIcon(item->userId(), item->iconUri());
                return QIcon();
            }
            break;
        case 1:
            if(item->isProtected()){
                return QIcon(":/res/lock.png");
            }else{
                return QIcon();
            }
            break;
        default:
            return QIcon();
        }
    }else if(role == Qt::BackgroundRole){
        quint64 baseUserId;
        if(this->baseIndex() >= 0){
            Twitter::TwitterItem baseItem = itemAt(baseIndex());
            if(item->id() == baseItem.inReplyToId())
                return QBrush(QweenSettings::globalSettings()->atReplyColor()); //@先
            else if(baseItem.userId() == item->userId())
                return QBrush(QweenSettings::globalSettings()->selUserColor());
            //else if(item->screenName() == baseItem.screenName())
            //    return QBrush(QColor(192,255,192));
            else if(item->replyToList().indexOf(QweenSettings::globalSettings()->userid())>=0)
                return QBrush(QweenSettings::globalSettings()->replyToMeColor());
            else if(baseItem.replyToList().indexOf(item->screenName())>=0)
                return QBrush(QweenSettings::globalSettings()->sel2ReplyColor());
            else if(item->replyToList().indexOf(baseItem.screenName())>=0)
                return QBrush(QweenSettings::globalSettings()->reply2SelColor());
        }
    }else if(role == Qt::ForegroundRole){
        if(item->userId() == myId())
            return QBrush(QweenSettings::globalSettings()->selfColor());
    }
    return QVariant();
}

QVariant TimelineModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("名前");
        case 1:
            return tr("投稿");
        case 2:
            return tr("日時");
        case 3:
            return tr("Fav");
        case 4:
            return tr("ユーザ名");
        case 5:
            return tr("ID");

        default:
            return QVariant();
        }
    }
    return QVariant();
}

void TimelineModel::appendItem(Twitter::TwitterItem item)//, bool ignoreId)
{
    int index;
    if(!item.read()){
        m_unreadCount++;
        emit unreadCountChanged(m_unreadCount);
    }
    if(item.id()>m_newestId){
        m_newestId = item.id();
        index = m_itemList.count();
        beginInsertRows(QModelIndex(), index, index);
        Twitter::TwitterItem *internalItem = new Twitter::TwitterItem(item);
        internalItem->setParent(this);
        m_itemList.append(internalItem);
        endInsertRows();
    }else{
        int i;
        for(i = m_itemList.count() - 1;i>=0;i--){
            if(m_itemList.at(i)->id() > item.id()) continue;
            else if(m_itemList.at(i)->id() == item.id()) return;
            else {
                index = i+1;
                beginInsertRows(QModelIndex(), index, index);
                Twitter::TwitterItem *internalItem = new Twitter::TwitterItem(item);
                internalItem->setParent(this);
                m_itemList.insert(index, internalItem);
                endInsertRows();
                return;
            }
        }
        beginInsertRows(QModelIndex(), 0, 0);
        Twitter::TwitterItem *internalItem = new Twitter::TwitterItem(item);
        internalItem->setParent(this);
        m_itemList.insert(0, internalItem);
        endInsertRows();
    }
}

/*
void TimelineModel::insertItem(int index, Twitter::TwitterItem item, bool ignoreId)
{
    if(index > m_itemList.count() || index < 0) return;
    beginInsertRows(QModelIndex(), index, index);
    Twitter::TwitterItem *internalItem = new Twitter::TwitterItem(item);
    internalItem->setParent(this);
    m_itemList.insert(index, internalItem);
    if(!ignoreId && m_newestId < item.id()) m_newestId = item.id();
    endInsertRows();
}*/

Twitter::TwitterItem TimelineModel::removeItem(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    Twitter::TwitterItem *p = m_itemList.takeAt(index);
    if(!p->read()){
        m_unreadCount--;
        emit unreadCountChanged(m_unreadCount);
    }
    Twitter::TwitterItem rv(*p);
    rv.setParent(NULL);
    delete p;
    endRemoveRows();
    return rv;
}

Twitter::TwitterItem TimelineModel::itemAt(int index) const
{
    Twitter::TwitterItem *p = m_itemList.at(index);
    //Twitter::TwitterItem rv(*p);
    return *p;
}


void TimelineModel::setRead(int index, bool read){
    if(index > m_itemList.count() || index < 0) return;
    Twitter::TwitterItem *item = m_itemList.at(index);
    if(read == item->read()) return;
    item->setRead(read);
    m_unreadCount--;
    emit unreadCountChanged(m_unreadCount);
    QModelIndex idx = this->index(index,0,QModelIndex());
    emit dataChanged(idx, idx);
}

void TimelineModel::setFav(int index, bool fav){
    if(index > m_itemList.count() || index < 0) return;
    Twitter::TwitterItem *item = m_itemList.at(index);
    item->setFav(fav);
    QModelIndex idx = this->index(index,3,QModelIndex());
    emit dataChanged(idx, idx);
}

void TimelineModel::setMyId(quint64 id){
    if(m_myId != id){
        m_myId = id;
        for(int i=0;i<m_itemList.count();i++){
            Twitter::TwitterItem *item = m_itemList.at(i);
            if(item->type() == Twitter::DirectMessage && item->id() == m_myId){
                QModelIndex idx = index(i,0,QModelIndex());
                emit dataChanged(idx, idx);
            }
        }
    }
}

/*
void setData(int index, Returnables::StatusElement *newData)
{
emit(dataChanged(index, index));
}
*/

/*
bool TimelineModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
        if (index.isValid() && role == Qt::EditRole) {
                int row = index.row();

                QPair<QString, QString> p = listOfPairs.value(row);

                if (index.column() == 0)
                        p.first = value.toString();
                else if (index.column() == 1)
                        p.second = value.toString();
        else
            return false;

        listOfPairs.replace(row, p);
                emit(dataChanged(index, index));

        return true;
        }

        return false;
}*/

bool TimelineModel::hasChildren(const QModelIndex &parent) const
{
    if(parent.isValid())
        return false;
    else
        return true;
}

QModelIndex TimelineModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid())
        return createIndex(row, column, m_itemList.at(row));
    else
        return QModelIndex(); //TLモデルでは子供はアイテムを持たないから
}

QModelIndex TimelineModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

Qt::ItemFlags TimelineModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractItemModel::flags(index);// | Qt::ItemIsEditable;
}

void TimelineModel::OnIconDownloaded(quint64 userid, const QIcon &icon)
{
    for(int i=0;i<m_itemList.count();i++){
        Twitter::TwitterItem *item = m_itemList.at(i);
        if(item->userId() == userid){
            QModelIndex idx = index(i,0,QModelIndex());
            emit dataChanged(idx, idx);
        }
    }
}
