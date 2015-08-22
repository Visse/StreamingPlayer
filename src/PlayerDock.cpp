#include "PlayerDock.h"
#include "PlayerTab.h"

#include "StreamingPlayer.h"
#include "SearchProvider.h"
#include "FeedWidget.h"
#include "FlowLayout.h"
#include "Feed.h"

#include "ui_SearchDock.h"
#include "ui_ReleatedDock.h"
#include "ui_DescriptionDock.h"
#include "ui_HistoryDock.h"
#include "ui_WatchLaterDock.h"

#include <qdatetime.h>
#include <qscrollarea.h>
#include <qscrollbar.h>
#include <qdebug.h>

PlayerDock::PlayerDock( PlayerTab *tab ) :
    QDockWidget(tab),
    mTab(tab)
{
    mContent = new QWidget;
    setWidget( mContent );
}

void PlayerDock::closeEvent( QCloseEvent * event )
{
    closed();
    QDockWidget::closeEvent( event );
}

class SearchDock :
    public PlayerDock
{
    Q_OBJECT
public:
    SearchDock( PlayerTab *tab ) :
        PlayerDock(tab)
    {
        Ui::SearchDock ui;
        ui.setupUi( mContent );
        mFeed = ui.feed;
        mFeed->init( mTab->getPlayer(), new FlowLayout );
        mSearchText = ui.searchText;

        connect( ui.search, SIGNAL(clicked()), SLOT(performSearch()) );
        connect( mFeed, SIGNAL(openUrl(QString)), SIGNAL(openUrl(QString)) );

        setWindowTitle( "Search" );
    }
    
    virtual void entryOpened( QSharedPointer<Entry> entry ) 
    {}


private slots:
    void performSearch()
    {
        QString searchTerm = mSearchText->text().simplified();

        if( searchTerm.startsWith("http://") || searchTerm.startsWith("https://") ) {
            openUrl( searchTerm );
        }
        else {
            SearchProvider *provider = mTab->getPlayer()->getSearchProviders().first().data();

            QSharedPointer<Feed> feed = provider->search( searchTerm );
            mFeed->setFeed( feed );
        }
    }

private:
    QLineEdit *mSearchText;
    FeedWidget *mFeed;
};

