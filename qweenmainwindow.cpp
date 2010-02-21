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

#include "qweenmainwindow.h"
#include "ui_qweenmainwindow.h"
#include "aboutdialog.h"
#include "qweensettings.h"
#include "qweentabctrl.h"
#include "settingdialog.h"
#include <QtCore>
#include <QtGui>
#include "urishortensvc.h"
#include "iconmanager.h"
#include "qweenapplication.h"
#include "timelineview.h"
#include "tabsettingsdialog.h"
#include "forwardruledialog.h"
#include "usersmodel.h"
#include "const.h"

QweenMainWindow::QweenMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QweenMainWindow),m_firstShow(true),m_postAfterShorten(false),m_usersModel(NULL),
    m_completer(NULL), m_urisvc(NULL), m_idAsUInt64(0), m_in_reply_to_status_id(0), m_newestFriendsStatus(0),m_newestRecvDM(0),m_newestSentDM(0),
    m_newestReply(0),m_newestFav(0)
{
    ui->setupUi(this);
    makeWidgets();
    makeConnections();

    setupMenus();

    setAcceptDrops(true);

    settings = QweenSettings::globalSettings();
    //ユーザーIDまたはパスワードが無いので設定ダイアログで入力してもらう
    if(settings->userid().isEmpty() || settings->password().isEmpty())
    {
        SettingDialog dlg(this);
        if(dlg.exec() != QDialog::Accepted ||
           settings->userid().isEmpty() || settings->password().isEmpty())
        {
            exit(-1); //入力されなかったので終了
        }else{
            applySettings();
        }
    }else{
        applySettings();
    }

    restoreGeometry(settings->geometry());
    restoreState(settings->windowState());
    ui->splitter->restoreState(settings->splitterState());

    //TODO: if(outOfScreen()){
        //画面内に戻す
    //}

    //setupWebview

    setupTrayIcon();
    setupTabs();
    setupTimers();
    setupTwitter();
}

QweenMainWindow::~QweenMainWindow()
{
    delete ui;
    m_trayIcon->hide();
    if(m_petrelLib)
        delete m_petrelLib;
}

