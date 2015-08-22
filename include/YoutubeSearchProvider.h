    #pragma once

#include "SearchProvider.h"


class YoutubeManager;

class YoutubeSearchProvider :
    public SearchProvider
{
public:
    YoutubeSearchProvider( QSharedPointer<YoutubeManager> youtubeMgr, QObject *parent );

    virtual QSharedPointer<Feed> search( QString searchTerm ) override;

private:
    QSharedPointer<YoutubeManager> mYoutubeMgr;
};