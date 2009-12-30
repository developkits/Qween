#ifndef QWEENMAINWINDOW_H
#define QWEENMAINWINDOW_H

#include <QMainWindow>
#include "qtwitlib.h"
#include "twitter.h"
namespace Ui {
    class QweenMainWindow;
}

typedef enum  {
TinyUrl, Bitly, Isgd, Unu, Jmp, Twurl, Googl
} ShortUriSvc;

class QweenSettings;
class QweenTabCtrl;
class QTwitLib;
class QTimer;
class QMenu;
class QIcon;
class QSystemTrayIcon;
class AbstractUriShortener;
class QweenMainWindow : public QMainWindow {
    Q_OBJECT
public:
    QweenMainWindow(QWidget *parent = 0);
    ~QweenMainWindow();

private:
    void applySettings();
    QString convertUri(ShortUriSvc svc, const QString& source, bool shorten = true);
    void doUriConvert(ShortUriSvc svc);
    bool isNetworkAvailable();
    void setupMenus();
    void setupTabs();
    void setupTimers();
    void setupTrayIcon();
    void setupTwitter();
    void makeConnections();
    void makeWidgets();
    void doPost();
    void makeReplyOrDirectStatus(bool isAuto, bool isReply, bool isAll);

    int getRestStatusCount(const QString& str, bool footer = true);


protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void showEvent(QShowEvent *event);

private:
    Ui::QweenMainWindow *ui;
    QweenSettings *settings;
    QweenTabCtrl *tabWidget;
    QTwitLib  *m_twitLib;
    QTimer *m_timelineTimer;
    QTimer *m_DMTimer;
    QTimer *m_replyTimer;
    QTimer *m_fetchAnimTimer;
    QMenu *m_postModeMenu;
    QAction *m_actAutoShortenUri;

    QSystemTrayIcon *m_trayIcon;
    QIcon *m_normalIcon;
    QIcon *m_errorIcon;
    QIcon *m_offlineIcon;
    QIcon *m_replyIcon;
    QIcon *m_unreadIcon;
    QList<QIcon *> m_busyIconList;

    QList<QString> m_inputHistory;

    //flags:
    //最初のshowEventかどうかを判断するフラグ
    bool m_firstShow;
    //短縮URI機能はEnterで発動する場合と、メニューから発動する場合がある。
    //Enterの時はPostしなければいけないので、その判断をするフラグ
    bool m_postAfterShorten;

    //DEBUG
    AbstractUriShortener *m_urisvc;

public slots:
    //void OnError(QString error);
    //void OnMessageReceived(QString message);
    //void OnStatusReceived(SERVER::RESP response);
    void OnResponseReceived(Returnables::Response *);
    void OnItemSelected(const Twitter::TwitterItem &item);
    void OnPostModeMenuOpen();
    void OnUriShortened(const QString& src, const QString& dest);
    void OnIconDownloaded(quint64 userid, const QIcon& icon);
    void OnMessageClicked();
    void OnUriShorteningFinished();

private slots:
    void on_actShortenUri_triggered();
    void on_actExplosion_triggered();
    void on_actCopyIdUri_triggered();
    void on_actCopyStot_triggered();
    void on_actUpdate_triggered();
    void on_actionTest_icon_triggered();
    void on_actionTest_iconmanager_triggered();
    void on_statusText_returnPressed();
    void on_actionTest_bitly_triggered();
    void on_actQweenHomepage_triggered();
    void on_actApiInfo_triggered();
    void on_actShowUserStatus_triggered();
    void on_statusText_textChanged(QString );
    void on_postButton_clicked();
    void on_actAboutQween_triggered();
    void on_actAboutQt_triggered();
    void on_actOptions_triggered();
    void on_timelineTimer_timeout();
};

#endif // QWEENMAINWINDOW_H
