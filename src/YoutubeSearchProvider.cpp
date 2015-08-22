#include "YoutubeSearchProvider.h"
#include "YoutubeManager.h"




YoutubeSearchProvider::YoutubeSearchProvider( QSharedPointer<YoutubeManager> youtubeMgr, QObject *parent ) :
    SearchProvider(parent),
    mYoutubeMgr(youtubeMgr)
{
}
QSharedPointer<Feed> YoutubeSearchProvider::search( QString searchTerm )
{
    return mYoutubeMgr->search( searchTerm, YoutubeManager::ST_Video );
}
