#include "CreateCustomFormatDialog.h"

#include "ui_CreateCustomFormatDialog.h"

#include <qcombobox.h>
#include <qmessagebox.h>

CreateCustomFormatDialog::CreateCustomFormatDialog( QWidget *parent, QList<Format> formats ) :
    QDialog(parent),
    mFormats(formats)
{
    Ui::CreateCustomFormatDialog ui;
    ui.setupUi( this );

    mFormatName = ui.formatName;

    mVideoFormatList = ui.videoFormat;
    mAudioFormatList = ui.audioFormat;

    mVideoFormatInfo = ui.videoFormatInfo;
    mAudioFormatInfo = ui.audioFormatInfo;

    connect( mVideoFormatList, SIGNAL(currentIndexChanged(int)), SLOT(videoFormatSelectionChanged()) );
    connect( mAudioFormatList, SIGNAL(currentIndexChanged(int)), SLOT(audioFormatSelectionChanged()) );

    connect( this, SIGNAL(accepted()), SLOT(onAccept()) );

    mVideoFormatList->addItem( "", "" );
    mAudioFormatList->addItem( "", "" );

    for( const Format &f : mFormats ) {
        if( f.type == VT_VideoOnly || f.type == VT_Normal ) {
            mVideoFormatList->addItem( f.title, f.id );
        }
        if( f.type == VT_AudioOnly || f.type == VT_Normal ) {
            mAudioFormatList->addItem( f.title, f.id );
        }
    }
}
    
void CreateCustomFormatDialog::onAccept()
{
    QString name = mFormatName->text().trimmed();
    QString videoFormat = mVideoFormatList->currentData().toString();
    QString audioFormat = mAudioFormatList->currentData().toString();

    if( name.isEmpty() ) {
        name = QString("Unnamed format - %1 %2").arg(videoFormat, audioFormat);
    }

    createFormat( name, videoFormat, audioFormat );
}

void CreateCustomFormatDialog::videoFormatSelectionChanged()
{
    mVideoFormatInfo->clear();

    QString id = mVideoFormatList->currentData().toString();

    const Format *format = nullptr;
    for( const Format &f : mFormats ) {
        if( f.id == id ) {
            format = &f;
            break;
        }
    }
    if( !format ) return;


    QString info = 
        "Name: %1\n"
        "Id: %2\n"
        "Type: %3\n"
        "Width: %4\n"
        "Height: %5\n"
        "BitRate: %6\n";
    info = info.arg( format->title, 
              format->id, 
              format->filetype, 
              QString::number(format->width),
              QString::number(format->height), 
              QString::number(format->bitrate) 
    );

    mVideoFormatInfo->setText( info );
}

void CreateCustomFormatDialog::audioFormatSelectionChanged()
{
    mAudioFormatInfo->clear();

    QString id = mAudioFormatList->currentData().toString();

    const Format *format = nullptr;
    for( const Format &f : mFormats ) {
        if( f.id == id ) {
            format = &f;
            break;
        }
    }
    if( !format ) return;


    QString info = 
        "Name: %1\n"
        "Id: %2\n"
        "Type: %3\n"
        "BitRate: %6\n";
    info = info.arg( format->title, 
              format->id, 
              format->filetype,
              QString::number(format->bitrate) 
    );

    mAudioFormatInfo->setText( info );
}