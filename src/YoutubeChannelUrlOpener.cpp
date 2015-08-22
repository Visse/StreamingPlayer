#include "YoutubeChannelUrlOpener.h"
#include "YoutubeManager.h"

bool extractChannelId( QString url, QString &id )
{
    static QRegExp regex = QRegExp( "https?:\\/\\/(?:www\\.)?youtube\\.com\\/channel\\/([^\\/\\?\\&\\#]{6,})(?:[\\?\\#].+)?" );

    if( regex.exactMatch(url) ) {
        id = regex.cap(1);
        return true;
    }
    return false;
}

YoutubeChannelUrlOpener::YoutubeChannelUrlOpener( QSharedPointer<YoutubeManager> youtubeMgr, QObject *parent ) :
    UrlOpener(parent),
    mYoutubeMgr(youtubeMgr)
{}
    
bool YoutubeChannelUrlOpener::canOpenUrl( QString url )
{
    QString id;
    return extractChannelId( url, id );
}

QSharedPointer<Entry> YoutubeChannelUrlOpener::openUrl( QString url )
{
    QString id;
    if( extractChannelId(url, id) ) {
        return mYoutubeMgr->retriveChannelInfo( id );
    }
    return QSharedPointer<Entry>();
}