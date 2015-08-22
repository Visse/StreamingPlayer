#include "ThumbnailRetriver.h"

#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>

#include <qpixmap.h>

ThumbnailRetriver::ThumbnailRetriver( QObject *parent ) :
    QObject( parent )
{
    connect( &mNetworkMgr, SIGNAL(finished(QNetworkReply*)), SLOT(responceRecived(QNetworkReply*)) );
}

void ThumbnailRetriver::retriveThumbnail( QString url, QObject *reciver, const char *slot )
{
    QNetworkReply *reply = mNetworkMgr.get( QNetworkRequest(QUrl(url)) );

    ThumbnailRetriverSignal *signal = new ThumbnailRetriverSignal;

    connect( signal, SIGNAL(retrivedThumbnail(QPixmap)), reciver, slot );
    mPendingRequests.insert( reply, signal );
}

void ThumbnailRetriver::responceRecived( QNetworkReply *reply )
{
    auto iter = mPendingRequests.find( reply );
    if( iter == mPendingRequests.end() ) {
        qDebug( "Hmmm.. request not in pending (ThumbnailRetriver::responceRecived)" );
    }
    QPixmap image;
    image.loadFromData( reply->readAll() );

    iter.value()->retrivedThumbnail( image );
    
    iter.value()->deleteLater();
    reply->deleteLater();
    mPendingRequests.erase( iter );
}