#include <QtWidgets\qapplication.h>

#include "StreamingPlayer.h"
#include "PlayerTab.h"

#include "YoutubeManager.h"
#include "YoutubeSearchProvider.h"
#include "YoutubeVideoUrlOpener.h"
#include "YoutubeChannelUrlOpener.h"
#include "ThumbnailRetriver.h"
#include "GenericUrlOpener.h"

#include <QGst/init.h>

int main( int argc, char* argv[] )
{
    QApplication app(argc, argv);
    QGst::init(&argc, &argv);

    QSharedPointer<YoutubeManager> youtubeMgr( new YoutubeManager(&app) );
    QSharedPointer<SearchProvider> youtubeSearch( new YoutubeSearchProvider(youtubeMgr, &app) );
    QSharedPointer<YoutubeVideoUrlOpener> youtubeVideoUrl( new YoutubeVideoUrlOpener(youtubeMgr, &app) );
    QSharedPointer<YoutubeChannelUrlOpener> youtubeChannelUrl( new YoutubeChannelUrlOpener(youtubeMgr, &app) );
    QSharedPointer<ThumbnailRetriver> thumbnailRetriver( new ThumbnailRetriver(&app) );

    QSharedPointer<GenericUrlOpener> genericUrlOpener( new GenericUrlOpener(&app) );

    StreamingPlayer player( thumbnailRetriver );
    player.addSearchProvider( youtubeSearch );
    player.addUrlOpener( youtubeVideoUrl );
    player.addUrlOpener( youtubeChannelUrl );

    // add the generic one last
    player.addUrlOpener( genericUrlOpener );

    player.createNewDefaultTab();
    player.setWindowIcon( QIcon(":/StreamingPlayer/Icon") );
    player.show();

    return app.exec();
}