void QweenMainWindow::makeWidgets(){
    m_petrelLib = new Petrel("","");

    m_timelineTimer = new QTimer(this);
    m_DMTimer = new QTimer(this);
    m_replyTimer = new QTimer(this);
    m_favTimer = new QTimer(this);
    m_fetchAnimTimer = new QTimer(this);

    m_trayIcon = new QSystemTrayIcon(this);

    m_postModeMenu = new QMenu(this);

    tabWidget = new QweenTabCtrl(ui->splitter);
    tabWidget->setTabPosition(QTabWidget::South);
    tabWidget->setFocusPolicy(Qt::ClickFocus);
    ui->splitter->insertWidget(0,tabWidget);
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 0);

    m_usersModel = new UsersModel(QweenApplication::iconManager(), this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSourceModel(m_usersModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->sort(0, Qt::AscendingOrder);

    m_completer = new QCompleter(m_proxyModel, this);
    ui->statusText->setCompleter(m_completer);
}

void QweenMainWindow::applySettings(){
    ui->statusText->setStyleSheet(settings->inputStyle());
    ui->statusText->setRequireCtrlOnEnter(settings->requireCtrlOnEnter());

    setupTimers();
}

bool QweenMainWindow::isNetworkAvailable(){
    //stub
    //TODO: implement
    return true;
}

void QweenMainWindow::save(){
  QFile tabSettings(QweenApplication::profileDir()+"/tabs.xml");
  tabSettings.open(QFile::WriteOnly);
  tabWidget->saveState(&tabSettings);

  settings->setGeometry(saveGeometry());
  settings->setWindowState(saveState());
  settings->setSplitterState(ui->splitter->saveState());
  settings->save();
}

void QweenMainWindow::setupMenus()
{
    //TODO: 実際にTreeView内でCtrl+Cが機能するようにする
    ui->actCopyStot->setText(ui->actCopyStot->text()+"\tCtrl+C");
    ui->actCopyIdUri->setText(ui->actCopyIdUri->text()+"\tCtrl+Shift+C");


    m_actDivideUriFromZenkaku = new QAction(QIcon(), tr("URLからの全角文字列の切り離し"), this);
    m_actDivideUriFromZenkaku->setCheckable(true);
    connect(m_actDivideUriFromZenkaku, SIGNAL(triggered(bool)),
            this, SLOT(OnActDivideUriFromZenkakuToggled(bool)));
    m_postModeMenu->addAction(m_actDivideUriFromZenkaku);

    m_actAvoidApiCommand = new QAction(QIcon(), tr("APIコマンドを回避する"), this);
    m_actAvoidApiCommand->setCheckable(true);
    connect(m_actAvoidApiCommand, SIGNAL(triggered(bool)), this, SLOT(OnActAvoidApiCommandToggled(bool)));
    m_postModeMenu->addAction(m_actAvoidApiCommand);

    m_actAutoShortenUri = new QAction(QIcon(), tr("自動的にURLを短縮する"), this);
    m_actAutoShortenUri->setCheckable(true);
    connect(m_actAutoShortenUri, SIGNAL(triggered(bool)), this, SLOT(OnActAutoShortenUriToggled(bool)));
    m_postModeMenu->addAction(m_actAutoShortenUri);

    m_actReplaceZenkakuSpace = new QAction(QIcon(), tr("全角スペースを半角スペースにする"), this);
    m_actReplaceZenkakuSpace->setCheckable(true);
    connect(m_actReplaceZenkakuSpace, SIGNAL(triggered(bool)), this, SLOT(OnActReplaceZenkakuSpaceToggled(bool)));
    m_postModeMenu->addAction(m_actReplaceZenkakuSpace);

    ui->postButton->setMenu(m_postModeMenu);
}

void QweenMainWindow::setupTabs(){
    QFile tabSettings(QweenApplication::profileDir()+"/tabs.xml");
    if(tabSettings.exists()) tabWidget->restoreState(&tabSettings);
    else tabWidget->restoreState(NULL);
}

void QweenMainWindow::setupTimers(){
    m_timelineTimer->setInterval(settings->tlUpdateIntv()*1000);
    m_timelineTimer->start();

    m_DMTimer->setInterval(settings->dmUpdateIntv()*1000);
    m_DMTimer->start();
    //m_DMTimer->stop();

    m_replyTimer->setInterval(settings->replyUpdateIntv()*1000);
    m_replyTimer->start();
    //m_replyTimer->stop();

    m_favTimer->setInterval(600*1000);
    m_favTimer->start();
    //m_favTimer->stop();

    /*m_fetchAnimTimer->setInterval(85);
    m_fetchAnimTimer->start();*/
}

void QweenMainWindow::setupTrayIcon(){
    m_trayIcon->setIcon(QIcon(":/res/normal.png"));
    setWindowIcon(QIcon(":/res/normal.png"));
    m_trayIcon->setContextMenu(ui->menu_File);
    m_trayIcon->show();
}

void QweenMainWindow::setupTwitter(){
    m_petrelLib->abort();
    //m_twitLib->Logout(); TODO: EndSessionで置き換える
    m_petrelLib->setLoginInfo(settings->userid(), settings->password());
}

void QweenMainWindow::makeConnections(){
    //MainMenu
    connect(ui->actExit, SIGNAL(triggered()),
            this, SLOT(OnExit()));
    //PostMode
    connect(m_postModeMenu, SIGNAL(aboutToShow()),
            this, SLOT(OnPostModeMenuOpen()));
    //StatusText->postButton
    connect(ui->statusText,SIGNAL(returnPressed()),
            ui->postButton,SLOT(click()));
    //TrayIcon
    connect(m_trayIcon, SIGNAL(messageClicked()), this, SLOT(OnMessageClicked()));
    connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(OnIconActivated(QSystemTrayIcon::ActivationReason)));

    //Timers
    connect(m_timelineTimer, SIGNAL(timeout()), this, SLOT(OnTimelineTimerTimeout()));
    connect(m_DMTimer, SIGNAL(timeout()), this, SLOT(OnDmTimerTimeout()));
    connect(m_replyTimer, SIGNAL(timeout()), this, SLOT(OnReplyTimerTimeout()));
    connect(m_favTimer, SIGNAL(timeout()), this, SLOT(OnFavTimerTimeout()));

    //Twitter
    connect(m_petrelLib, SIGNAL(homeTimelineReceived(statuses_t&)),
            this, SLOT(OnHomeTimelineReceived(statuses_t&)));
    connect(m_petrelLib, SIGNAL(verifyCredentialsReceived(user_t&)),
            this, SLOT(OnVerifyCredentialsReceived(user_t&)));
    connect(m_petrelLib, SIGNAL(sentDirectMessagesReceived(direct_messages_t&)),
            this, SLOT(OnSentDirectMessagesReceived(direct_messages_t&)));
    connect(m_petrelLib, SIGNAL(directMessagesReceived(direct_messages_t&)),
            this, SLOT(OnDirectMessagesReceived(direct_messages_t&)));
    connect(m_petrelLib, SIGNAL(updateReceived(status_t&)),
            this, SLOT(OnUpdateReceived(status_t&)));
    connect(m_petrelLib, SIGNAL(rateLimitStatusReceived(hash_t&)),
            this, SLOT(OnRateLimitStatusReceived(hash_t&)));
    connect(m_petrelLib, SIGNAL(mentionsReceived(statuses_t&)),
            this, SLOT(OnMentionsReceived(statuses_t&)));
    connect(m_petrelLib, SIGNAL(userTimelineReceived(statuses_t&)),
            this, SLOT(OnUserTimelineReceived(statuses_t&)));
    connect(m_petrelLib, SIGNAL(existsFriendshipsReceived(friends_t&)),
            this, SLOT(OnExistsFriendshipsReceived(friends_t&)));
    connect(m_petrelLib, SIGNAL(showUsersReceived(user_t&)),
            this, SLOT(OnShowUserDetailsReceived(user_t&)));
    connect(m_petrelLib, SIGNAL(createFriendshipReceived(user_t&)),
            this, SLOT(OnCreateFriendshipReceived(user_t&)));
    connect(m_petrelLib, SIGNAL(destroyFriendshipReceived(user_t&)),
            this, SLOT(OnDestroyFriendshipReceived(user_t&)));

    connect(m_petrelLib, SIGNAL(error(int,QDomElement)),
            this, SLOT(OnError(int,QDomElement)));

    //Tab
    connect(tabWidget, SIGNAL(itemSelected(Twitter::TwitterItem)),
            this, SLOT(OnItemSelected(Twitter::TwitterItem)));
    //StatusText
    connect(ui->statusText, SIGNAL(uriShorteningFinished()),
            this, SLOT(OnUriShorteningFinished()));
}

