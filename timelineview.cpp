#include "timelineview.h"
#include <QtXml>
TimelineView::TimelineView(QWidget *parent) :
    QTreeView(parent)
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setRootIsDecorated(false);
    setAlternatingRowColors(true);
}


QDomElement TimelineView::saveToElement(QDomDocument& doc){
    QDomElement elm = doc.createElement("tab");
    elm.setAttribute("title", title());
    elm.setAttribute("type", type());
    QDomElement header = doc.createElement("header");
    QString peHeaderState(this->header()->saveState().toPercentEncoding());
    header.setAttribute("state", peHeaderState);
    elm.appendChild(header);
    //TODO: �U�蕪���ݒ�̏����o��
    return elm;
}

void TimelineView::restoreFromElement(const QDomElement& element){
    QDomElement header = element.firstChildElement("header");
    QString headerStateStr = header.attribute("state", "");
    if(!headerStateStr.isEmpty()){
        QByteArray arr;
        this->header()->restoreState(arr.fromPercentEncoding(headerStateStr.toAscii()));
    }
    //TODO: �U�蕪���ݒ�̓ǂݍ���
}
