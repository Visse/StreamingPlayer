#pragma once

#include "VideoProvider.h"

#include <QTime>
#include <qpointer.h>

class YoutubeManager;

class YoutubeVideoProvider :
    public VideoProvider
{
public:
    QString videoId;
    QList<Format> formats;
    QPointer<YoutubeManager> youtubeMgr;

    QTime updatedTime;

    virtual QList<Format> getFormats() override;
};