void QweenMainWindow::OnExit()
{
    save();
    QweenApplication::exit();
}

void QweenMainWindow::OnHomeTimelineReceived(statuses_t& s){
        QString popupText;
        QString title(tr("新着 ") + QString::number(s.status.count()) + tr("件\n"));
        foreach(QSharedPointer<status_t> ptr, s.status){
            Twitter::TwitterItem item(Twitter::Status, ptr, HOME_TIMELINE, false);
            if(m_newestFriendsStatus < item.id()) m_newestFriendsStatus = item.id();
            popupText.append(QString("%1 : %2\n").arg(item.userName(), item.status()));
            if(!m_usersModel->userExists(item.userId()))
                m_usersModel->appendItem(item);
            tabWidget->addItem(item);
        }
        //TODO: dm, reply, sound
        //バルーン・サウンドは最初は抑制するようだ
        //設定項目があるのでそこを見るべし
        m_trayIcon->showMessage(title, popupText, QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information),
                                5 * 1000);
}

void QweenMainWindow::OnVerifyCredentialsReceived(user_t& user){
    if(user.id!=0){
        tabWidget->setMyId(user.id);
        m_idAsUInt64 = user.id;
        m_petrelLib->homeTimeline(m_newestFriendsStatus,0,20,0);
        OnDmTimerTimeout();
        OnReplyTimerTimeout();
        OnFavTimerTimeout();
    }
}

