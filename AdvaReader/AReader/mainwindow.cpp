#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QMessageBox>
#include <QTextStream>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //settings root's pathgit
    QString sPath="/";

    //settings file System of directories
    dirModel=new QFileSystemModel(this);
    dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
    dirModel->setRootPath(sPath);

    //settings files in the columns
    fileModel=new QFileSystemModel(this);
    fileModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    fileModel->setRootPath(sPath);

    //settings data files in system
    dataFileModel=new QFileSystemModel(this);
    dataFileModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    //filters for advacam sources
    QStringList nameFilter;
    nameFilter<<"*.txt"<<"*.pmx"<<"*.pbf"<<"*.h5"<<"*.pmx"<<"*.pbf"<<".dsc"<<"*.idx"<<"*.clog"<<".dsc";
    dataFileModel->setNameFilters(nameFilter);

    dataFileModel->setRootPath(sPath);
    dataFileModel->setNameFilterDisables(false);

    ui->TV_dataFiles->setModel(dataFileModel);
    ui->TV_fileSystem->setModel(dirModel);
    ui->LV_showFiles->setModel(fileModel);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_TV_fileSystem_clicked(const QModelIndex &index)
{
   QString sPath=dirModel->fileInfo(index).absoluteFilePath();
   ui->LV_showFiles->setRootIndex(fileModel->setRootPath(sPath));
   ui->TV_dataFiles->setRootIndex(dataFileModel->setRootPath(sPath));
}

void MainWindow::on_LV_showFiles_clicked(const QModelIndex &index)
{
   QString sPath=dirModel->fileInfo(index).absoluteFilePath();
   QFile file(sPath);
   if(!file.open(QIODevice::ReadOnly))
        QMessageBox::information(0, "info", file.errorString());
   QTextStream in(&file);
   ui->TB_readAsText->setText(in.readAll());

}
