//
// Created by mistlight on 1/27/2017.
//

#include <QtSql/QSqlTableModel>
#include "AudiobookListViewHandler.h"

AudiobookListViewHandler::AudiobookListViewHandler(QMainWindow *window,
                                                   QListView *audiobookListView,
                                                   std::shared_ptr<ProxyManager> proxyManager
) {
    this->mainWindow = window;
    this->audiobookListView = audiobookListView;
    this->proxyManager = proxyManager;
}


void AudiobookListViewHandler::handleResetAudiobook(std::shared_ptr<AudiobookProxy> audiobook) {

}

void AudiobookListViewHandler::handleDeleteAudiobook(std::shared_ptr<AudiobookProxy> audiobook) {

}

void AudiobookListViewHandler::contextMenuRequested(const QPoint &position) {
    // we first want to check if user clicked on an audiobook item
    auto modelIndex = this->audiobookListView->indexAt(position);
    if(modelIndex.isValid()) {
        auto model = dynamic_cast<QSqlTableModel*>(this->audiobookListView->model());
        auto record = model->record(modelIndex.row());
        auto audiobookProxy = this->proxyManager->getAudiobookProxy(record);

        auto removeAction = audiobookProxy->getRemoveAction();

        QMenu *menu = new QMenu(this->mainWindow);
        menu->addAction("Reset read state");
        menu->addAction("Mark as read");
        menu->addAction("Rescan this Audiobook");
        menu->addAction(removeAction);
        menu->exec(this->audiobookListView->mapToGlobal(position));

    } else {
        // if the user clicked somewhere other than the audiobook item, there is nothing to show...
        return;
    }
}

