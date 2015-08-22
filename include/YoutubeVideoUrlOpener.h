#pragma once

#include "UrlOpener.h"

#include <qsharedpointer.h>

class YoutubeManager;

class YoutubeVideoUrlOpener :
    public UrlOpener
{
public:
    YoutubeVideoUrlOpener( QSharedPointer<YoutubeManager> youtubeMgr, QObject *parent = nullptr );
    
    virtual bool canOpenUrl( QString url ) override;
    virtual QSharedPointer<Entry> openUrl( QString url ) override;

private:
    QSharedPointer<YoutubeManager> mYoutubeMgr;
};