void QweenMainWindow::OnSentDirectMessagesReceived(direct_messages_t& direct_messages){
    foreach(QSharedPointer<direct_message_t> ptr, direct_messages.direct_message){
        Twitter::TwitterItem item(Twitter::DirectMessage, ptr, SENT_DIRECT_MESSAGES, false);
        if(m_newestSentDM < item.id()) m_newestSentDM = item.id();
        tabWidget->addItem(item);
    }
}

void QweenMainWindow::OnDirectMessagesReceived(direct_messages_t& direct_messages){
    foreach(QSharedPointer<direct_message_t> ptr, direct_messages.direct_message){
        Twitter::TwitterItem item(Twitter::DirectMessage, ptr, DIRECT_MESSAGES, false);
        if(m_newestRecvDM < item.id()) m_newestRecvDM = item.id();
        if(!m_usersModel->userExists(item.userId()))
            m_usersModel->appendItem(item);
        tabWidget->addItem(item);
    }
}

void QweenMainWindow::OnUpdateReceived(status_t& status){
    ui->statusText->setText("");
    ui->statusText->setEnabled(true);
    ui->postButton->setEnabled(true);
    m_in_reply_to_status_id = 0;
    QSharedPointer<status_t> s(new status_t(status));
    tabWidget->addItem(Twitter::TwitterItem(Twitter::Status, s, UPDATE, false));
}


void QweenMainWindow::OnRateLimitStatusReceived(hash_t& hash){
    QMessageBox::information(this,tr("API情報"),
                             tr("上限: %1\n残数: %2\nリセット日時: %3\n")
                             .arg(hash.hourly_limit,
                                  hash.remaining_hits,
                                  hash.reset_time));
}


void QweenMainWindow::OnMentionsReceived(statuses_t& s){
    foreach(QSharedPointer<status_t> ptr, s.status){
        Twitter::TwitterItem item(Twitter::Status, ptr, MENTIONS, false);
        if(m_newestReply < item.id()) m_newestReply = item.id();
        if(!m_usersModel->userExists(item.userId()))
            m_usersModel->appendItem(item);
        tabWidget->addItem(item);
    }
}

void QweenMainWindow::OnFavoritesReceived(statuses_t& s){
    foreach(QSharedPointer<status_t> ptr, s.status){
        Twitter::TwitterItem item(Twitter::Status, ptr, MENTIONS, false);
        if(m_newestFav < item.id()) m_newestFav = item.id();
        if(!m_usersModel->userExists(item.userId()))
            m_usersModel->appendItem(item);
        tabWidget->addItem(item);
    }
}

void QweenMainWindow::OnUserTimelineReceived(statuses_t& s){
    QSharedPointer<status_t> st = s.status.takeFirst();
    if(!st.isNull())
        QMessageBox::information(this,tr("@twj の最新のTweet"),st->text);
}

void QweenMainWindow::OnExistsFriendshipsReceived(friends_t& friends){
    if(friends.value)
        QMessageBox::information(this, tr("友達関係"),
                                 tr("相互にフォローしています。")); //TODO: existsじゃなくてshowを使う
}

