#pragma once


#include <qobject.h>
#include <QtNetwork/qnetworkaccessmanager.h>


class ThumbnailRetriverSignal :
    public QObject
{
    Q_OBJECT
signals:
    void retrivedThumbnail( QPixmap image );
};

class ThumbnailRetriver :
    public QObject
{
    Q_OBJECT
public:
    ThumbnailRetriver( QObject *parent );

    void retriveThumbnail( QString url, QObject *reciver, const char *slot );


private slots:
    void responceRecived( QNetworkReply *reply );

private:
    QNetworkAccessManager mNetworkMgr;
    QMap<QNetworkReply*,ThumbnailRetriverSignal*> mPendingRequests;


};