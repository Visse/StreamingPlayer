#pragma once

#include <QtWidgets/qscrollarea.h>

class Tile;
class Feed;
class Entry;
class StreamingPlayer;

class FeedWidget :
    public QScrollArea
{
    Q_OBJECT
public:
    FeedWidget( QWidget *parent );

    void init( StreamingPlayer *player, QLayout *layout );

    void setFeed( QSharedPointer<Feed> feed );
    void clearFeed();

    void addEntry( QSharedPointer<Entry> entry );

signals:
    void openUrl( QString url );

private slots:
    void scrolledFeed();
    void feedRetrivedMore();
    void showMore();

private:
    StreamingPlayer *mPlayer;
    QLayout *mLayout;
    QList<Tile*> mTiles;


    int mReadFeedTo = 0,
        mTargetTileCount = 0;
    QSharedPointer<Feed> mFeed;
};