void QweenMainWindow::OnShowUserDetailsReceived(user_t& user){
    QMessageBox::information(this, tr("プロファイル情報"),
                             tr("Following : %1\n"
                                "Followers : %2\n"
                                "Statuses count : %3\n"
                                "Location : %4\n"
                                "Bio : %5")
                             .arg(QString::number(user.friends_count),
                                  QString::number(user.followers_count),
                                  QString::number(user.statuses_count),
                                  user.location,
                                  user.description));
}

void QweenMainWindow::OnCreateFriendshipReceived(user_t& user){
    if(!user.screen_name.isEmpty())
        QMessageBox::information(this, "Follow", tr("@%1 をFollow開始しました。").arg(user.screen_name));
}

void QweenMainWindow::OnDestroyFriendshipReceived(user_t& user){
    if(!user.screen_name.isEmpty())
        QMessageBox::information(this, "Remove", tr("@%1 をRemoveしました。").arg(user.screen_name));
}

void QweenMainWindow::OnError(int role, QDomElement elm){
    switch(role){
    case UPDATE:
      {
        ui->statusText->setEnabled(true);
        ui->postButton->setEnabled(true);
        break;
      }
    default:
        break;
    }
}

void QweenMainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void QweenMainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    if (settings->minimizeOnClose()) {
         hide();
         event->ignore();
    }else{
        save();
  }
}

void QweenMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain"))
        event->acceptProposedAction();
}

void QweenMainWindow::dropEvent(QDropEvent *event)
{
    ui->statusText->setText(ui->statusText->text() + event->mimeData()->text());
    event->acceptProposedAction();
}

void QweenMainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if(isNetworkAvailable() && m_firstShow){
        m_petrelLib->verifyCredentials();
        m_firstShow = false;
    //TODO: version check
    /*
    if(settings->checkVersionOnStartup()){
        checkNewVersion();
    }
    */
    }
}

void QweenMainWindow::on_actOptions_triggered()
{
    SettingDialog dlg(this);
    //bool chgUseApi = false;
    if(dlg.exec() == QDialog::Accepted){
        applySettings();

        if(dlg.loginInfoChanged()){
            setupTwitter();
            m_petrelLib->verifyCredentials();
        }
    }
}

void QweenMainWindow::on_actAboutQt_triggered()
{
    QMessageBox::aboutQt(this);
}

void QweenMainWindow::on_actAboutQween_triggered()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void QweenMainWindow::doPost(){
    QString postText = ui->statusText->text().trimmed();
    if(postText.isEmpty()){
        //TODO: refresh();
        return;
    }
    ui->statusText->setEnabled(false);
    ui->postButton->setEnabled(false);
    m_petrelLib->update(postText + tr(" ") + settings->statusSuffix(),m_in_reply_to_status_id,"",""); // TODO:クライアント名"Qween"を付加 OAuth対応後
}

void QweenMainWindow::makeReplyOrDirectStatus(bool isAuto, bool isReply, bool isAll){
    Q_UNUSED(isAuto)
    Q_UNUSED(isReply)
    Q_UNUSED(isAll)
}

void QweenMainWindow::OnUriShorteningFinished(){
    if(m_postAfterShorten){
        m_postAfterShorten = false;
        doPost();
    }
}

void QweenMainWindow::on_postButton_clicked()
{
    if(settings->uriAutoShorten()){
        m_postAfterShorten = true;
        ui->statusText->shortenUri();
    }else{
        doPost();
    }

}

void QweenMainWindow::OnTimelineTimerTimeout()
{
    m_petrelLib->homeTimeline(m_newestFriendsStatus,0,200,0);
}


void QweenMainWindow::OnDmTimerTimeout(){
    m_petrelLib->sentDirectMessages(m_newestSentDM,0,0,1);
    m_petrelLib->directMessages(m_newestRecvDM,0,0,1);
}

void QweenMainWindow::OnReplyTimerTimeout(){
    m_petrelLib->mentions(m_newestReply,0,0,0);
}

