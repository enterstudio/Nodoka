//
// Created by mistlight on 12/22/2016.
//

#include <QDebug>
#include <future>
#include <include/vlc/vlc.h>
#include "ConcretePlayer.h"
#include <QTextCodec>
#include <iostream>


Core::ConcretePlayer::ConcretePlayer(Setting* setting, std::shared_ptr<ProxyManager> manager) {
    // load settings
    this->setting = setting;
    this->proxyManager = manager;

    // init volume and speed based on the settings file
    this->volume = setting->getVolume();
    this->speed = setting->getSpeed();

    /* Load the VLC engine */
    this->inst = libvlc_new(0, NULL);
    if(this->inst == NULL) {
        qWarning() << "ERROR";
        throw "Exception has occured";
    }

    this->mediaPlayer = libvlc_media_player_new(this->inst);

    if(this->mediaPlayer == NULL) {
        qWarning() << "ERROR: Could not create media player";
    }
    this->playerEventManager = libvlc_media_player_event_manager(this->mediaPlayer);

    // set up callbacks
    this->setupVLCCallbacks();
    this->setupEventHandlers();

    // set the volume of the media player to the read volume
    libvlc_audio_set_volume(this->mediaPlayer, volume);
    libvlc_media_player_set_rate(this->mediaPlayer, this->getRate());

    // null initalization
    this->mediaLoaded = false;
    this->audiobookFileProxy = nullptr;
    this->autoPlay = false;
    this->threadPool = std::unique_ptr<ThreadPool>(new ThreadPool(1));

    this->hasSeekTo = false;
}

bool Core::ConcretePlayer::canLoadMedia(QSqlRecord record) {
    this->audiobookFileProxy = this->proxyManager->getAudiobookFileProxy(record);

    // if the file doesn't even exist, the media is assumed to be unloadable
    if(!this->audiobookFileProxy->fileExists()) {
        return false;
    }

    return true;
}

void Core::ConcretePlayer::loadMedia(QSqlRecord record) {
    if(this->mediaLoaded) {
        return;
    }

    this->audiobookFileProxy = this->proxyManager->getAudiobookFileProxy(record);
    // if the file doesn't even exist, do not continue;
    if(!this->audiobookFileProxy->fileExists()) {
        return;
    }

    this->currentPath = audiobookFileProxy->path();

    auto path =  this->currentPath;
    this->currentFile = std::unique_ptr<QFile>(new QFile(path));

    if(!this->currentFile->open(QIODevice::ReadWrite)) {
        qDebug() << "QFILE FAILED!: " << path;
        return;
    }


    this->mediaItem = libvlc_media_new_fd(this->inst, this->currentFile->handle());
    if(this->mediaItem == NULL) {
        return;
    }
    libvlc_media_player_set_media(this->mediaPlayer, this->mediaItem);
    this->mediaEventManager = libvlc_media_event_manager(this->mediaItem);

    this->setupMediaCallbacks();

    this->mediaLoaded = true;

    if(this->autoPlay) {
        this->play();
    }
}

void Core::ConcretePlayer::releaseMedia() {
    this->mediaLoaded = false;
    if(this->mediaLoaded) {
        libvlc_media_release(this->mediaItem);
    }
}

void Core::ConcretePlayer::play() {
    if(this->mediaLoaded) {
        libvlc_media_player_play(this->mediaPlayer);
        this->autoPlay = true;
    }
}

void Core::ConcretePlayer::stop() {
    libvlc_media_player_pause(this->mediaPlayer);
}

void Core::ConcretePlayer::setupVLCCallbacks() {

    libvlc_event_attach(this->playerEventManager,
                        libvlc_MediaPlayerTimeChanged,
                        (libvlc_callback_t) [](const struct libvlc_event_t * event, void *data) {
                            auto player = static_cast<ConcretePlayer *>(data);

                            player->threadPool->enqueue([data, player]() {
                                if (player->mediaLoaded) {
                                    emit player->timeProgressed(player->getCurrentTime());
                                }
                            });

                        },
                        this);


    libvlc_event_attach(this->playerEventManager,
                        libvlc_MediaPlayerEncounteredError,
                        (libvlc_callback_t) [](const struct libvlc_event_t * event, void *data) {
                            auto player = static_cast<ConcretePlayer*>(data);
                            auto errorMsg = libvlc_errmsg();

                            if(errorMsg != nullptr) {
                                qDebug() << "Error is: " << errorMsg;
                            }
                        },
                        this);

    libvlc_event_attach(this->playerEventManager,
                        libvlc_MediaPlayerEndReached, ConcretePlayer::handleFinished, this);
}

