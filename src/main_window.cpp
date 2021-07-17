#include "main_window.h"

MainWindow::MainWindow() {
    readSettings();

    QWidget *mWidget = new QWidget;

    QVBoxLayout *vBoxLayout = new QVBoxLayout;

    rayCasting = new RayCasting;
    vBoxLayout->addWidget(rayCasting);

    mWidget->setLayout(vBoxLayout);
    setCentralWidget(mWidget);

    connect(this, &MainWindow::readVolumeDataFinished,
            this, &MainWindow::updateRayCasting);

    // 在后面一点执行，避免出现多线程执行完了，但是还没有初始化完 slots 的情况
    readDataProcess = QtConcurrent::run(this, &MainWindow::readData);
}

MainWindow::~MainWindow() {
    delete rawReader;
    delete rayCasting;
}

void MainWindow::readSettings() {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = screen()->availableGeometry();
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}
void MainWindow::writeSettings() {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::readData() {
    rawReader = new RawReader("../../data/cbct_sample_z=507_y=512_x=512.raw", Z, Y, X);
    volumeData = new VolumeData(rawReader->data(), dim, spacing, true);
    emit readVolumeDataFinished();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    writeSettings();
    event->accept();
}

void MainWindow::updateRayCasting() {
    rayCasting->setVolumeData(volumeData);
}
