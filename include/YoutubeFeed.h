#pragma once

#include "Feed.h"
#include "YoutubeManager.h"

#include <qpointer.h>
#include <qurl.h>

class YoutubeFeed :
    public Feed
{
public:
    virtual bool hasMore() {
        return !nextPageUrl.isEmpty();
    }
    virtual void retriveMore()
    {
        if( hasMore() && youtubeMgr ) {
           youtubeMgr->retriveMore( this );
        }
    }

public:
    QPointer<YoutubeManager> youtubeMgr;
    QUrl nextPageUrl;
};
