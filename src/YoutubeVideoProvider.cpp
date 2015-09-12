#include "YoutubeVideoProvider.h"
#include "YoutubeManager.h"


QList<Format> YoutubeVideoProvider::getFormats()
 {
     QTime expires( updatedTime );
     expires = expires.addSecs( 60*60 ); // add 1 hour
    if( expires < QTime::currentTime() ) {
        // retrive formats again if more than a hour has passed,
        // the urls isn't allways valid forever.
        formats.clear(); 
        qDebug() << "Clearing old formats.";
    }

    if( formats.isEmpty() && youtubeMgr ) {
        updatedTime = QTime::currentTime();
        youtubeMgr->retriveVideoFormats( this );
    }
    return formats;
}