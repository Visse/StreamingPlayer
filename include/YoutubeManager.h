#pragma once

#include <qobject.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <qpointer.h>
#include <qcache.h>

#include "Feed.h"


class YoutubeParser;
struct SearchResult;

class QProcess;
class YoutubeFeed;
class YoutubeVideoProvider;

class YoutubeManager :
    public QObject
{
    Q_OBJECT
public:
    enum SearchType {
        ST_Video = 0
    };

public:
    YoutubeManager( QObject *parent = nullptr );

    QSharedPointer<Feed> search( QString term, SearchType type );
    QSharedPointer<Feed> releatedVideos( QString videoId );
    QSharedPointer<Entry> retriveVideoInfo( QString videoId );

    QSharedPointer<Entry> retriveChannelInfo( QString channelId );

    QSharedPointer<Feed> playlistFromId( QString playlistId );

    QSharedPointer<Feed> commentsToVideoId( QString videoId );
    QSharedPointer<Feed> repliesToCommentId( QString commentId );

    void retriveMore( YoutubeFeed *feed );
    void retriveVideoFormats( YoutubeVideoProvider *provider );

private slots:
    void requestResponceResived( QNetworkReply *reply );
    void youtubeVideoInfoRetrived();

private:
    struct RequestInfo;
    void handleResponceQuary( RequestInfo info, QNetworkReply *reply );
    
    void queryForMoreVideoInfo( QMap<QString,QSharedPointer<Entry>> videos );
    void queryForMoreChannelInfo( QMap<QString,QSharedPointer<Entry>> channels );

private:
    enum RequestType {
        RT_Quary,
    };

    struct RequestInfo {
        RequestType type;
        QPointer<YoutubeFeed> feed;

        QMap<QString,QSharedPointer<Entry>> entries;
    };

private:
    QNetworkAccessManager mNetworkMgr;
    QMap<QNetworkReply*, RequestInfo> mPendingRequests;
    QMap<QProcess*, QPointer<YoutubeVideoProvider>> mPendingVideoRequests;
    YoutubeParser *mParser;

    QSet<YoutubeFeed*> mPendingFeeds;
    QSet<YoutubeVideoProvider*> mPendingVideoFormats;

    QCache<QString,QSharedPointer<Entry>> mVideoEntryCache;
    QCache<QString,QSharedPointer<Entry>> mChannelEntryCache;
};