//
// Created by mistlight on 12/22/2016.
//

#ifndef NODOKANATIVE_MAINWINDOW_H
#define NODOKANATIVE_MAINWINDOW_H


#include <QMenu>
#include <QtWidgets/QWidget>
#include <QMainWindow>
#include <QAbstractItemView>
#include <src/model/Directory.h>
#include <src/model/Audiobook.h>
#include <src/model/FileDisplayModel.h>
#include <src/core/ConcretePlayer.h>
#include <src/core/Setting.h>
#include <QComboBox>
#include <QTreeWidget>
#include <QStandardItemModel>
#include <src/event-handler/AudiobookListViewHandler.h>
#include <src/core/ScanPlayer.h>
#include <src/event-handler/AudiobookCollectionHandler.h>
#include <src/event-handler/FileListViewHandler.h>
#include "ui_MainWindow.h"
#include "SettingsForm.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {

private:
    Audiobook* audiobookModel;
    FileDisplayModel* fileDisplayModel;
    Core::ConcretePlayer* concretePlayer;
    Core::ScanPlayer* scanPlayer;
    Directory* directoryModel;
    Ui::MainWindow *ui;
    SettingsForm* settingsForm;

    Core::Setting* settings;

    //menus
    QMenu* audiobookMenu;

    // event handlers
    AudiobookListViewHandler *abListHandler;
    FileListViewHandler *fileListHandler;
    std::shared_ptr<AudiobookCollectionHandler> collectionHandler;

    // logical states
    bool isPlaying;
    std::shared_ptr<AudiobookFileProxy> currentlyPlayingFile;
    double currentTime;
    void setCurrentlyPlayingFile(std::shared_ptr<AudiobookFileProxy> file);



public:
    MainWindow(Directory* directoryModel,
               Audiobook* audiobookModel,
               Core::ConcretePlayer* concretePlayer,
               Core::ScanPlayer* scanPlayer,
               Core::Setting* setting,
               std::shared_ptr<ProxyManager> manager,
               std::shared_ptr<AudiobookCollectionHandler> handler,
               QWidget *parent = 0);
    virtual ~MainWindow();

    void performSettings();
    void performRescan();
    void performExit();

    // helper functions
    void menuSetup();
    void setup();
    void loadCurrentAudiobookIfExists();

    void setSelectedFile(QString path);
    void setCurrentTime(long long currentTime);
    void setIsPlaying(bool isPlaying);
    void updateFileView();
    void populateSpeedChoose();
    void setSpeed(QString speed);


public slots:
    void audiobookFileStateUpdated(std::shared_ptr<AudiobookFileProxy> abFile);
    void playerStateUpdated(std::shared_ptr<AudiobookFileProxy> abFile, bool isPlaying);
    void playerTimeUpdated(std::shared_ptr<AudiobookFileProxy> abFile, long long currentTime);
    void performAudiobookAdd();

    void setLabel(QLabel *pLabel,
                  std::shared_ptr<AudiobookFileProxy> proxy = std::shared_ptr<AudiobookFileProxy>(new AudiobookFileProxy()),
                  long long currentTime = -1);
};

// Stylesheet string to be used in the Audiobook list view
const static char* AB_ITEM_STYLESHEET = ""
        "div.item {"
        "font-family: \"Roboto\";"
        "margin-left: 20px;"
        "}"
        ""
        "span.name {"
        "font-size: 14px;"
        "font-weight: bold;"
        "}"
;

const static char* LIST_VIEW_STYLESHEET = ""
        "QWidget#audiobookViewVertical, QListView#audiobookView {"
        "font-size: 14px;"
        "border: 0px solid #38302e;"
        "background-color: #eee;"
        "color: #515151;"
        "}"
        ""
        "QWidget#fileViewVertical, QListView#fileView {"
        "border: 0px solid #38302e;"
        "background-color: #fff;"
        "color: #515151;"
        "}"
        ""
        "QListView::item:selected {"
        "background-color: #555152;"
        "color: #eee;"
        "}"
        ""
        "QListView::item:selected:hover {"
        "background-color: #555152;"
        "}"
        ""
        "QListView::item:selected:active {"
        "background-color: #555152;"
        "color: #eee;"
        "}"
        ""
        "QListView::item:hover {"
        "background-color: #e4e0e1;"
        "}"
        ""
        "QAbstractScrollArea {"
        "background-color: #91b3bc;"
        "color: #2b4251;"
        "}"
        ""
        "QListView#audiobookView::item {"
        "border-bottom: 1px solid #dcdcdd;"
        "margin-left: 20px;"
        "padding-top: 10px;"
        "padding-bottom: 10px;"
        "}"
        ""
        "QListView#fileView::item {"
        "padding-top: 5px;"
        "padding-bottom: 5px;"
        "}"
;

// Stylesheet string to be used in the File list view
const static char* FILE_ITEM_STYLESHEET = ""
        "div.file-item {"
        "margin-left: 20px;"
        "font-family: \"Roboto\""
        "}"
        ""
        "span.name {"
        "font-weight: bold;"
        "}"
        ""
;

#endif //NODOKANATIVE_MAINWINDOW_H
