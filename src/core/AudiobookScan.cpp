#include <memory>
#include <QtWidgets/QMessageBox>
#include "AudiobookScan.h"

// helper functions

// do the actual recusrive scan directory
static void performScanDirectory(QSqlRecord directoryRecord, std::shared_ptr<QDir> currentDirectory, Audiobook* audiobook);
static bool checkDirectorysimilarity(std::vector<std::shared_ptr<QDir>> vector);
static QMap<QString, bool> isAudioBookFileCache;

void Core::scanDirectory(QSqlRecord directoryRecord, Audiobook* audiobook) {
    QString path = directoryRecord.value("full_path").toString();
    std::shared_ptr<QDir> currentDirectory(new QDir(path));

    if(currentDirectory->exists()) {
        performScanDirectory(directoryRecord, currentDirectory, audiobook);
    }
}

void performScanDirectory(QSqlRecord directoryRecord, std::shared_ptr<QDir> currentDirectory, Audiobook* audiobook) {
    QDirIterator it(*currentDirectory, QDirIterator::NoIteratorFlags);
    std::vector<std::shared_ptr<QDir>> loadedDirectories;
    std::vector<std::shared_ptr<QFile>> loadedAudioFiles;

    while(it.hasNext()) {
        QString currentPath = it.next();

        // check if the path is a directory or a file
        std::shared_ptr<QDir> potentialDir(new QDir(currentPath));
        std::shared_ptr<QFile> potentialFile(new QFile(currentPath));


        // if it is a directory, then we
        if(potentialDir->exists()) {
            // don't bother with these..
            if(potentialDir->dirName() == "." || potentialDir->dirName() == "..") {
                continue;
            }

            loadedDirectories.push_back(potentialDir);
        } else if(potentialFile->exists()) {
            if(Core::isAudiobookFile(currentPath, potentialFile)) {
                loadedAudioFiles.push_back(potentialFile);
            }
        }
    }

    if(loadedDirectories.size() > 0) {
        // if all of the directories are similar, then simply make a call
        // to register this audiobook right away

        // TODO: add more checks here...
        if(checkDirectorysimilarity(loadedDirectories)) {
            audiobook->registerAudiobook(directoryRecord, currentDirectory);
        } else {
            for(auto &dir : loadedDirectories) {
                performScanDirectory(directoryRecord, dir, audiobook);
            }
        }
    } else {
        if(loadedAudioFiles.size() > 0) {
            audiobook->registerAudiobook(directoryRecord, currentDirectory);
        }
    }
}

bool checkDirectorysimilarity(std::vector<std::shared_ptr<QDir>> dirList) {
    // base case
    if(1 == dirList.size()) {
        return true;
    }

    // TODO: implement string similarity checks here
    return false;
}

bool Core::isAudiobookFile(QString path, std::shared_ptr<QFile> file) {
    // by default, non-existing file is not considered to be an audiobook file
    if(!file->exists()) {
        return false;
    }

    if(isAudioBookFileCache.contains(path)) {
        return isAudioBookFileCache[path];
    }

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(*file);

    if(type.name().startsWith("audio") || type.name().startsWith("video")) {
        isAudioBookFileCache[path] = true;
        return true;
    } else {
        isAudioBookFileCache[path] = false;
        return false;
    }

}


QList<QString> Core::getAllFiles(std::shared_ptr<QDir> directory) {
    QList<QString> filePaths;
    QDirIterator it(*directory, QDirIterator::NoIteratorFlags);

    while(it.hasNext()) {
        QString currentPath = it.next();

        QFileInfo currentFileInfo(currentPath);

        if(currentFileInfo.isFile()) {
            std::shared_ptr<QFile> currentFile(new QFile(currentPath));

            if(Core::isAudiobookFile(currentPath, currentFile)) {
                filePaths.push_back(currentPath);
            }

        } else if(currentFileInfo.isDir()) {
            std::shared_ptr<QDir> currentDir(new QDir(currentPath));
            // skip the special . and .. directories
            if(currentDir->dirName() == "." || currentDir->dirName() == "..") {
                continue;
            }

            auto subFilePaths = getAllFiles(currentDir);
            filePaths.append(subFilePaths);
        }
    }

    filePaths.sort();

    return filePaths;
}

