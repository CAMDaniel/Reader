#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QString sPath="/";
    dirModel=new QFileSystemModel(this);
    dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
    dirModel->setRootPath(sPath);

    fileModel=new QFileSystemModel(this);
    fileModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    fileModel->setRootPath(sPath);

    ui->TV_fileSystem->setModel(dirModel);
    ui->LV_files->setModel(fileModel);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_TV_fileSystem_clicked(const QModelIndex &index)
{
    QString sPath=dirModel->fileInfo(index).absoluteFilePath();
    ui->LV_files->setRootIndex(fileModel->setRootPath(sPath));

}

void MainWindow::on_LV_files_clicked(const QModelIndex &index)
{
    QString sPath=dirModel->fileInfo(index).absoluteFilePath();
    QFile file(sPath);
    if(!file.open(QIODevice::ReadOnly))
        QMessageBox::information(0, "info", file.errorString());
    QTextStream in(&file);
    ui->TB_readAsTxt->setText(in.readAll());

    //double test[10] = {1, 2, 3, 1, 2,5, 6, 7, 8 , 9};
    //ui->frame->setData(test, 2, 5);

}
