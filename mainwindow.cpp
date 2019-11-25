#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QKeyEvent>
#include <QSettings>
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tagsEdit->installEventFilter(this);
    ui->searchEdit->installEventFilter(this);

    // load settings
    QSettings settings("ttt", "eptg");
    int size = settings.beginReadArray("recents");
    for(int i=0 ; i<size ; i++)
    {
        settings.setArrayIndex(i);
        QString path = settings.value("recent").toString();
        ui->menuOpenRecent->addAction(path)->setData(path);
    }
    settings.endArray();

    connect(ui->menuOpenRecent, SIGNAL(triggered(QAction*)), this, SLOT(on_menuOpenRecent(QAction*)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_menuOpenRecent(QAction *action)
{
    QString path = action->data().toString();
    openRecent(path);
}

bool MainWindow::eventFilter(QObject* obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        {
            if (keyEvent->key() == Qt::Key_Up)
            {
                if (obj == ui->tagsEdit)
                {
                    if (ui->fillList->currentRow() <= 0)
                        return true;
                    ui->fillList->setCurrentRow(ui->fillList->currentRow() - 1);
                    ui->tagsEdit->setFocus();
                    return true;
                }
                else if (obj == ui->searchEdit)
                {
                    if (ui->searchList->currentRow() <= 0)
                        return true;
                    ui->searchList->setCurrentRow(ui->searchList->currentRow() - 1);
                    ui->searchEdit->setFocus();
                    return true;
                }
            }
            else if(keyEvent->key() == Qt::Key_Down)
            {
                if (obj == ui->tagsEdit)
                {
                    if (ui->fillList->currentRow()+1 >= ui->fillList->count())
                        return true;
                    ui->fillList->setCurrentRow(ui->fillList->currentRow() + 1);
                    ui->tagsEdit->setFocus();
                    return true;
                }
                else if (obj == ui->searchEdit)
                {
                    if (ui->searchList->currentRow()+1 >= ui->searchList->count())
                        return true;
                    ui->searchList->setCurrentRow(ui->searchList->currentRow() + 1);
                    ui->searchEdit->setFocus();
                    return true;
                }
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

QString PathAppend(const QString & path1, const QString & path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

void MainWindow::on_menuOpenFolder_triggered()
{
    QString pathName = QFileDialog::getExistingDirectory(this, "Select folder", "~");
    if (pathName == "")
        return;

    openRecent(pathName);
}

void MainWindow::openRecent(const QString & pathName)
{
    std::function<void(eptg::Model &, const QString, const QString)> inspect_folder;
    inspect_folder = [this,&inspect_folder](eptg::Model & model, const QString base_dir, const QString rel_dir)
        {
            QDir directory(PathAppend(base_dir, rel_dir));
            QStringList images = directory.entryList(QStringList() << "*.jpg" << "*.JPG",QDir::Files);
            for (auto & i : images)
                i = PathAppend(rel_dir, i);
            ui->fillList->addItems(images);

            QStringList folders = directory.entryList(QStringList(), QDir::Dirs);
            for (auto & f : folders)
                if ( ! f.startsWith("."))
                    inspect_folder(model, base_dir, PathAppend(rel_dir, f));
        };

    model = eptg::Load(pathName.toStdString());

    ui->fillList->clear();
    ui->tagsEdit->clear();
    inspect_folder(*model, pathName, ".");
    QPixmap void_image;
    ui->label_2->setPixmap(void_image);
    if (ui->fillList->count() > 0)
        ui->fillList->setCurrentRow(0);
    this->setWindowTitle("eptgQt - " + pathName);

    // set recent menu
    if (ui->menuOpenRecent->actions().size() == 0)
        ui->menuOpenRecent->addAction(pathName);
    else
    {
        // remove duplicates
        for(int i = ui->menuOpenRecent->actions().size() - 1 ; i >= 0 ; i--)
            if (pathName == ui->menuOpenRecent->actions()[i]->text())
                ui->menuOpenRecent->removeAction(ui->menuOpenRecent->actions()[i]);
        // insert
        QAction * qaction = new QAction(pathName);
        qaction->setData(pathName);
        ui->menuOpenRecent->insertAction(ui->menuOpenRecent->actions()[0], qaction);
        // remove excess recents
        while (ui->menuOpenRecent->actions().size() > 4)
            ui->menuOpenRecent->removeAction(ui->menuOpenRecent->actions()[4]);
    }

    // save recent
    QSettings settings("ttt", "eptg");
    settings.beginWriteArray("recents");
    for(int i = 0 ; i < ui->menuOpenRecent->actions().size() ; i++)
    {
        settings.setArrayIndex(i);
        settings.setValue("recent", ui->menuOpenRecent->actions()[i]->text());
    }
    settings.endArray();
}

void MainWindow::on_fillList_currentRowChanged(int currentRow)
{
    if (currentRow == -1)
        return;
    auto rel_path = ui->fillList->item(currentRow)->text();
    auto currentText = PathAppend(QString(model->path.c_str()), rel_path);
    QPixmap image(currentText);
    if (image.width() > 800 || image.height() > 600)
        image = image.scaled(800,600, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
    ui->label_2->setPixmap(image);

    const eptg::File & f = model->get_file(rel_path.toStdString());
    QString tag_line;
    for (const auto & t : f.tags)
        tag_line.append(QString(t.c_str())).append(" ");

    ui->tagsEdit->setText(tag_line);
    ui->tagsEdit->setFocus();
}


void MainWindow::on_tagsEdit_returnPressed()
{
    if ( ! model)
        return;

    auto selected_items = ui->fillList->selectedItems();
    if (selected_items.size() != 1)
        return;
    auto rel_path = selected_items[0]->text();

    eptg::File & f = model->get_file(rel_path.toStdString());
    for (const QString & tag : ui->tagsEdit->text().split(' '))
        if (tag.size() > 0)
            f.add_tag(tag.toStdString());

    eptg::Save(model);

    if (ui->fillList->currentRow()+1 < ui->fillList->count())
        ui->fillList->setCurrentRow(ui->fillList->currentRow()+1);
}

void MainWindow::on_searchEdit_returnPressed()
{
    if ( ! model)
        return;

    ui->searchList->clear();

    std::set<std::string> tags;
    for (const QString & tag : ui->searchEdit->text().split(' '))
        if (tag.size() > 0)
            tags.insert(tag.toStdString());

    QStringList images;
    for (const auto & f : model->get_files(tags))
        images.append(QString(f.first.c_str()));

    ui->searchList->addItems(images);
}

void MainWindow::on_searchList_currentRowChanged(int currentRow)
{
    if (currentRow == -1)
        return;
    auto rel_path = ui->searchList->item(currentRow)->text();
    auto currentText = PathAppend(QString(model->path.c_str()), rel_path);
    QPixmap image(currentText);
    if (image.width() > 800 || image.height() > 600)
        image = image.scaled(800,600, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
    ui->searchPreview->setPixmap(image);

    ui->searchEdit->setFocus();
}
