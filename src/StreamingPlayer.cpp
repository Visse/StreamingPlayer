#include "StreamingPlayer.h"
#include "PlayerTab.h"

#include "ui_MainWindow.h"

#include <qsignalmapper.h>

StreamingPlayer::StreamingPlayer( QSharedPointer<ThumbnailRetriver> thumbnailRetriver, QWidget *parent ) :
    QMainWindow(parent),
    mThumbnailRetriver(thumbnailRetriver)
{
    Ui::MainWindow ui;
    ui.setupUi( this );
    
    mTabWidget = ui.tabWidget;

    connect( ui.actionCreateTab, SIGNAL(triggered()), SLOT(createNewDefaultTab()) );

    connect( mTabWidget, SIGNAL(currentChanged(int)), SLOT(currentChanged(int)) );
    connect( mTabWidget, SIGNAL(tabCloseRequested(int)), SLOT(tabCloseRequested(int)) );

    QSignalMapper *mapper = new QSignalMapper(this);
    connect( mapper, SIGNAL(mapped(int)), SLOT(toogleDock(int)) );

    for( int i=0; i < PlayerTab::PlayerDockType::PD_COUNT; ++i ) {
        QString name = PlayerTab::NameOfDock( static_cast<PlayerTab::PlayerDockType>(1<<i) );
        QAction *action = ui.menuDocks->addAction( name );
        action->setCheckable( true );

        mapper->setMapping( action, i );
        connect( action, SIGNAL(triggered()), mapper, SLOT(map()) );

        DockInfo info;
            info.dock = 1 << i;
            info.action = action;
        mDockActions.push_back( info );
    }
}

PlayerTab* StreamingPlayer::createTab()
{
    PlayerTab *tab = new PlayerTab( this );
    
    mTabWidget->addTab( tab, QStringLiteral("New Tab") );
    connect( tab, SIGNAL(windowTitleChanged(QString)), SLOT(tabTitleChanged(QString)) );
    
    return tab;
}

PlayerTab* StreamingPlayer::createNewDefaultTab()
{
    PlayerTab *tab = createTab();
    tab->showDockPage( PlayerTab::DP_SearchPage );
    return tab;
}

void StreamingPlayer::addSearchProvider( QSharedPointer<SearchProvider> provider )
{
    mSearchProviders.push_back( provider );
}

void StreamingPlayer::addUrlOpener( QSharedPointer<UrlOpener> opener )
{
    mUrlOpeners.push_back( opener );
}

void StreamingPlayer::tabTitleChanged( QString newTitle )
{
    QWidget *widget = qobject_cast<QWidget*>(sender());
    int index = mTabWidget->indexOf( widget );
    Q_ASSERT( index >= 0 );
    
    mTabWidget->setTabText( index, newTitle.left(20) );
    mTabWidget->setTabToolTip( index, newTitle );

    if( index == mTabWidget->currentIndex() ) {
        setWindowTitle( newTitle ); 
    }
}

void StreamingPlayer::currentChanged( int index )
{
    PlayerTab *tab = qobject_cast<PlayerTab*>( mTabWidget->widget(index) );
    Q_ASSERT( tab );
    setWindowTitle( tab->windowTitle() );

    disconnect( this, SLOT(updateDockActionChecks()) );
    connect( tab, SIGNAL(dockOpened(PlayerDockType)), SLOT(updateDockActionChecks()) );
    connect( tab, SIGNAL(dockClosed(PlayerDockType)), SLOT(updateDockActionChecks()) );

    updateDockActionChecks();
}

void StreamingPlayer::tabCloseRequested( int index )
{
    QWidget *tab = mTabWidget->widget(index);
    Q_ASSERT( tab );
    tab->deleteLater();
    mTabWidget->removeTab( index );
}

void StreamingPlayer::toogleDock( int dock )
{
    PlayerTab *currentTab = qobject_cast<PlayerTab*>( mTabWidget->currentWidget() );

    PlayerTab::PlayerDockType type = static_cast<PlayerTab::PlayerDockType>( 1 << dock );

    if( !currentTab->isDockOpen(type) ) {
        currentTab->openDock(type);
    }
    else {
        currentTab->closeDock( type );
    }
}


void StreamingPlayer::updateDockActionChecks()
{
    PlayerTab *tab = qobject_cast<PlayerTab*>( mTabWidget->currentWidget() );
    for( const DockInfo &info : mDockActions ) {
        bool open = tab->isDockOpen( static_cast<PlayerTab::PlayerDockType>(info.dock) );
        info.action->setChecked( open );
    }
}