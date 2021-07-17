#include "main_window.h"

MainWindow::MainWindow() {
    readSettings();

    QWidget *mWidget = new QWidget;

    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    QHBoxLayout *hBoxLayout1 = new QHBoxLayout;
    QHBoxLayout *hBoxLayout2 = new QHBoxLayout;

    rayCasting = new RayCasting;
    vBoxLayout->addWidget(rayCasting, 10);

    opacityThresholdSlider = createThresholdSlider(
        [=](int value) {
            rayCasting->setOpacityThreshold(value);
        },
        rayCasting->getOpacityThreshold());
    colorThresholdSlider = createThresholdSlider(
        [=](int value) {
            rayCasting->setColorThreshold(value);
        },
        rayCasting->getColorThreshold());
    hBoxLayout1->addWidget(new QLabel("Opacity threshold (transfer function)"));
    hBoxLayout1->addWidget(opacityThresholdSlider);
    hBoxLayout2->addWidget(new QLabel("Color threshold   (transfer function)"));
    hBoxLayout2->addWidget(colorThresholdSlider);

    vBoxLayout->addLayout(hBoxLayout1, 1);
    vBoxLayout->addLayout(hBoxLayout2, 1);

    mWidget->setLayout(vBoxLayout);
    setCentralWidget(mWidget);

    connect(this, &MainWindow::readVolumeDataFinished,
            this, &MainWindow::updateRayCasting);

    // 在后面一点执行，避免出现多线程执行完了，但是还没有初始化完 slots 的情况
    readDataProcess = QtConcurrent::run(this, &MainWindow::readData);
}

QSlider *MainWindow::createThresholdSlider(std::function<void(int)> callback, int initVal, int maxThreshold) {
    QSlider *slider = new QSlider;
    slider->setOrientation(Qt::Orientation::Horizontal);
    slider->setFocusPolicy(Qt::StrongFocus);
    slider->setTickPosition(QSlider::TicksBothSides);
    slider->setTickInterval(100);
    slider->setSingleStep(1);
    slider->setMaximum(maxThreshold);
    slider->setValue(initVal);
    connect(slider, &QSlider::valueChanged,
            this, callback);
    return slider;
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