libvlc_state_t Core::ConcretePlayer::getCurrentState() {
    if(this->mediaPlayer == nullptr) {
        return libvlc_NothingSpecial;
    }

    return libvlc_media_player_get_state(this->mediaPlayer);
}

libvlc_time_t Core::ConcretePlayer::getCurrentTime() {
    return libvlc_media_player_get_time(this->mediaPlayer);
}

std::shared_ptr<AudiobookFileProxy> Core::ConcretePlayer::getAudiobookFile() {
    return this->audiobookFileProxy;
}

long long Core::ConcretePlayer::getDurationInMs() {
    long long durationInMs = libvlc_media_get_duration(this->mediaItem);
    if(durationInMs == -1) {
        return -1;
    }
    return durationInMs;
}

double Core::ConcretePlayer::getDurationInSeconds() {
    int durationInMs = libvlc_media_get_duration(this->mediaItem);
    if(durationInMs == -1) {
        return -1;
    }

    double durationInSeconds = durationInMs / 1000.0;

    return durationInSeconds;
}

void Core::ConcretePlayer::updateSeekPosition(long long position) {
    // first check if we have a file
    if(!this->mediaLoaded) {
        // do not bother if we don't have a loaded media
        return;
    }

    if(libvlc_media_player_is_seekable(this->mediaPlayer)) {
        libvlc_media_player_set_time(this->mediaPlayer, static_cast<libvlc_time_t>(position));
    } else {
        qDebug() << "Media not seekable";
    }
}

void Core::ConcretePlayer::setupEventHandlers() {
    connect(this->setting, &Setting::volumeUpdated, [=](int newVolume) {
        this->setVolume(newVolume);
    });
}

void Core::ConcretePlayer::setVolume(int volume) {
    this->volume = volume;

    if(this->mediaLoaded) {
        libvlc_audio_set_volume(this->mediaPlayer, volume);
    }
}

Core::ConcretePlayer::~ConcretePlayer() {
    qDebug() << "Player destructor called";
}

void Core::ConcretePlayer::setupMediaCallbacks() {
    libvlc_event_attach(this->mediaEventManager,
                        libvlc_MediaStateChanged,
                        (libvlc_callback_t) [](const struct libvlc_event_t * event, void *data) {
                            auto player = static_cast<ConcretePlayer*>(data);
                            auto newState = event->u.media_state_changed.new_state;
                            emit player->stateChanged(newState);
                        },
                        this);

    libvlc_event_attach(this->mediaEventManager,
                        libvlc_MediaParsedChanged,
                        (libvlc_callback_t) [](const struct libvlc_event_t * event, void *data) {
                            auto player = static_cast<ConcretePlayer*>(data);
                            if(!player->mediaLoaded) {
                                return;
                            }

                            int parsedStatus = libvlc_media_is_parsed(player->mediaItem);

                            if(parsedStatus) {
                                emit player->parsedStatusChanged(true);

                                // load the current time if possible
                                if(!player->audiobookFileProxy->currentTimeNull()) {
                                    player->updateSeekPosition(player->audiobookFileProxy->getCurrentTime());
                                }
                            } else {
                                emit player->parsedStatusChanged(false);
                            }
                        },
                        this);


}

void Core::ConcretePlayer::playNextFile() {
    if(this->audiobookFileProxy != nullptr) {
        auto record = this->audiobookFileProxy->getNextFile();
        if(!record.getNullState()) {
            this->loadMedia(record.getRecord());
            this->play();
        }
    }
}

void Core::ConcretePlayer::handleFinished(const libvlc_event_t *event, void *data) {
    // we have to put this into a different thread because we can't use vlc functions
    // from a callback directly...
    std::thread t1([data]() {
        auto player = static_cast<ConcretePlayer*>(data);
        // set as complete
        player->getAudiobookFile()->setAsComplete();

        // go to the next file
        player->playNextFile();
    });

    t1.detach();
}

void Core::ConcretePlayer::setSpeed(QString speed) {
    this->speed = speed;

    libvlc_media_player_set_rate(this->mediaPlayer, this->getRate());
}

float Core::ConcretePlayer::getRate() {
    return this->speed.toFloat();
}