void QweenMainWindow::OnFavTimerTimeout(){
    m_petrelLib->favorites(0,0);
}

void QweenMainWindow::OnItemSelected(const Twitter::TwitterItem &item)
{
    switch(item.type()){
    case Twitter::Status:
    {
        QString status(item.status());
        QRegExp linkrx(LINK_RX_DATA);
        int pos=0;
        while ((pos = linkrx.indexIn(status, pos)) != -1) {
            QStringList list = linkrx.capturedTexts();
            QString str;
            QString anchor;
            int length;
            if (list[1] != ""){ //hashtag
                str = list[1];
                if (str.at(0) != '#'){
                    str.remove(0,1);
                    pos++;
                }
                QString str2 = str;
                str2.remove(0,1);
                QUrl url("http://twitter/");
                url.setFragment("search?q=%23"+str2);
                anchor = QString("<a href=\"%2\">%1</a>")
                         .arg(str,url.toString());
                length = str.length();
            }
            else if(list[2] != ""){ //reply
                str = list[2];
                if (str.at(0) == '@'){
                    str.remove(0,1);
                    pos++;
                }else{
                    str.remove(0,2);
                    pos+=2;
                }
                anchor = QString("<a href=\"http://twitter.com/%1\">%1</a>").arg(str);
                length = str.length();
            }
            else if(list[3] != ""){ //URI
                str = list[3];
                anchor = QString("<a href=\"%1\">%1</a>").arg(str);
                length = str.length();
            }
            status.replace(pos, length, anchor);
            pos += anchor.length();
        }
        ui->textBrowser->setHtml(tr("<html><body style=\"%1\">")
                                 .arg(settings->statusViewStyle()) +
                                 status + tr("</body></html>"));
        ui->lblNameId->setText(item.screenName() + "/" + item.userName());
        ui->lblUpdateDatetime->setText(item.createdAt());
        if(QweenApplication::iconManager()->isIconAvailable(item.userId())){
            QIcon icon(QweenApplication::iconManager()->getIcon(item.userId()));
            ui->userIconLabel->setPixmap(icon.pixmap(50,50,QIcon::Normal,QIcon::On));
        }else{
            ui->userIconLabel->setPixmap(QPixmap(50,50));
            QweenApplication::iconManager()->fetchIcon(item.userId(), item.iconUri());
        }
        ui->userIconLabel->repaint();
        break;
    }
    default:
        break;
    }
}

void QweenMainWindow::OnPostModeMenuOpen(){
    m_actAutoShortenUri->setChecked(settings->uriAutoShorten());
    m_actAvoidApiCommand->setChecked(settings->avoidApiCmd());
    m_actDivideUriFromZenkaku->setChecked(settings->divideUriFromZenkaku());
    m_actReplaceZenkakuSpace->setChecked(settings->replaceZenkakuSpace());
}

void QweenMainWindow::OnUriShortened(const QString& src, const QString& dest){
    Q_UNUSED(src)
    QMessageBox::information(this, "", dest);
}

void QweenMainWindow::OnIconDownloaded(quint64 userid, const QIcon &icon){
    Q_UNUSED(userid)
    ui->userIconLabel->setPixmap(icon.pixmap(50,50,QIcon::Normal,QIcon::On));
}

void QweenMainWindow::OnMessageClicked(){
    //FIXME: X11環境だと動かないことがある？
    this->raise();
    this->activateWindow();
}

void QweenMainWindow::OnIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  switch (reason) {
     case QSystemTrayIcon::Trigger:
     case QSystemTrayIcon::DoubleClick:
        if(!this->isVisible())
            this->show();
         break;
     case QSystemTrayIcon::MiddleClick:
         break;
     default:
         ;
  }
}

