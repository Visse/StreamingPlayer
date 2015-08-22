#pragma once

#include "Feed.h"

#include <qobject.h>
#include <qpointer.h>

class YoutubeManager;
class YoutubeFeed;
struct Format;


class YoutubeParser :
    public QObject
{
public:
    YoutubeParser( YoutubeManager *youtubeMgr, QObject *parent );

    QSharedPointer<YoutubeFeed> parseFeed( QByteArray feed );
    QList<Format> parseVideoFormats( QByteArray info );

private:
    QPointer<YoutubeManager> mYoutubeMgr;
};