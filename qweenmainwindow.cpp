#include "qweenmainwindow.h"
#include "ui_qweenmainwindow.h"
#include "aboutdialog.h"
#include "qweensettings.h"
#include "qweentabctrl.h"
#include "settingdialog.h"
#include "QTwitLib.h"
#include <QDialog>
#include <QMessageBox>
#include <QtGui>
//#include <QtWebKit>
#include "urishortensvc.h"
#include "iconmanager.h"
#include "qweenapplication.h"

QweenMainWindow::QweenMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QweenMainWindow),m_firstShow(true),m_postAfterShorten(false),m_urisvc(NULL),
    m_newestFriendsStatus(0),m_newestRecvDM(0),m_newestSentDM(0),m_newestReply(0),m_newestFav(0)
{
    ui->setupUi(this);
    makeWidgets();
    makeConnections();

    setupMenus();

    setAcceptDrops(true);

    settings = QweenSettings::globalSettings();
    //���[�U�[ID�܂��̓p�X���[�h�������̂Őݒ�_�C�A���O�œ��͂��Ă��炤
    if(settings->userid().isEmpty() || settings->password().isEmpty())
    {
        SettingDialog dlg(this);
        if(dlg.exec() != QDialog::Accepted ||
           settings->userid().isEmpty() || settings->password().isEmpty())
        {
            exit(-1); //���͂���Ȃ������̂ŏI��
        }else{
            applySettings();
        }
    }else{
        applySettings();
    }

    restoreGeometry(settings->geometry());
    restoreState(settings->windowState());
    //TODO: splitter�𕜌�
    //TODO: if(outOfScreen()){
        //��ʓ��ɖ߂�
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
    if(m_twitLib)
        delete m_twitLib;
}

void QweenMainWindow::makeWidgets(){
    m_twitLib = new QTwitLib();

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
}

void QweenMainWindow::applySettings(){
    ui->statusText->setStyleSheet(settings->inputStyle());
    ui->statusText->setRequireCtrlOnEnter(settings->requireCtrlOnEnter());

    //setupTimers();
    //setupTwitter();
}

bool QweenMainWindow::isNetworkAvailable(){
    //stub
    //TODO: implement
    return true;
}

void QweenMainWindow::setupMenus()
{
    //TODO: ���ۂ�TreeView����Ctrl+C���@�\����悤�ɂ���
    ui->actCopyStot->setText(ui->actCopyStot->text()+"\tCtrl+C");
    ui->actCopyIdUri->setText(ui->actCopyIdUri->text()+"\tCtrl+Shift+C");
    QAction *act = new QAction(QIcon(), tr("URL����̑S�p������̐؂藣��"), this);
    //connect(act, SIGNAL(triggered()), this, SLOT(open()));
    m_postModeMenu->addAction(act);

    act = new QAction(QIcon(), tr("API�R�}���h���������"), this);
    //connect(act, SIGNAL(triggered()), this, SLOT(open()));
    m_postModeMenu->addAction(act);

    m_actAutoShortenUri = new QAction(QIcon(), tr("�����I��URL��Z�k����"), this);
    m_actAutoShortenUri->setCheckable(true);
    //connect(act, SIGNAL(triggered()), this, SLOT(open()));
    m_postModeMenu->addAction(m_actAutoShortenUri);

    act = new QAction(QIcon(), tr("�S�p�X�y�[�X�𔼊p�X�y�[�X�ɂ���"), this);
    //connect(act, SIGNAL(triggered()), this, SLOT(open()));
    m_postModeMenu->addAction(act);

    ui->postButton->setMenu(m_postModeMenu);
}

void QweenMainWindow::setupTabs(){
    QFile tabSettings(QweenApplication::profileDir()+"/tabs.xml");
    if(tabSettings.exists()) tabWidget->restoreState(&tabSettings);
    else tabWidget->restoreState(NULL);
}

