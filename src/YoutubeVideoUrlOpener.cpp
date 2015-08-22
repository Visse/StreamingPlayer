#include "YoutubeVideoUrlOpener.h"
#include "YoutubeManager.h"

bool extractVideoId( QString url, QString &id )
{
    static QRegExp regex = QRegExp( "https?:\\/\\/(?:www\\.)?(?:youtube\\.com\\/watch\\?(?:.+&)?v=|youtu\\.be\\/|youtube\\.com/v/)([^#\\&\\?]{6,})(?:.+)?" );

    if( regex.exactMatch(url) ) {
        id = regex.cap(1);
        return true;
    }
    return false;
}

YoutubeVideoUrlOpener::YoutubeVideoUrlOpener( QSharedPointer<YoutubeManager> youtubeMgr, QObject *parent ) :
    UrlOpener(parent),
    mYoutubeMgr(youtubeMgr)
{}
    
bool YoutubeVideoUrlOpener::canOpenUrl( QString url )
{
    QString id;
    return extractVideoId( url, id );
}

QSharedPointer<Entry> YoutubeVideoUrlOpener::openUrl( QString url )
{
    QString id;
    if( extractVideoId(url, id) ) {
        return mYoutubeMgr->retriveVideoInfo( id );
    }
    return QSharedPointer<Entry>();
}