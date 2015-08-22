#include "YoutubeParser.h"
#include "YoutubeFeed.h"
#include "YoutubeVideoProvider.h"

#include <qsharedpointer.h>
#include <qjsondocument.h>
#include <qjsonobject.h>

YoutubeParser::YoutubeParser( YoutubeManager *youtubeMgr, QObject *parent ) :
    QObject(parent),
    mYoutubeMgr(youtubeMgr)
{
}

void parseVideoItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item );
void parseChannelItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item );
void parsePlaylistItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item );
void parseCommentThreadItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item );
void parseCommentItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item );

QSharedPointer<YoutubeFeed> YoutubeParser::parseFeed( QByteArray bytes )
{
    QSharedPointer<YoutubeFeed> feed( new YoutubeFeed );
    feed->youtubeMgr = mYoutubeMgr;

    QJsonDocument doc = QJsonDocument::fromJson( bytes );

    QVariantMap content = doc.toVariant().toMap();
    
    if( content["error"].isValid() ) {
        qDebug() << "Youtube error: " << content["error"];
    }

    feed->properties["nextPageToken"] = content["nextPageToken"];
    
    QVariantList items = content["items"].toList();

    for( QVariant item_ : items ) {
        QVariantMap item = item_.toMap();

        QSharedPointer<Entry> entry( new Entry );
        
        QString kind = item["kind"].toString();
        QString type;

        bool search = false;
        if( kind == QStringLiteral("youtube#searchResult") ) {
            type = item["id"].toMap()["kind"].toString();
            search = true;
        }
        else {
            type = kind;
        }

        if( type == QStringLiteral("youtube#video") ) {
            parseVideoItem( mYoutubeMgr, *entry, item );
        }
        else if( type == QStringLiteral("youtube#channel") ) {
            parseChannelItem( mYoutubeMgr, *entry, item );
        }
        else if( type == QStringLiteral("youtube#playlistItem") ) {
            parsePlaylistItem( mYoutubeMgr, *entry, item );
        }
        else if( type == QStringLiteral("youtube#commentThread") ) {
            parseCommentThreadItem( mYoutubeMgr, *entry, item );
        }
        else if( type == QStringLiteral("youtube#comment") ) {
            parseCommentItem( mYoutubeMgr, *entry, item );
        }
        else {
            qDebug() << "Unknown youtube item: " << type;
            continue;
        }


        feed->content.push_back( entry );
    }
    return feed;
}

QList<Format> YoutubeParser::parseVideoFormats( QByteArray bytes )
{
    QJsonDocument doc = QJsonDocument::fromJson( bytes );
    QVariantMap content = doc.toVariant().toMap();

    QVariantList formats = content["formats"].toList();
    QList<Format> result;

    for( QVariant _format : formats ) {
        QVariantMap format = _format.toMap();
        Format f;

        f.filetype = format["ext"].toString();
        f.url = format["url"].toString();
        f.id = format["format_id"].toString();

        f.bitrate = format["abr"].toInt();

        f.width = format["width"].toInt();
        f.height = format["height"].toInt();

        if( format["format_note"].toString() == QStringLiteral("DASH video") ) {
            f.type = VT_VideoOnly;
        }
        else if( format["format_note"].toString() == QStringLiteral("DASH audio") ) {
            f.type = VT_AudioOnly;
        }
        else {
            f.type = VT_Normal;
        }

        QString title;
        if( f.type != VT_AudioOnly ) {
            title = QStringLiteral("%1x%2 (%3)").arg(
                format["width"].toString(),
                format["height"].toString(),
                format["ext"].toString()
            );
            if( f.type == VT_VideoOnly ) {
                title += " - Video Only";
            }
        }
        else {
            title = QString( "%1 bit/s (%2) - Audio Only" ).arg( QString::number(f.bitrate), format["ext"].toString() );
        }
        f.title = title;

        result.push_back( f );
    }
    return result;
}

void parseVideoItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item )
{
    
    QString id = item["id"].toString();
    if( id.isEmpty() ) {
        id = item["id"].toMap()["videoId"].toString();
    }
    
    entry.properties["type"] = "video";
    entry.properties["youtubeType"] = "youtube#video";
    entry.properties["id"] = id;
    entry.properties["url"] = QStringLiteral("https://www.youtube.com/watch?v=%1").arg(id);
    entry.properties["releative"] = qVariantFromValue( youtubeMgr->releatedVideos(id) );
    entry.properties["comments"] = qVariantFromValue( youtubeMgr->commentsToVideoId(id) );
            
    QSharedPointer<YoutubeVideoProvider> videoProvider( new YoutubeVideoProvider );
    videoProvider->videoId = id;
    videoProvider->youtubeMgr = youtubeMgr;
    entry.properties["videoProvider"] = qVariantFromValue(videoProvider.staticCast<VideoProvider>());

    if( item.contains(QStringLiteral("snippet")) ) {
        QVariantMap snippet = item["snippet"].toMap();

        entry.properties["title"] = snippet["title"];
        entry.properties["thumbnail"] = snippet["thumbnails"].toMap()["medium"].toMap()["url"];
        entry.properties["youtubeChannelId"] = snippet["channelId"];
        entry.properties["author"] = snippet["channelTitle"];
        entry.properties["authorUrl"] = QStringLiteral("https://www.youtube.com/channel/%1").arg(snippet["channelId"].toString());
        entry.properties["uploaded"] = QDateTime::fromString( snippet["publishedAt"].toString(), Qt::ISODate );
        entry.properties["description"] = snippet["description"];
        // doesn't seem like we have a updated tag, just 'publishedAt'
        entry.properties["updated"] = entry.properties["uploaded"];
        entry.properties["youtube#snippet"] = true;
    }
    if( item.contains(QStringLiteral("contentDetails")) ) {
        QVariantMap details = item["contentDetails"].toMap();

        QString time = details["duration"].toString();
        QRegExp regex( "P(?:(\\d+)D)?T?(?:(\\d{1,2})H)?(?:(\\d{1,2})M)?(?:(\\d{1,2})S)?" );
        
        if( regex.indexIn(time) >= 0 ) {
            unsigned int d, t, m, s;
            d = regex.cap(1).toUInt();
            t = regex.cap(2).toUInt();
            m = regex.cap(3).toUInt();
            s = regex.cap(4).toUInt();

            entry.properties["lenght"] = (d*24+t)*60*60 + m*60 + s;
        }
        entry.properties["definition"] = details["definition"];
        entry.properties["youtube#contentDetails"] = true;
    }
    if( item.contains(QStringLiteral("statistics")) ) {
        QVariantMap statistics = item["statistics"].toMap();

        entry.properties["likeCount"] = statistics["likeCount"];
        entry.properties["dislikeCount"] = statistics["dislikeCount"];
        entry.properties["viewCount"] = statistics["viewCount"];
        entry.properties["commentCount"] = statistics["commentCount"];
        entry.properties["youtube#statistics"] = true;
    }
}

void parseChannelItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item )
{
    QString id = item["id"].toString();
    if( id.isEmpty() ) {
        id = item["id"].toMap()["channelId"].toString();
    }
    
    entry.properties["type"] = "channel";
    entry.properties["youtubeType"] = "youtube#channel";
    entry.properties["id"] = id;
    entry.properties["url"] = QStringLiteral("https://www.youtube.com/channel/%1").arg(id);
            
    
    if( item.contains(QStringLiteral("snippet")) ) {
        QVariantMap snippet = item["snippet"].toMap();

        entry.properties["title"] = snippet["title"];
        entry.properties["description"] = snippet["description"];
        entry.properties["updated"] = QDateTime::fromString( snippet["publishedAt"].toString(), Qt::ISODate );
        entry.properties["thumbnail"] = snippet["thumbnails"].toMap()["default"].toMap()["url"];
    }
    if( item.contains(QStringLiteral("contentDetails")) ) {
        QVariantMap contentDetails = item["contentDetails"].toMap();
        QVariantMap relatedPlaylists = contentDetails["relatedPlaylists"].toMap();
        entry.properties["uploads"] = qVariantFromValue( youtubeMgr->playlistFromId(relatedPlaylists["uploads"].toString()) );
        entry.properties["favorites"] = qVariantFromValue( youtubeMgr->playlistFromId(relatedPlaylists["favorites"].toString()) );

        entry.properties["youtube#contentDetails"] = true;
    }
    
    if( item.contains(QStringLiteral("statistics")) ) {
        QVariantMap statistics = item["snippet"].toMap();

        entry.properties["viewCount"] = statistics["viewCount"];
        entry.properties["subscriberCount"] = statistics["subscriberCount"];
        entry.properties["uploadCount"] = statistics["videoCount"];

        entry.properties["youtube#statistics"] = true;
    }

}

void parsePlaylistItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item )
{
    QVariantMap snippet = item["snippet"].toMap();
    QVariantMap resourceId = snippet["resourceId"].toMap();

    entry.properties["title"] = snippet["title"];
    entry.properties["description"] = snippet["description"];

    QString kind = resourceId["kind"].toString();
    if( kind == "youtube#video" ) {
        entry.properties["type"] = "video";
        entry.properties["youtubeType"] = "youtube#video";
        entry.properties["thumbnail"] = snippet["thumbnails"].toMap()["medium"];

        QString id = resourceId["videoId"].toString();
        entry.properties["url"] = QStringLiteral("https://www.youtube.com/watch?v=%1").arg(id);
        entry.properties["id"] = resourceId["videoId"];
        entry.properties["releative"] = qVariantFromValue( youtubeMgr->releatedVideos(id) );
    }
}

void parseCommentThreadItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item )
{
    QVariantMap snippet = item["snippet"].toMap();
    parseCommentItem( youtubeMgr, entry, snippet["topLevelComment"].toMap() ); 

    if( snippet[""].toUInt() > 0 ) {
        entry.properties["replies"] = qVariantFromValue( youtubeMgr->repliesToCommentId(entry.properties["id"].toString()) );
    }
}

void parseCommentItem( YoutubeManager *youtubeMgr, Entry &entry, QVariantMap item )
{
    entry.properties["id"] = item["id"];
    entry.properties["type"] = "comment";

    QVariantMap snippet = item["snippet"].toMap();

    entry.properties["commentText"] = snippet["textDisplay"];
    entry.properties["uploaded"] = QDateTime::fromString( snippet["publishedAt"].toString(), Qt::ISODate );
    entry.properties["updated"] =QDateTime::fromString( snippet["updatedAt"].toString(), Qt::ISODate );
    entry.properties["author"] = snippet["authorDisplayName"];
    entry.properties["authorUrl"] = snippet["authorChannelUrl"];
}