void QweenMainWindow::setupTimers(){
    m_timelineTimer->setInterval(60*1000);
    m_timelineTimer->start();

    m_DMTimer->setInterval(600*1000);
    m_DMTimer->start();
    //m_DMTimer->stop();

    m_replyTimer->setInterval(240*1000);
    m_replyTimer->start();
    //m_replyTimer->stop();

    m_favTimer->setInterval(600*1000);
    m_favTimer->start();
    //m_favTimer->stop();

    m_fetchAnimTimer->setInterval(85);
    m_fetchAnimTimer->start();
}

void QweenMainWindow::setupTrayIcon(){
    m_trayIcon->setIcon(QIcon(":/res/normal.png"));
    setWindowIcon(QIcon(":/res/normal.png"));
    m_trayIcon->show();
}

void QweenMainWindow::setupTwitter(){
    m_twitLib->SetLoginInfo(settings->userid(), settings->password());
}

void QweenMainWindow::makeConnections(){
    //MainMenu
    connect(ui->actExit, SIGNAL(triggered()),
            this, SLOT(close()));
    //PostMode
    connect(m_postModeMenu, SIGNAL(aboutToShow()),
            this, SLOT(OnPostModeMenuOpen()));
    //StatusText->postButton
    connect(ui->statusText,SIGNAL(returnPressed()),
            ui->postButton,SLOT(click()));
    //TrayIcon
    connect(m_trayIcon, SIGNAL(messageClicked()), this, SLOT(OnMessageClicked()));

    //Timers
    connect(m_timelineTimer, SIGNAL(timeout()), this, SLOT(OnTimelineTimerTimeout()));
    connect(m_DMTimer, SIGNAL(timeout()), this, SLOT(OnDmTimerTimeout()));
    connect(m_replyTimer, SIGNAL(timeout()), this, SLOT(OnReplyTimerTimeout()));
    connect(m_favTimer, SIGNAL(timeout()), this, SLOT(OnFavTimerTimeout()));

    //Twitter
    connect(m_twitLib,SIGNAL(OnResponseReceived(Returnables::Response *)),this,SLOT(OnResponseReceived(Returnables::Response *)));

    //Tab
    connect(tabWidget, SIGNAL(itemSelected(Twitter::TwitterItem)),
            this, SLOT(OnItemSelected(Twitter::TwitterItem)));
    //StatusText
    connect(ui->statusText, SIGNAL(uriShorteningFinished()),
            this, SLOT(OnUriShorteningFinished()));
}