class FeedViewDock :
    public PlayerDock
{
    Q_OBJECT
public:
    FeedViewDock( QString feedName, QString title, PlayerTab *tab ) :
        PlayerDock(tab),
        mFeedName(feedName)
    {
        Ui::ReleatedDock ui;
        ui.setupUi( mContent );

        mFeed = ui.feed;
        mFeed->init( mTab->getPlayer(), new FlowLayout );

        connect( mFeed, SIGNAL(openUrl(QString)), SIGNAL(openUrl(QString)) );

        setWindowTitle(title);
    }

    virtual void entryOpened( QSharedPointer<Entry> entry ) 
    {
        disconnect( this, SLOT(entryChanged()) );
        mEntry = entry;
        connect( mEntry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
        entryChanged();
    }
private slots:
    void entryChanged()
    {
        Q_ASSERT( mEntry );
        QSharedPointer<Feed> feed = qvariant_cast<QSharedPointer<Feed>>(mEntry->properties[mFeedName]);

        if( feed ) {
            mFeed->setFeed( feed );
        }
    }

private:
    FeedWidget *mFeed;
    QString mFeedName;
    QSharedPointer<Entry> mEntry;
};

class HistoryDock :
    public PlayerDock
{
public:
    HistoryDock( PlayerTab *tab ) :
        PlayerDock(tab)
    {
        Ui::HistoryDock ui;
        ui.setupUi( mContent );

        mFeed = ui.feed;
        mFeed->init( mTab->getPlayer(), new FlowLayout );

        connect( mFeed, SIGNAL(openUrl(QString)), SIGNAL(openUrl(QString)) );

        mFeed->setFeed( tab->getHistory() );

        setWindowTitle( "History" );
    }

    virtual void entryOpened( QSharedPointer<Entry> entry ) {}
private:
    FeedWidget *mFeed;
};

class DescriptionDock :
    public PlayerDock
{
    Q_OBJECT
public:
    
    DescriptionDock( PlayerTab *tab ) :
        PlayerDock(tab)
    {
        Ui::DesciptionDock ui;
        ui.setupUi( mContent );

        mTitle = ui.title;
        mUpdated = ui.updated;
        mDescription = ui.description;
        mAuthor = ui.author;

        mDescriptionScroll = ui.scrollArea;

        setWindowTitle( "Description" );
    }

    virtual void entryOpened( QSharedPointer<Entry> entry ) 
    {
        disconnect( this, SLOT(entryChanged()) );
        mEntry = entry;

        connect( mEntry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
        entryChanged();
    }

private slots:
    void entryChanged()
    {
        mTitle->setText(mEntry->properties["title"].toString());
        mUpdated->setText(mEntry->properties["updated"].toDateTime().toString("dd/MM/yy hh:mm:ss"));
        mDescription->setText(mEntry->properties["description"].toString());
        mAuthor->setText(mEntry->properties["author"].toString());

        mDescriptionScroll->verticalScrollBar()->setValue(0);
    }

private:
    QLabel *mTitle, *mUpdated, *mDescription;
    QAbstractButton *mAuthor;

    QScrollArea *mDescriptionScroll;
    QSharedPointer<Entry> mEntry;
};

class CommentDock :
    public PlayerDock
{
    Q_OBJECT
public:
    CommentDock( PlayerTab *tab ) :
        PlayerDock(tab)
    {
        Ui::ReleatedDock ui;
        ui.setupUi( mContent );

        mFeed = ui.feed;
        mFeed->init( mTab->getPlayer(), new QVBoxLayout );

        connect( mFeed, SIGNAL(openUrl(QString)), SIGNAL(openUrl(QString)) );

        setWindowTitle("Comments");
    }

    virtual void entryOpened( QSharedPointer<Entry> entry ) 
    {
        disconnect( this, SLOT(entryChanged()) );
        mEntry = entry;
        connect( mEntry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
        entryChanged();
    }
private slots:
    void entryChanged()
    {
        Q_ASSERT( mEntry );
        QSharedPointer<Feed> feed = qvariant_cast<QSharedPointer<Feed>>(mEntry->properties["comments"]);

        if( feed ) {
            mFeed->setFeed( feed );
        }
    }

private:
    FeedWidget *mFeed;
    QSharedPointer<Entry> mEntry;
};

class WatchLaterDock :
    public PlayerDock
{
    Q_OBJECT
public:
    WatchLaterDock( PlayerTab *tab ) :
        PlayerDock(tab)
    {
        Ui::WatchLaterDock ui;
        ui.setupUi( mContent );

        mFeed = ui.feed;
        mFeed->init( mTab->getPlayer(), new FlowLayout );

        connect( mFeed, SIGNAL(openUrl(QString)), SIGNAL(openUrl(QString)) );

        setWindowTitle( "History" );
    }

    virtual void entryOpened( QSharedPointer<Entry> entry ) 
    {
        mEntry = entry;
        disconnect( this, SLOT(entryChanged()) );
        connect( entry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
        
        entryChanged();
    }
    
private slots:
    void entryChanged()
    {
        Q_ASSERT( mEntry );
    }
private:
    FeedWidget *mFeed;
    QSharedPointer<Entry> mEntry;
};



PlayerDock* CreateSeachDock( PlayerTab *tab )
{
    return new SearchDock( tab );
}

PlayerDock* CreateReleatedDock( PlayerTab *tab )
{
    return new FeedViewDock( "releative", "Releated", tab );
}

PlayerDock* CreateDescriptionDock( PlayerTab *tab )
{
    return new DescriptionDock( tab );
}

PlayerDock* CreateCommentDock( PlayerTab *tab )
{
    return new CommentDock( tab );
}

PlayerDock* CreateHistoryDock( PlayerTab *tab )
{
    return new HistoryDock( tab );
}

PlayerDock* CreateUploadsDock( PlayerTab *tab )
{
    return new FeedViewDock( "uploads", "Uploads", tab );
}

PlayerDock* CreateWatchLaterDock( PlayerTab *tab )
{
    return nullptr;
}

#include "PlayerDock.moc"