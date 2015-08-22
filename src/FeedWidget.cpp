#include "FeedWidget.h"

#include "Feed.h"
#include "Tile.h"

#include <qlayout.h>
#include <qscrollbar.h>

FeedWidget::FeedWidget( QWidget *parent ) :
    QScrollArea(parent)
{
}


void FeedWidget::init( StreamingPlayer *player, QLayout *layout )
{
    mPlayer = player;
    mLayout = layout;

    widget()->setLayout( layout );

    connect( verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(scrolledFeed()) );
}

void FeedWidget::setFeed( QSharedPointer<Feed> feed )
{
    clearFeed();
    mFeed = feed;

    connect( mFeed.data(), SIGNAL(retiviedMore()), SLOT(feedRetrivedMore()) );
    showMore();
}

void FeedWidget::clearFeed()
{
    for( Tile *tile : mTiles ) {
        mLayout->removeWidget( tile );
        tile->deleteLater();
    }
    mTiles.clear();
    mFeed.reset();

    mReadFeedTo = 0;
    mTargetTileCount = 0;
    
    disconnect( this, SLOT(feedRetrivedMore()) );
}

void FeedWidget::addEntry( QSharedPointer<Entry> entry )
{
    Tile *tile = createTile( mPlayer, nullptr, entry );
    if( tile ) {
        mLayout->addWidget( tile );
        mTiles.push_back( tile );

        connect( tile, SIGNAL(openUrl(QString)), SIGNAL(openUrl(QString)) );
    }
}

void FeedWidget::feedRetrivedMore()
{
    for( int i = mReadFeedTo; i < mFeed->content.size(); ++i ) {
        if( mTiles.size() >= mTargetTileCount ) {
            break;
        }

        mReadFeedTo++;
        addEntry( mFeed->content.at(i) );
    }
}

void FeedWidget::scrolledFeed()
{
    if( mFeed ) {
        int max = verticalScrollBar()->maximum();
        int value = verticalScrollBar()->value();

        if( value+50 > max ) {
            showMore();
            mTargetTileCount += 25;
        }
    }
}

void FeedWidget::showMore()
{
    if( !mFeed ) return;
    mTargetTileCount += 25;
    
    if( mFeed->content.size() > mReadFeedTo ) {
        feedRetrivedMore();
    }
    else {
        mFeed->retriveMore();
    }

}