void QweenMainWindow::OnResponseReceived(Returnables::Response *resp){
    if(resp)
    {
        switch(resp->reqID)
        {
        case Returnables::FRIENDS_TIMELINE:
            {
                Returnables::FriendsTimeline *pTimeline = static_cast<Returnables::FriendsTimeline *>(resp);
                QString popupText;
                QString title(tr("�V�� ") + QString::number(pTimeline->list.count()) + tr("��\n"));
                while(!pTimeline->list.isEmpty()){
                    Returnables::StatusElementPtr element = pTimeline->list.takeLast();
                    Twitter::TwitterItem item(Twitter::Status, element, resp->reqID, false);
                    if(m_newestFriendsStatus < item.id()) m_newestFriendsStatus = item.id();
                    popupText.append(QString("%1 : %2\n").arg(item.userName(), item.status()));
                    tabWidget->addItem(item);
                }
                delete pTimeline;
                m_trayIcon->showMessage(title, popupText, QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information),
                                        5 * 1000);
                break;
            }
        case Returnables::NEW_STATUS:
        {
            ui->statusText->setText("");
            ui->statusText->setEnabled(true);
            ui->postButton->setEnabled(true);
            Returnables::NewStatus *pNewstatus = static_cast<Returnables::NewStatus*>(resp);
            Returnables::StatusElementPtr element = pNewstatus->status;
            tabWidget->addItem(Twitter::TwitterItem(Twitter::Status, element, resp->reqID, false));
            delete pNewstatus;
            break;
        }
        case Returnables::RECENT_MENTIONS:
        {
            Returnables::RecentMentions *p = static_cast<Returnables::RecentMentions *>(resp);
            while(!p->list.isEmpty()){
                Returnables::StatusElementPtr element = p->list.takeLast();
                Twitter::TwitterItem item(Twitter::Status, element, resp->reqID, false);
                if(m_newestReply < item.id()) m_newestReply = item.id();
                tabWidget->addItem(item);
            }
            delete p;
            break;
        }
        case Returnables::SENT_DIRECT_MESSAGES:
        {
            Returnables::SentDirectMessages *p = static_cast<Returnables::SentDirectMessages *>(resp);
            while(!p->list.isEmpty()){
                Returnables::DirectMessageElementPtr element = p->list.takeLast();
                Twitter::TwitterItem item(Twitter::DirectMessage, element, resp->reqID, false);
                if(m_newestSentDM < item.id()) m_newestSentDM = item.id();
                tabWidget->addItem(item);
            }
            delete p;
            break;
        }
        case Returnables::RECEIVED_DIRECT_MESSAGES:
        {
            Returnables::ReceivedDirectMessages *p = static_cast<Returnables::ReceivedDirectMessages *>(resp);
            while(!p->list.isEmpty()){
                Returnables::DirectMessageElementPtr element = p->list.takeLast();
                Twitter::TwitterItem item(Twitter::DirectMessage, element, resp->reqID, false);
                if(m_newestRecvDM < item.id()) m_newestRecvDM = item.id();
                tabWidget->addItem(item);
            }
            delete p;
            break;
        }
        case Returnables::FAVORITES:
        {
            Returnables::Favorites *p = static_cast<Returnables::Favorites *>(resp);
            while(!p->list.isEmpty()){
                Returnables::StatusElementPtr element = p->list.takeLast();
                Twitter::TwitterItem item(Twitter::Status, element, resp->reqID, false);
                if(m_newestFav < item.id()) m_newestFav = item.id();
                tabWidget->addItem(item);
            }
            delete p;
            break;
        }
        case Returnables::VERIFY_CREDENTIALS:
        {
            Returnables::VerifyCredentials *p = static_cast<Returnables::VerifyCredentials*>(resp);
            Returnables::ExtUserInfoElementPtr element = p->userExt;
            tabWidget->setMyId(element->user.id);
            if(element->user.id!=0){
                SERVER::Option1 opt;
                opt.since_id = m_newestFriendsStatus;
                opt.count = 20;
                m_twitLib->GetFriendsTimeline(&opt);
                OnDmTimerTimeout();
                OnReplyTimerTimeout();
                OnFavTimerTimeout();
            }
            delete p;
            break;
        }
        case Returnables::API_REQUESTS:
        {
            Returnables::ApiRequests *p = static_cast<Returnables::ApiRequests*>(resp);
            QMessageBox::information(this,tr("API���"),
                                     tr("���: %1\n�c��: %2\n���Z�b�g����: %3\n")
                                     .arg(QString::number(p->hourlyLimit),
                                          QString::number(p->remainingHits),
                                          p->resetTime));
            delete p;
            break;
        }
        case Returnables::USER_DETAILS:
        {
            Returnables::UserDetails *p = static_cast<Returnables::UserDetails*>(resp);
            Returnables::ExtUserInfoElementPtr element = p->userExt;
            QMessageBox::information(this, tr("�v���t�@�C�����"),
                                     tr("Following : %1\n"
                                        "Followers : %2\n"
                                        "Statuses count : %3\n"
                                        "Location : %4\n"
                                        "Bio : %5")
                                     .arg(QString::number(element->details.friendsCount),
                                          QString::number(element->user.followersCount),
                                          QString::number(element->details.statusesCount),
                                          element->user.location,
                                          element->user.description));
        }
        default:
            break;
        }
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
    /*if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }*/

    QFile tabSettings(QweenApplication::profileDir()+"/tabs.xml");
    tabSettings.open(QFile::WriteOnly);
    tabWidget->saveState(&tabSettings);

    settings->setGeometry(saveGeometry());
    settings->setWindowState(saveState());
    settings->save();
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
        m_twitLib->VerifyCredentials();
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
    m_twitLib->PostNewStatus(postText + tr(" ") + settings->statusSuffix(), 0, "Qween");
}

void QweenMainWindow::makeReplyOrDirectStatus(bool isAuto, bool isReply, bool isAll){

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
    SERVER::Option1 opt;
    opt.since_id = m_newestFriendsStatus;
    opt.count = 200;
    m_twitLib->GetFriendsTimeline(&opt);
}


