#include "YoutubeManager.h"
#include "YoutubeParser.h"
#include "SearchProvider.h"
#include "YoutubeFeed.h"
#include "YoutubeVideoProvider.h"

#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <qprocess.h>
#include <qurlquery.h>

static const QString YOUTUBE_KEY  =  QStringLiteral( "AIzaSyASnp3YHYHGMxZ0xBTsxEM_UtXYt8fQTpE" );
static const QString SEARCH_BASE_URL = QStringLiteral( "https://www.googleapis.com/youtube/v3/search?part=snippet&maxResults=50&safeSearch=none&key=%1&type=%2&q=%3" );
static const QString VIDEO_LIST_BASE_URL = QStringLiteral( "https://www.googleapis.com/youtube/v3/videos?part=contentDetails,snippet,statistics&key=%1&id=%2" );
static const QString VIDEO_RELEATED_BASE_URL = QStringLiteral( "https://www.googleapis.com/youtube/v3/search?part=snippet&maxResults=50&safeSearch=none&type=video&key=%1&relatedToVideoId=%3" );
static const QString CHANNEL_LIST_BASE_URL = QStringLiteral( "https://www.googleapis.com/youtube/v3/channels?part=contentDetails,snippet,statistics&key=%1&id=%2" );
static const QString PLAYLIST_BASE_URL = QStringLiteral( "https://www.googleapis.com/youtube/v3/playlistItems?part=snippet&maxResults=50&key=%1&playlistId=%2" );

static const QString COMMENT_THREAD_BASE_URL = QStringLiteral( "https://www.googleapis.com/youtube/v3/commentThreads?part=snippet&maxResults=100&textFormat=html&key=%1&videoId=%2" );
static const QString COMMENT_REPLIES_BASE_URL = QStringLiteral( "https://www.googleapis.com/youtube/v3/commentThreads?part=snippet&maxResults=100&textFormat=html&key=%1&parentId=%2" );

QString toString( YoutubeManager::SearchType type )
{
    switch( type ) {
    case( YoutubeManager::ST_Video ):
        return QStringLiteral("video");
    }
    return QStringLiteral("video");
}

YoutubeManager::YoutubeManager( QObject *parent ) :
    QObject(parent)
{
    mParser = new YoutubeParser( this, this );
    connect( &mNetworkMgr, SIGNAL(finished(QNetworkReply*)), SLOT(requestResponceResived(QNetworkReply*)) );
    mVideoEntryCache.setMaxCost( 1000 );
}

QSharedPointer<Feed> YoutubeManager::search( QString term, SearchType type)
{
    QUrl url = SEARCH_BASE_URL.arg( YOUTUBE_KEY, toString(type), term );

    QSharedPointer<YoutubeFeed> feed( new YoutubeFeed );
        feed->nextPageUrl = url;
        feed->youtubeMgr = this;

    return feed;
}

QSharedPointer<Feed> YoutubeManager::releatedVideos( QString videoId )
{
    QUrl url = VIDEO_RELEATED_BASE_URL.arg( YOUTUBE_KEY, videoId );

    QSharedPointer<YoutubeFeed> feed( new YoutubeFeed );
        feed->nextPageUrl = url;
        feed->youtubeMgr = this;

    return feed;
}

QSharedPointer<Entry> YoutubeManager::retriveVideoInfo( QString videoId )
{
    QSharedPointer<Entry> *entry = mVideoEntryCache[videoId];
    if( entry ) {
        return *entry;
    }

    entry = new QSharedPointer<Entry>;
    entry->reset( new Entry );
    (*entry)->properties["type"] = "video";

    RequestInfo request;
    request.type = RT_Quary;
    request.entries.insert( videoId, *entry );


    QUrl url = VIDEO_LIST_BASE_URL.arg( YOUTUBE_KEY, videoId );
    QNetworkReply *reply = mNetworkMgr.get( QNetworkRequest(url) );

    mPendingRequests.insert( reply, request );
    mVideoEntryCache.insert( videoId, entry );

    return *entry;
}

QSharedPointer<Entry> YoutubeManager::retriveChannelInfo( QString channelId )
{
    QSharedPointer<Entry> *entry = mVideoEntryCache[channelId];
    if( entry ) {
        return *entry;
    }
    
    entry = new QSharedPointer<Entry>;
    entry->reset( new Entry );

    RequestInfo request;
    request.type = RT_Quary;
    request.entries.insert( channelId, *entry );
    
    QUrl url = CHANNEL_LIST_BASE_URL.arg( YOUTUBE_KEY, channelId );
    QNetworkReply *reply = mNetworkMgr.get( QNetworkRequest(url) );

    mPendingRequests.insert( reply, request );
    mVideoEntryCache.insert( channelId, entry );

    return *entry;
}

QSharedPointer<Feed> YoutubeManager::playlistFromId( QString playlistId )
{
    QUrl url = PLAYLIST_BASE_URL.arg( YOUTUBE_KEY, playlistId );
    
    QSharedPointer<YoutubeFeed> feed( new YoutubeFeed );
        feed->nextPageUrl = url;
        feed->youtubeMgr = this;

    return feed;
}

