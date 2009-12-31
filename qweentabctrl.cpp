#include <QTabWidget>
#include <QListWidget>
#include <QTableView>
#include <QHeaderView>
#include "qweentabctrl.h"
#include "qweenapplication.h"
#include "tabinfo.h"
#include "timelinemodel.h"
#include "timelineview.h"
#include "timelineitemdelegate.h"

//XXX: ���񂾂񉘂��Ȃ��Ă����c

QweenTabCtrl::QweenTabCtrl(QWidget *parent) :
    QTabWidget(parent),m_homeView(NULL),m_replyView(NULL),
    m_dmView(NULL),m_favView(NULL)
{
    setMovable(true);
}

void QweenTabCtrl::saveState(QIODevice* device)
{
    const int IndentSize = 2;
    QTextStream out(device);
    QDomDocument doc;
    QDomElement root = doc.createElement("tabs");
    doc.appendChild(root);
    for(int i=0;i<this->count();i++){
        TimelineView *view = (TimelineView*)(this->widget(i));
        QDomElement elm = view->saveToElement(doc);
        root.appendChild(elm);
    }
    doc.save(out, IndentSize);
}

void QweenTabCtrl::restoreState(QIODevice* device){
    if(!device){
        fixLackingTabs();
        return;
    }
    QDomDocument doc;
    QString errorStr;
    int errorLine, errorColumn;
    if (!doc.setContent(device, true, &errorStr, &errorLine,
                                &errorColumn)) {
        qDebug() << tr("Parse error at line %1, column %2: %3").arg(errorLine).arg(errorColumn).arg(errorStr);
        fixLackingTabs();
        return;
    }
    QDomElement root = doc.documentElement();
    if (root.tagName() != "tabs") {
        qDebug() << tr("invalid root element");
        fixLackingTabs();
        return;
    }
    QDomElement tabelm = root.firstChildElement("tab");
    while (!tabelm.isNull()) {
        //XXX: �U�蕪���ݒ�Ƃ��̃f�[�^��View�������Ă�̂��C��������
        QString type = tabelm.attribute("type", "userdefined");
        QString title = tabelm.attribute("title", "Untitled");
        TimelineView* view = new TimelineView(this);
        if(type == "home"){
            if(!m_homeView) m_homeView = view;
            else type = "userdefined";
        }else if(type == "reply"){
            if(!m_replyView) m_replyView = view;
            else type = "userdefined";
        }else if(type == "dm"){
            if(!m_dmView) m_dmView = view;
            else type = "userdefined";
        }else if(type == "fav"){
            if(!m_favView) m_favView = view;
            else type = "userdefined";
        }
        view->setType(type);
        view->setTitle(title);
        view->restoreFromElement(tabelm);
        view->setItemDelegate(new TimelineItemDelegate(QweenApplication::iconManager()));
        view->setModel(new TimelineModel(QweenApplication::iconManager(),this));
        connect(view, SIGNAL(itemSelected(Twitter::TwitterItem)),
                this, SLOT(OnItemSelected(Twitter::TwitterItem)));
        this->addTab(view, QIcon(), view->title());
        tabelm = tabelm.nextSiblingElement("tab");
    }
    fixLackingTabs();
}


void QweenTabCtrl::fixLackingTabs(){
    TimelineView* view;
    //XXX: �R�s�y�o��
    if(!m_homeView){
        view = new TimelineView(this);
        view->setType("home");
        view->setTitle("Home");
        view->setItemDelegate(new TimelineItemDelegate(QweenApplication::iconManager()));
        view->setModel(new TimelineModel(QweenApplication::iconManager(),this));
        view->header()->resizeSection(1, 400);
        connect(view, SIGNAL(itemSelected(Twitter::TwitterItem)),
                this, SLOT(OnItemSelected(Twitter::TwitterItem)));
        this->addTab(view,QIcon(),view->title());
        m_homeView = view;
    }

    if(!m_replyView){
        view = new TimelineView(this);
        view->setType("reply");
        view->setTitle("Reply");
        view->setItemDelegate(new TimelineItemDelegate(QweenApplication::iconManager()));
        view->setModel(new TimelineModel(QweenApplication::iconManager(),this));
        view->header()->resizeSection(1, 400);
        connect(view, SIGNAL(itemSelected(Twitter::TwitterItem)),
                this, SLOT(OnItemSelected(Twitter::TwitterItem)));
        this->addTab(view,QIcon(),view->title());
        m_replyView = view;
    }

    if(!m_dmView){
        view = new TimelineView(this);
        view->setType("dm");
        view->setTitle("DM");
        view->setItemDelegate(new TimelineItemDelegate(QweenApplication::iconManager()));
        view->setModel(new TimelineModel(QweenApplication::iconManager(),this));
        view->header()->resizeSection(1, 400);
        connect(view, SIGNAL(itemSelected(Twitter::TwitterItem)),
                this, SLOT(OnItemSelected(Twitter::TwitterItem)));
        this->addTab(view,QIcon(),view->title());
        m_dmView = view;
    }

    if(!m_favView){
        view = new TimelineView(this);
        view->setType("fav");
        view->setTitle("Favorites");
        view->setItemDelegate(new TimelineItemDelegate(QweenApplication::iconManager()));
        view->setModel(new TimelineModel(QweenApplication::iconManager(),this));
        view->header()->resizeSection(1, 400);
        connect(view, SIGNAL(itemSelected(Twitter::TwitterItem)),
                this, SLOT(OnItemSelected(Twitter::TwitterItem)));
        this->addTab(view,QIcon(),view->title());
        m_favView = view;
    }
}

