#pragma once

#include "VideoProvider.h"

#include <qpointer.h>

class YoutubeManager;

class YoutubeVideoProvider :
    public VideoProvider
{
public:
    QString videoId;
    QList<Format> formats;
    QPointer<YoutubeManager> youtubeMgr;

    virtual QList<Format> getFormats() override;
};