void QweenMainWindow::on_statusText_textChanged(QString string)
{
    Q_UNUSED(string)
    int rest = getRestStatusCount(ui->statusText->text().trimmed());
    ui->lblStatusLength->setText(QString("%1").arg(rest));
    if(rest < 0){ 
        ui->statusText->setStyleSheet(settings->inputStyle()+" *{color:rgb(255,0,0);}");
    }else{
        ui->statusText->setStyleSheet(settings->inputStyle());
    }
}

int QweenMainWindow::getRestStatusCount(const QString &str, bool footer)
{
    int rv = 140 - str.length();
    if(footer)
        rv -= settings->statusSuffix().length()+1;
    if(settings->avoidApiCmd()){

    }

    if(settings->replaceZenkakuSpace()){

    }

    if(settings->divideUriFromZenkaku()){

    }

    //TODO: フッタ機能と連動
    //TODO: Shiftキー
    //詳しくはTweenのソースを検索 GetRestStatusCount
    return rv;
}

void QweenMainWindow::on_actShowUserStatus_triggered()
{
    m_petrelLib->showUsers(m_idAsUInt64,0,"");
}

void QweenMainWindow::on_actApiInfo_triggered()
{
    m_petrelLib->rateLimitStatus();
}

void QweenMainWindow::on_actQweenHomepage_triggered()
{
    //TODO: ブラウザを設定できるようにする
    QDesktopServices::openUrl(QUrl("http://qween.tnose.net/"));
}

void QweenMainWindow::on_actionTest_bitly_triggered()
{
    if(m_urisvc) delete m_urisvc;
    QInputDialog dlg(this);
    dlg.setOption(QInputDialog::UseListViewForComboBoxItems, true);
    dlg.setComboBoxEditable(false);
    QStringList items;
    items << tr("bitly") << tr("tinyurl") << tr("twurl") << tr("isgd") << tr("unu");
    dlg.setComboBoxItems(items);
    dlg.setWindowTitle("test uri shortener");
    dlg.setLabelText("Services:");
    if (dlg.exec()==QDialog::Accepted && !dlg.textValue().isEmpty()){
        m_urisvc = getUriShortenService(dlg.textValue(), this);
        if(m_urisvc){
            connect(m_urisvc, SIGNAL(uriShortened(QString,QString)),
                    this, SLOT(OnUriShortened(QString,QString)));
            m_urisvc->shortenAsync("http://is2008.is-a-geek.org/");
        }
    }
}

void QweenMainWindow::on_statusText_returnPressed()
{
    //TODO: 複数行対応？やりたくねー
}

void QweenMainWindow::on_actionTest_iconmanager_triggered()
{
    QMessageBox::information(this, "", QString("%1").arg(255,0,16));
}

void QweenMainWindow::on_actionTest_icon_triggered()
{
    connect(QweenApplication::iconManager(),SIGNAL(iconDownloaded(quint64,QIcon)),
            this,SLOT(OnIconDownloaded(quint64,QIcon)));
    QweenApplication::iconManager()->fetchIcon(1261519751, "http://a1.twimg.com/profile_images/525002820/CIMG0272_bigger.JPG");
}

void QweenMainWindow::on_actUpdate_triggered()
{
    m_petrelLib->homeTimeline(m_newestFriendsStatus,0,20,0);
}

void QweenMainWindow::on_actCopyStot_triggered()
{
    //TODO: 複数選択可能にする
    Twitter::TwitterItem item = tabWidget->currentItem();
    if(item.type()==Twitter::Undefined) return;
    //TODO: Protectedならコピーしない設定を追加
    //TODO: ReTweet対応
    QString data = "%0:%1 [http://twitter.com/%0/status/%2]";
    QString dataRT = "%0:%1 [http://twitter.com/%2/status/%3]";
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(data.arg(item.screenName(), item.status(), QString::number(item.id())), QClipboard::Clipboard);
}

