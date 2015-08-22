#include "Tile.h"
#include "Feed.h"
#include "StreamingPlayer.h"
#include "ThumbnailRetriver.h"

#include <QtWidgets/qlabel.h>

Tile::Tile( QWidget *parent ) :
    QWidget(parent)
{}

void Tile::enterEvent( QEvent *event )
{
    QWidget::enterEvent(event);
    mouseEntered();
}

void Tile::leaveEvent( QEvent *event )
{
    QWidget::leaveEvent(event);
    mouseLeft();
}

QString formatLenght( unsigned int lenght )
{
    unsigned int s = lenght;
    unsigned int m = s / 60; 
    unsigned int h = m / 60;

    s %= 60;
    m %= 60;

    if( h == 0 ) {
        return QStringLiteral("%1:%2").arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
    }
    return QStringLiteral("%1:%2:%3").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
}

QString formatNumber( unsigned int num )
{
    if( num > 1000000 ) {
        return QString("%1.%2M").arg(num/1000000).arg(num/100000%10);
    }
    if( num > 1000 ) {
        return QString("%1.%2K").arg(num/1000).arg(num/100%10);
    }
    return QString::number(num);
}

#include "ui_VideoTile.h"
class VideoTile :
    public Tile
{
    Q_OBJECT
public:
    VideoTile( StreamingPlayer *player, QWidget *parent, QSharedPointer<Entry> entry ) :
        Tile(parent),
        mEntry(entry),
        mPlayer(player)
    {
        Ui::VideoTile ui;
        ui.setupUi( this );
        
        connect( this, SIGNAL(mouseEntered()), ui.ratingOverlay, SLOT(show()) );
        connect( this, SIGNAL(mouseLeft()), ui.ratingOverlay, SLOT(hide()) );

        ui.ratingOverlay->hide();

        connect( entry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
        connect( ui.playButton, SIGNAL(clicked()), SLOT(playButtonClicked()) );
        connect( ui.author, SIGNAL(clicked()), SLOT(authorClicked()) );

        mTitle = ui.title;
        mAuthor = ui.author;
        mUploaded = ui.uploaded;
        mLikeCount = ui.likeCount;
        mDislikeCount = ui.dislikeCount;
        mViewCount = ui.viewCount;
        mLenght = ui.lenght;
        mThumbnail = ui.image;
        mLikeBar = ui.likeBar;
        mDislikeBar = ui.dislikeBar;
        mPlayButton = ui.playButton;

        entryChanged();
    }

private slots:
    void thumbnailRetrived( QPixmap thumbnail )
    {
        mThumbnail->setPixmap( thumbnail );
    }

    void entryChanged()
    {
        mTitle->setText( mEntry->properties["title"].toString() );
        mTitle->setToolTip( mEntry->properties["title"].toString() );
        mAuthor->setText( mEntry->properties["author"].toString() );
        mAuthor->setToolTip( mEntry->properties["author"].toString() );
        mUploaded->setText( mEntry->properties["uploaded"].toDateTime().toString("dd/MM/yy hh:mm") );
        mUploaded->setToolTip( mEntry->properties["uploaded"].toDateTime().toString() );

        unsigned int likeCount = mEntry->properties["likeCount"].toUInt();
        unsigned int dislikeCount = mEntry->properties["dislikeCount"].toUInt();
        unsigned int viewCount = mEntry->properties["viewCount"].toUInt();

        mLikeCount->setText( formatNumber(likeCount) );
        mLikeCount->setToolTip( QString("%1 Likes").arg(locale().toString(likeCount)) );
        mDislikeCount->setText( formatNumber(dislikeCount) );
        mDislikeCount->setToolTip( QString("%1 Dislikes").arg(locale().toString(dislikeCount)) );
        mViewCount->setText( formatNumber(viewCount) );
        mViewCount->setToolTip( QString("%1 Views").arg(locale().toString(viewCount)) );

        unsigned int total = likeCount + dislikeCount;

        mLikeBar->setMaximum( total );
        mLikeBar->setValue( likeCount );
        mDislikeBar->setMaximum( total );
        mDislikeBar->setValue( dislikeCount );
        
        unsigned int lenght = mEntry->properties["lenght"].toUInt();
        mLenght->setText( formatLenght(lenght) );
        mLenght->setToolTip( QString("%1 Seconds").arg(locale().toString(lenght)) );

        QString thumbnail = mEntry->properties["thumbnail"].toString();
        if( mHasRetrivedThumbnail == false && !thumbnail.isEmpty() ) { 
            mPlayer->getThumbnailRetriver()->retriveThumbnail(  thumbnail, this, SLOT(thumbnailRetrived(QPixmap)) );
            mHasRetrivedThumbnail = true;
        }
    }

    void playButtonClicked()
    {
        QString url = mEntry->properties["url"].toString();
        if( !url.isEmpty() ) {
            openUrl( url );
        }
    }
    void authorClicked()
    {
        QString url = mEntry->properties["authorUrl"].toString();
        if( !url.isEmpty() ) {
            openUrl( url );
        }
    }

private:
    QSharedPointer<Entry> mEntry;
    StreamingPlayer *mPlayer;
    QLabel *mTitle, *mUploaded,
           *mLikeCount, *mDislikeCount, *mViewCount,
           *mLenght, *mThumbnail;
    QProgressBar *mLikeBar, *mDislikeBar;
    QAbstractButton *mPlayButton, *mAuthor;

    bool mHasRetrivedThumbnail = false;
};


#include "ui_ChannelTile.h"
class ChannelTile :
    public Tile
{
    Q_OBJECT
public:
    ChannelTile( StreamingPlayer *player, QWidget *parent, QSharedPointer<Entry> entry ) :
        Tile(parent),
        mEntry(entry),
        mPlayer(player)
    {
        Ui::ChannelTile ui;
        ui.setupUi( this );
        
        connect( this, SIGNAL(mouseEntered()), ui.ratingOverlay, SLOT(show()) );
        connect( this, SIGNAL(mouseLeft()), ui.ratingOverlay, SLOT(hide()) );

        ui.ratingOverlay->hide();

        connect( entry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
        connect( ui.openButton, SIGNAL(clicked()), SLOT(openButtonClicked()) );

        mTitle = ui.title;
        mUpdated = ui.updated;
        mThumbnail = ui.image;
        mSubscriberCout = ui.subscriberCount;
        mUploadCount = ui.uploadCount;
        mViewCount = ui.viewCount;
        mOpenButton = ui.openButton;

        entryChanged();
    }

private slots:
    void thumbnailRetrived( QPixmap thumbnail )
    {
        mThumbnail->setPixmap( thumbnail );
    }

    void entryChanged()
    {
        mTitle->setText( mEntry->properties["title"].toString() );
        mTitle->setToolTip( mEntry->properties["title"].toString() );
        mUpdated->setText( mEntry->properties["updated"].toDateTime().toString("dd/MM/yy hh:mm") );

        unsigned int subscriberCount = mEntry->properties["subscriberCount"].toUInt();
        unsigned int uploadCount = mEntry->properties["uploadCount"].toUInt();
        unsigned int viewCount = mEntry->properties["viewCount"].toUInt();

        mSubscriberCout->setText( formatNumber(subscriberCount) );
        mSubscriberCout->setToolTip( QString("%1 Subscribes").arg(locale().toString(subscriberCount)) );
        mUploadCount->setText( formatNumber(uploadCount) );
        mUploadCount->setToolTip( QString("%1 Uploads").arg(locale().toString(uploadCount)) );
        mViewCount->setText( formatNumber(viewCount) );
        mViewCount->setToolTip( QString("%1 Views").arg(locale().toString(viewCount)) );

        QString thumbnail = mEntry->properties["thumbnail"].toString();
        if( mHasRetrivedThumbnail == false && !thumbnail.isEmpty() ) { 
            mPlayer->getThumbnailRetriver()->retriveThumbnail(  thumbnail, this, SLOT(thumbnailRetrived(QPixmap)) );
            mHasRetrivedThumbnail = true;
        }
    }

    void openButtonClicked()
    {
        QString url = mEntry->properties["url"].toString();
        if( !url.isEmpty() ) {
            openUrl( url );
        }
    }

private:
    QSharedPointer<Entry> mEntry;
    StreamingPlayer *mPlayer;
    QLabel *mTitle, *mUpdated, *mThumbnail,
            *mSubscriberCout, *mUploadCount, *mViewCount;
    QAbstractButton *mOpenButton;

    bool mHasRetrivedThumbnail = false;
};

#include "ui_CommentTile.h"
class CommentTile :
    public Tile
{
    Q_OBJECT
public:
    CommentTile( QWidget *parent, QSharedPointer<Entry> entry ) :
        Tile(parent),
        mEntry(entry)
    {
        Ui::CommentTile ui;
        ui.setupUi( this );

        mContent = ui.content;
        mUpdated = ui.updated;
        mAuthor = ui.author;

        entryChanged();
        connect( entry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
    }
    
private slots:
    void entryChanged()
    {
        mContent->setText( mEntry->properties["commentText"].toString() );
        mUpdated->setText( mEntry->properties["updated"].toDateTime().toString("dd/MM/yy hh:mm") );
        mAuthor->setText( mEntry->properties["author"].toString() );
    }

private:
    QSharedPointer<Entry> mEntry;
    QLabel *mContent,
           *mUpdated, *mAuthor;

};


Tile* createTile( StreamingPlayer *player, QWidget *parent, QSharedPointer<Entry> entry )
{
    QString type = entry->properties["type"].toString();

    if( type == QStringLiteral("video") ) {
        return new VideoTile( player, parent, entry );
    }
    if( type == QStringLiteral("channel") ) {
        return new ChannelTile( player, parent, entry ); 
    }
    if( type == QStringLiteral("comment") ) {
        return new CommentTile( parent, entry );
    }
    qDebug() << "Failed to create tile of type " << type;
    return nullptr;
}

#include "Tile.moc"