void QweenTabCtrl::addItem(Twitter::TwitterItem item){
    switch(item.origin()){
    case Returnables::FRIENDS_TIMELINE:
        if(m_homeView) m_homeView->model()->appendItem(item);
        break;
    case Returnables::RECENT_MENTIONS:
        if(m_replyView) m_replyView->model()->appendItem(item);
        break;
    case Returnables::RECEIVED_DIRECT_MESSAGES:
        if(m_dmView) m_dmView->model()->appendItem(item);
        break;
    case Returnables::SENT_DIRECT_MESSAGES:
        if(m_dmView) m_dmView->model()->appendItem(item);
        break;
    case Returnables::FAVORITES:
        if(m_favView) m_favView->model()->appendItem(item);
        break;
    case Returnables::NEW_STATUS:
        if(m_homeView) m_homeView->model()->appendItem(item);
        break;
    default:
        break;
    }
}

Twitter::TwitterItem QweenTabCtrl::currentItem(){
    return m_curItem;
}

TimelineView *QweenTabCtrl::currentTimelineView(){
    return (TimelineView*)this->currentWidget();
}

TimelineView *QweenTabCtrl::timelineView(int index){
    return (TimelineView*)this->widget(index);
}

TimelineView *QweenTabCtrl::addTimelineView(const QString &title){
    return insertTimelineView(count(), title);
}

TimelineView *QweenTabCtrl::insertTimelineView(int index, const QString& title){
    TimelineView* view = new TimelineView(this);
    view->setType("userdefined");
    view->setTitle(title);
    view->setItemDelegate(new TimelineItemDelegate(QweenApplication::iconManager()));
    view->setModel(new TimelineModel(QweenApplication::iconManager(),this));
    view->header()->resizeSection(1, 400);
    connect(view, SIGNAL(itemSelected(Twitter::TwitterItem)),
            this, SLOT(OnItemSelected(Twitter::TwitterItem)));
    this->insertTab(index, view, QIcon(), view->title());
    return view;
}

void QweenTabCtrl::removeTimelineView(int index){
    TimelineView *view = timelineView(index);
    if(view == m_homeView || view == m_dmView || view == m_favView || view || m_replyView) return;
    delete view;
}

void QweenTabCtrl::moveTimelineView(int before, int after){
    TimelineView *view = timelineView(before);
    this->removeTab(before);
    if(before > after){
        this->insertTab(after - 1, view, QIcon(), view->title());
    }else{
        this->insertTab(after, view, QIcon(), view->title());
    }
}

void QweenTabCtrl::setMyId(quint64 myid){
    //TODO: �eModel��MyID���Z�b�g
    m_myID = myid;
}

quint64 QweenTabCtrl::getNewestHomeId() const {
    if(!m_homeView) return 0;
    return m_homeView->model()->newestId();
}

quint64 QweenTabCtrl::getNewestDMId() const {
    if(!m_dmView) return 0;
    return m_dmView->model()->newestId();
}

quint64 QweenTabCtrl::getNewestReplyId() const{
    if(!m_replyView) return 0;
    return m_replyView->model()->newestId();
}

quint64 QweenTabCtrl::getNewestFavId() const{
    if(!m_favView) return 0;
    return m_favView->model()->newestId();
}

void QweenTabCtrl::OnItemSelected(const Twitter::TwitterItem& item){
    m_curItem = item;
    emit itemSelected(item);
}
