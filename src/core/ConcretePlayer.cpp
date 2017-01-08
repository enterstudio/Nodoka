//
// Created by mistlight on 12/22/2016.
//

#include <QDebug>
#include "ConcretePlayer.h"

Core::ConcretePlayer::ConcretePlayer() {
    /* Load the VLC engine */
    this->inst = libvlc_new(0, NULL);

    // null initalization
    this->mediaLoaded = false;
    this->audiobookFileProxy = nullptr;

    return;
}

void Core::ConcretePlayer::loadMedia(QSqlRecord record) {
    if(mediaLoaded) {
        this->releaseMedia();
    }

    this->audiobookFileProxy = std::shared_ptr<AudiobookFileProxy>(new AudiobookFileProxy(record));
    this->currentPath = audiobookFileProxy->path();

    this->mediaItem = libvlc_media_new_path(this->inst, this->currentPath.toStdString().c_str());
    this->mediaPlayer = libvlc_media_player_new_from_media(this->mediaItem);
    this->mediaEventManager = libvlc_media_event_manager(this->mediaItem);
    this->playerEventManager = libvlc_media_player_event_manager(this->mediaPlayer);

    if(this->mediaPlayer != NULL) {
        this->mediaLoaded = true;
    }

    this->setupCallbacks();
}

void Core::ConcretePlayer::releaseMedia() {
    libvlc_media_player_release(this->mediaPlayer);
    this->mediaLoaded = false;
}

void Core::ConcretePlayer::play() {
    if(this->mediaLoaded) {
        libvlc_media_player_play(this->mediaPlayer);
    }
}

void Core::ConcretePlayer::stop() {
    libvlc_media_player_pause(this->mediaPlayer);
}

void Core::ConcretePlayer::setupCallbacks() {
    libvlc_event_attach(this->mediaEventManager,
                        libvlc_MediaStateChanged,
                        (libvlc_callback_t) [](const struct libvlc_event_t * event, void *data) {
                            auto player = static_cast<ConcretePlayer*>(data);
                            emit player->stateChanged(player->getCurrentState());
                        },
                        this);

    libvlc_event_attach(this->playerEventManager,
                        libvlc_MediaPlayerTimeChanged,
                        (libvlc_callback_t) [](const struct libvlc_event_t * event, void *data) {
                            auto player = static_cast<ConcretePlayer*>(data);
                            emit player->timeProgressed(player->getCurrentTime());
                        },
                        this);

    libvlc_event_attach(this->mediaEventManager,
                        libvlc_MediaParsedChanged,
                        (libvlc_callback_t) [](const struct libvlc_event_t * event, void *data) {
                            auto player = static_cast<ConcretePlayer*>(data);

                            int parsedStatus = libvlc_media_is_parsed(player->mediaItem);

                            if(parsedStatus) {
                                emit player->parsedStatusChanged(true);
                            } else {
                                emit player->parsedStatusChanged(false);
                            }
                        },
                        this);
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