void QweenMainWindow::on_actCopyIdUri_triggered()
{
    //TODO: 複数選択可能にする
    Twitter::TwitterItem item = tabWidget->currentItem();
    if(item.type()==Twitter::Undefined) return;
    //TODO: Protectedならコピーしない設定を追加
    //TODO: ReTweet対応
    QString data = "http://twitter.com/%0/status/%1";
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(data.arg(item.screenName(), QString::number(item.id())), QClipboard::Clipboard);
}

void QweenMainWindow::on_actExplosion_triggered()
{
    QMessageBox::information(this, tr("ゴルァ"), tr("だからやめれっての"));
}

void QweenMainWindow::on_actShortenUri_triggered()
{
    ui->statusText->shortenUri();
}

void QweenMainWindow::OnActDivideUriFromZenkakuToggled(bool val){
    settings->setDivideUriFromZenkaku(val);
}

void QweenMainWindow::OnActAvoidApiCommandToggled(bool val){
    settings->setAvoidApiCmd(val);
}

void QweenMainWindow::OnActAutoShortenUriToggled(bool val){
    settings->setUriAutoShorten(val);
}

void QweenMainWindow::OnActReplaceZenkakuSpaceToggled(bool val){
    settings->setReplaceZenkakuSpace(val);
}

void QweenMainWindow::on_actShowFriendships_triggered()
{
    QString name = tabWidget->currentItem().screenName();
    bool ok;
    QString rv = QInputDialog::getText(this, tr("フォロー関係を調べる"), tr("IDを入力してください"), QLineEdit::Normal, name, &ok);
    if(ok){
        m_petrelLib->existsFriendships(settings->userid(), rv);
    }
}

void QweenMainWindow::on_actFollow_triggered()
{
    QString name = tabWidget->currentItem().screenName();
    bool ok;
    QString rv = QInputDialog::getText(this, tr("Follow"), tr("IDを入力してください"), QLineEdit::Normal, name, &ok);
    if(ok){
        m_petrelLib->createFriendship(0,0,rv,false);
    }
}

void QweenMainWindow::on_actRemove_triggered()
{
    QString name = tabWidget->currentItem().screenName();
    bool ok;
    QString rv = QInputDialog::getText(this, tr("Follow"), tr("IDを入力してください"), QLineEdit::Normal, name, &ok);
    if(ok){
        m_petrelLib->destroyFriendship(0,0,rv);
    }
}

void QweenMainWindow::on_actCreateTab_triggered()
{
    QString name = QString("NewTab%1").arg(tabWidget->count());
    bool ok;
    QString rv = QInputDialog::getText(this, tr("新規タブ"), tr("タブ名を入力してください"), QLineEdit::Normal, name, &ok);
    if(ok){
        tabWidget->addTimelineView(rv);
    }
}

void QweenMainWindow::on_actRenameTab_triggered()
{
    TimelineView *view = tabWidget->currentTimelineView();
    bool ok;
    QString rv = QInputDialog::getText(this, tr("名前変更"), tr("名前を入力してください"), QLineEdit::Normal, view->title(), &ok);
    if(ok){
        view->setTitle(rv);
        tabWidget->setTabText(tabWidget->indexOf(view), rv);
    }
}

void QweenMainWindow::on_actTwitterNews_triggered()
{
    m_petrelLib->userTimeline(0,0,"twj",0,0,0,0);
}

void QweenMainWindow::on_actTabSettings_triggered()
{
    TabSettingsDialog dlg(tabWidget);
    dlg.exec();
}

void QweenMainWindow::on_actAtReply_triggered()
{
    //stub.
    //TODO: なにやら複雑な処理
    ui->statusText->setCursorPosition(0);
    ui->statusText->insert("@"+tabWidget->currentItem().screenName()+" ");
    m_in_reply_to_status_id = tabWidget->currentItem().id();
}

void QweenMainWindow::on_actSendDM_triggered()
{
    //stub.
    ui->statusText->setCursorPosition(0);
    ui->statusText->insert("D "+tabWidget->currentItem().screenName()+" ");
}
