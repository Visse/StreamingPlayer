#include "YoutubeVideoProvider.h"
#include "YoutubeManager.h"

QList<Format> YoutubeVideoProvider::getFormats()
 {
    if( formats.isEmpty() && youtubeMgr ) {
        youtubeMgr->retriveVideoFormats( this );
    }
    return formats;
}