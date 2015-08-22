#pragma once

#include "VideoProvider.h"

#include <qpointer.h>

class GenericVideoProvider :
    public VideoProvider
{
public:
    GenericVideoProvider( const QList<Format> &formats_ ) :
            formats(formats_)
    {}
    QList<Format> formats;

    virtual QList<Format> getFormats() override {
        return formats;
    }
};