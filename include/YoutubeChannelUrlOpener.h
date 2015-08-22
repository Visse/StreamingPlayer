#pragma once

#include "UrlOpener.h"

#include <qsharedpointer.h>

class YoutubeManager;

class YoutubeChannelUrlOpener :
    public UrlOpener
{
public:
    YoutubeChannelUrlOpener( QSharedPointer<YoutubeManager> youtubeMgr, QObject *parent = nullptr );
    
    virtual bool canOpenUrl( QString url ) override;
    virtual QSharedPointer<Entry> openUrl( QString url ) override;

private:
    
    QSharedPointer<YoutubeManager> mYoutubeMgr;
};