void QweenMainWindow::OnDmTimerTimeout(){
    SERVER::Option5 s_opt;
    s_opt.since_id = m_newestSentDM;
    s_opt.page = 1;
    m_twitLib->GetSentDirectMessages(&s_opt);

    SERVER::Option5 r_opt;
    r_opt.since_id = m_newestRecvDM;
    r_opt.page = 1;
    m_twitLib->GetReceivedDirectMessages(&r_opt);
}

void QweenMainWindow::OnReplyTimerTimeout(){
    SERVER::Option3 opt;
    opt.since_id = m_newestReply;
    m_twitLib->GetRecentMentions(&opt);
}

void QweenMainWindow::OnFavTimerTimeout(){
    m_twitLib->GetFavorites();
}

void QweenMainWindow::OnItemSelected(const Twitter::TwitterItem &item)
{
    switch(item.type()){
    case Twitter::Status:
        ui->textBrowser->setHtml(tr("<html><body style=\"%1\">")
                                 .arg(settings->statusViewStyle()) +
                                 item.status() + tr("</body></html>"));
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
    default:
        break;
    }
}

void QweenMainWindow::OnPostModeMenuOpen(){
    m_actAutoShortenUri->setChecked(settings->uriAutoShorten());
    //TODO:���̑��O��Menu
}

void QweenMainWindow::OnUriShortened(const QString& src, const QString& dest){
    QMessageBox::information(this, "", dest);
}

void QweenMainWindow::OnIconDownloaded(quint64 userid, const QIcon &icon){
    ui->userIconLabel->setPixmap(icon.pixmap(50,50,QIcon::Normal,QIcon::On));
}

void QweenMainWindow::OnMessageClicked(){
    this->activateWindow();
}

void QweenMainWindow::on_statusText_textChanged(QString string)
{
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
    //TODO: �t�b�^�@�\�ƘA��
    //TODO: Shift�L�[
    //�ڂ�����Tween�̃\�[�X������ GetRestStatusCount
    return rv;
}

void QweenMainWindow::on_actShowUserStatus_triggered()
{
    m_twitLib->GetUserDetails(settings->userid());
}

void QweenMainWindow::on_actApiInfo_triggered()
{
    m_twitLib->RemainingApiRequests();
}

void QweenMainWindow::on_actQweenHomepage_triggered()
{
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
    //TODO: �����s�Ή��H��肽���ˁ[
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
    SERVER::Option1 opt;
    opt.since_id = m_newestFriendsStatus;
    opt.count = 200;
    m_twitLib->GetFriendsTimeline(&opt);
}

void QweenMainWindow::on_actCopyStot_triggered()
{
    //TODO: �����I���\�ɂ���
    Twitter::TwitterItem item = tabWidget->currentItem();
    if(item.type()==Twitter::Undefined) return;
    //TODO: Protected�Ȃ�R�s�[���Ȃ��ݒ��ǉ�
    //TODO: ReTweet�Ή�
    QString data = "%0:%1 [http://twitter.com/%0/status/%2]";
    QString dataRT = "%0:%1 [http://twitter.com/%2/status/%3]";
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(data.arg(item.screenName(), item.status(), QString::number(item.id())), QClipboard::Clipboard);
}

void QweenMainWindow::on_actCopyIdUri_triggered()
{
    //TODO: �����I���\�ɂ���
    Twitter::TwitterItem item = tabWidget->currentItem();
    if(item.type()==Twitter::Undefined) return;
    //TODO: Protected�Ȃ�R�s�[���Ȃ��ݒ��ǉ�
    //TODO: ReTweet�Ή�
    QString data = "http://twitter.com/%0/status/%1]";
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(data.arg(item.screenName(), QString::number(item.id())), QClipboard::Clipboard);
}

void QweenMainWindow::on_actExplosion_triggered()
{
    QMessageBox::information(this, tr("��٧"), tr("�������߂���Ă�"));
}

void QweenMainWindow::on_actShortenUri_triggered()
{
    ui->statusText->shortenUri();
}
