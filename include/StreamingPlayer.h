#pragma once

#include <QtWidgets/qmainwindow.h>


class SearchProvider;
class PlayerTab;
class ThumbnailRetriver;
class UrlOpener;

class StreamingPlayer :
    public QMainWindow
{
    Q_OBJECT
public:
    StreamingPlayer( QSharedPointer<ThumbnailRetriver> thumbnailRetriver, QWidget *parent = nullptr );

    QList<QSharedPointer<SearchProvider>> getSearchProviders() {
        return mSearchProviders;
    }
    QList<QSharedPointer<UrlOpener>> getUrlOpeners() {
        return mUrlOpeners;
    }
    QSharedPointer<ThumbnailRetriver> getThumbnailRetriver() {
        return mThumbnailRetriver;
    }


    void addSearchProvider( QSharedPointer<SearchProvider> provider );
    void addUrlOpener( QSharedPointer<UrlOpener> opener );
    
public slots:
    PlayerTab* createTab();
    PlayerTab* createNewDefaultTab();

private slots:
    void tabTitleChanged( QString newTitle );
    void currentChanged( int index );
    void tabCloseRequested( int index );

    void toogleDock( int dock );
    void updateDockActionChecks();

private:
    QList<QSharedPointer<SearchProvider>> mSearchProviders;
    QList<QSharedPointer<UrlOpener>> mUrlOpeners;
    QTabWidget *mTabWidget;
    QSharedPointer<ThumbnailRetriver> mThumbnailRetriver;

    struct DockInfo {
        int dock;
        QAction *action;
    };
    QVector<DockInfo> mDockActions;
};