QSharedPointer<Feed> YoutubeManager::commentsToVideoId( QString videoId )
{
    QUrl url = COMMENT_THREAD_BASE_URL.arg( YOUTUBE_KEY, videoId );
    
    QSharedPointer<YoutubeFeed> feed( new YoutubeFeed );
        feed->nextPageUrl = url;
        feed->youtubeMgr = this;

    return feed;
}

QSharedPointer<Feed> YoutubeManager::repliesToCommentId( QString commentId )
{
    QUrl url = COMMENT_REPLIES_BASE_URL.arg( YOUTUBE_KEY, commentId );
    
    QSharedPointer<YoutubeFeed> feed( new YoutubeFeed );
        feed->nextPageUrl = url;
        feed->youtubeMgr = this;

    return feed;
}

void YoutubeManager::retriveVideoFormats( YoutubeVideoProvider *provider )
{
    if( mPendingVideoFormats.contains(provider) ) return;

    QStringList args;

    args << "--no-cache-dir" << "--dump-json" << "--write-annotations" << QStringLiteral("https://www.youtube.com/watch?v=%1").arg(provider->videoId);

    QProcess *process = new QProcess( this );
    connect( process, SIGNAL(finished(int)), SLOT(youtubeVideoInfoRetrived()) );
    process->start( QStringLiteral("youtube-dl"), args );
    
    mPendingVideoRequests.insert( process, provider );
    mPendingVideoFormats.insert( provider );
}

void YoutubeManager::retriveMore( YoutubeFeed *feed )
{
    if( mPendingFeeds.contains(feed) ) return;

    RequestInfo info;
        info.type = RT_Quary;
        info.feed = feed;

    QNetworkReply *reply = mNetworkMgr.get( QNetworkRequest(feed->nextPageUrl) );

    mPendingFeeds.insert( feed );
    mPendingRequests.insert( reply, info );
}

void YoutubeManager::requestResponceResived( QNetworkReply *reply )
{
    auto iter = mPendingRequests.find( reply );
    if( iter == mPendingRequests.end() ) {
        qDebug( "Reply not in pending (YoutubeManager::requestResponceResived)!" );
        return;
    }

    RequestInfo info = iter.value();
    switch ( info.type )
    {
    case( RT_Quary ):
        handleResponceQuary( info, reply );
        break;
    }

    reply->deleteLater();
    mPendingRequests.erase( iter );
}

void mergeEntries( Entry &e1, const Entry &e2 )
{
    for( auto iter = e2.properties.begin(), end=e2.properties.end(); iter != end; ++iter ) {
        e1.properties.insert( iter.key(), iter.value() );
    }
}

void YoutubeManager::handleResponceQuary( RequestInfo info, QNetworkReply *reply )
{
    QSharedPointer<YoutubeFeed> feed = mParser->parseFeed( reply->readAll() );
    
    QString nextPageToken = feed->properties["nextPageToken"].toString();
    if( !nextPageToken.isEmpty() ) {
        QUrl url = reply->url();
        QUrlQuery query(url);
        query.removeAllQueryItems("pageToken");
        query.addQueryItem( "pageToken", nextPageToken );

        url.setQuery( query );
        feed->nextPageUrl = url;
    }

    if( info.feed ) {
        mPendingFeeds.remove( info.feed.data() );

        info.feed->content.append( feed->content );
        info.feed->nextPageUrl = feed->nextPageUrl;
        info.feed->retiviedMore();
    }

    QMap<QString,QSharedPointer<Entry>> videoIds;

    for( const auto &entry : feed->content )
    {
        QString type = entry->properties["youtubeType"].toString();
        if( type == "youtube#video" ) {
            QString id =  entry->properties["id"].toString();
            if( entry->properties["youtube#contentDetails"].toBool() == false ) {
                videoIds.insert( id, entry );
            }
            mVideoEntryCache.insert( id, new QSharedPointer<Entry>(entry)  );
        }
        else if( type == "youtube#channel" ) {
        
        }
    }

    if( !videoIds.empty() ) {
        queryForMoreVideoInfo( videoIds );
    }


    for( const auto &entry : feed->content ) {
        auto iter = info.entries.find( entry->properties["id"].toString() );
        if( iter != info.entries.end() ) {
            QSharedPointer<Entry> req = iter.value();
            mergeEntries( *req, *entry );
            iter.value()->changed();
        }
    }
}

void YoutubeManager::youtubeVideoInfoRetrived()
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    Q_ASSERT( process != nullptr );

    auto iter = mPendingVideoRequests.find( process );
    Q_ASSERT( iter != mPendingVideoRequests.end() );

    QList<Format> formats = mParser->parseVideoFormats( process->readAll() );
    QPointer<YoutubeVideoProvider> provider = iter.value();

    if( provider ){
        provider->formats = formats;
        provider->changed();
    }

    mPendingVideoRequests.erase( iter );
}


void YoutubeManager::queryForMoreVideoInfo( QMap<QString,QSharedPointer<Entry>> videos )
{
    QUrl url = VIDEO_LIST_BASE_URL.arg( YOUTUBE_KEY, QStringList(videos.keys()).join(",") );
    QNetworkReply *reply = mNetworkMgr.get( QNetworkRequest(url) );
    RequestInfo request;
        request.entries = videos;
        request.type = RT_Quary;

    mPendingRequests.insert( reply, request );
}

void YoutubeManager::queryForMoreChannelInfo( QMap<QString,QSharedPointer<Entry>> channels )
{

}