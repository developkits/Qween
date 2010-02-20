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

#ifndef TWITTER_H
#define TWITTER_H

#include <QObject>
#include "petrel/petrel.h"
class TimelineModel;
namespace Twitter{
    enum ItemType
    {
        Undefined, Status, DirectMessage, BasicUserInfo, ExtUserInfo
    };

    class TwitterItem{
        //Q_OBJECT
    public:
        TwitterItem(TimelineModel *parent = 0)
            :m_type(Undefined),m_read(false),m_parent(parent)
        {}

        TwitterItem(ItemType type, QSharedPointer<status_t> ptr, enum ROLE_TYPE origin,
                    bool read, TimelineModel *parent = 0)
            :m_type(type), m_statusPtr(ptr), m_origin(origin), m_read(read),m_parent(parent)
        {}

        TwitterItem(ItemType type, QSharedPointer<direct_message_t> ptr, enum ROLE_TYPE origin,
                    bool read, TimelineModel *parent = 0)
            :m_type(type), m_dmPtr(ptr), m_origin(origin), m_read(read),m_parent(parent)
        {}

        TwitterItem(ItemType type, QSharedPointer<user_t> ptr, enum ROLE_TYPE origin,
                    bool read, TimelineModel *parent = 0)
            :m_type(type), m_userPtr(ptr), m_origin(origin), m_read(read),m_parent(parent)
        {}

        QString userName() const {
            switch(m_type){
            case Twitter::Status:
                return m_statusPtr->user->name;
                break;
            case Twitter::DirectMessage:
                return m_dmPtr->sender->name;
                break;
            case Twitter::BasicUserInfo:
                return m_userPtr->name;
                break;
            default:
                return QString();
            }
        }

        QString screenName() const {
            switch(m_type){
            case Twitter::Status:
                return m_statusPtr->user->screen_name;
                break;
            case Twitter::DirectMessage:
                return m_dmPtr->sender->screen_name;
                break;
            case Twitter::BasicUserInfo:
                return m_userPtr->screen_name;
                break;
            default:
                return QString();
            }
        }

        QString createdAt() const {
            switch(m_type){
            case Twitter::Status:
                return m_statusPtr->created_at.toString();
                break;
            case Twitter::DirectMessage:
                return m_dmPtr->created_at.toString();
                break;
            case Twitter::BasicUserInfo:
                return m_userPtr->status->created_at.toString();
                break;
            default:
                return QString();
            }
        }

        QString status() const {
            switch(m_type){
            case Twitter::Status:
                return m_statusPtr->text;
                break;
            case Twitter::DirectMessage:
                return m_dmPtr->text;
                break;
            case Twitter::BasicUserInfo:
                return m_userPtr->status->text;
                break;
            default:
                return QString();
            }
        }

        quint64 id()const {
            switch(m_type){
            case Twitter::Status:
                return m_statusPtr->id;
                break;
            case Twitter::DirectMessage:
                return m_dmPtr->id;
                break;
            case Twitter::BasicUserInfo:
                return m_userPtr->status->id;
                break;
            default:
                return 0;
            }
        }

        quint64 userId() const {
            switch(m_type){
            case Twitter::Status:
                return m_statusPtr->user->id;
                break;
            case Twitter::DirectMessage:
                return m_dmPtr->sender->id;
                break;
            case Twitter::BasicUserInfo:
                return m_userPtr->id;
                break;
            default:
                return 0;
            }
        }

        QString iconUri() const {
            switch(m_type){
            case Twitter::Status:
                return m_statusPtr->user->profile_image_url.toString();
                break;
            case Twitter::DirectMessage:
                return m_dmPtr->sender->profile_image_url.toString();
                break;
            case Twitter::BasicUserInfo:
                return m_userPtr->profile_image_url.toString();
                break;
            default:
                return 0;
            }
        }
        bool favorited() const {
            switch(m_type){
            case Twitter::Status:
                return m_statusPtr->favorited;
                break;
            case Twitter::DirectMessage:
                return false;
                break;
            case Twitter::BasicUserInfo:
                return false;
                break;
            default:
                return false;
            }
        }
        TimelineModel *parent() const { return m_parent; }
        void setParent(TimelineModel *parent){ m_parent = parent; }
        enum ItemType type() const { return m_type; }
        enum ROLE_TYPE origin() const { return m_origin; }
        bool read() const { return m_read; }
        void setRead(bool val = true) { m_read = val; }
        void setFav(bool val = true) {
            switch(m_type){
            case Twitter::Status:
                m_statusPtr->favorited = val;
                break;
            case Twitter::DirectMessage:
                return;
                break;
            default:
                return;
            }
        }
    private:
        enum ItemType m_type;
        QSharedPointer<status_t> m_statusPtr;
        QSharedPointer<direct_message_t> m_dmPtr;
        QSharedPointer<user_t> m_userPtr;
        enum ROLE_TYPE m_origin;
        bool m_read;
        TimelineModel *m_parent;
    };
};


#endif // TWITTER_H
