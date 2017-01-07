//
// Created by mistlight on 1/2/2017.
//

#include <QtSql>
#include <QtWidgets/QMessageBox>
#include "Audiobook.h"

Audiobook::Audiobook(AudiobookFile* audiobookFileModel, QObject *parent) : QSqlTableModel(parent) {
    this->setTable("audiobooks");
    this->setEditStrategy(EditStrategy::OnManualSubmit);

    this->audiobookFile = audiobookFileModel;

    this->select();
}

void Audiobook::registerAudiobook(QSqlRecord baseDirectoryRecord, std::shared_ptr<QDir> directory) {
    qDebug() << "Registering: " << directory->path();

    AudiobookRecord record(directory->path(), false);
    record.setValue("directory", baseDirectoryRecord.value("full_path").toString());
    record.setValue("completeness", 0);
    record.setValue("default_order", 0);
    record.setNull("selected_file");

    this->insertRecord(-1, record);

    // submit the data
    auto res = this->submitAll();
    if(!res) {
        QMessageBox::critical(0, "Error", "Insert audiobook failed");
    }

    res = this->select();
    if(!res) {
        QMessageBox::critical(0, "Error", "Update select failed");
    }


    QSqlQuery query;
    query.prepare("SELECT * FROM audiobooks WHERE full_path=?");
    query.addBindValue(directory->path());
    res = query.exec();
    auto hasNext = query.next();
    if(!res || !hasNext) {
        QMessageBox::critical(0, "Error", "Get audiobook id of recently added audiobook failed");
    }
    int audiobookId = query.record().value("id").toInt();

    this->audiobookFile->registerAudioBook(audiobookId, directory);
}

void Audiobook::removeAudiobookByBase(QString path) {
    QSqlQuery query;
    query.prepare("SELECT * FROM audiobooks WHERE directory=?");
    query.addBindValue(path);
    auto res = query.exec();
    if(!res) {
        QMessageBox::critical(0, "Warning", "Query to retrieve audiobook information failed");
        return;
    }

    while(query.next()) {
        auto audiobookId = query.value("id").toInt();
        this->audiobookFile->removeAudiobook(audiobookId);
    }

    QSqlQuery deleteQuery;
    deleteQuery.prepare("DELETE FROM audiobooks WHERE directory=?");
    deleteQuery.addBindValue(path);
    deleteQuery.exec();
    auto deleteRes = deleteQuery.exec();
    if(!deleteRes) {
        QMessageBox::critical(0, "Warning", "Query to delete audiobook information failed");
        return;
    }
}

void Audiobook::removeAudiobook(QSqlRecord record) {
    this->removeAudiobookByBase(record.value("full_path").toString());
}

QVariant Audiobook::data(const QModelIndex &index, int role) const {
    if(role == Qt::DisplayRole) {
        return this->record(index.row()).value("name");
    }

    return QSqlTableModel::data(index, role);
}
