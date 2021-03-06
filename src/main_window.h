#pragma once

#include <QFuture>
#include <QMainWindow>
#include <QtConcurrent>
#include <QtWidgets>
#include <functional>
#include <glm/glm.hpp>

#include "raw_reader.h"
#include "ray_casting.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
   public:
    MainWindow();
    ~MainWindow();

   protected:
    void closeEvent(QCloseEvent *event) override;

   private:
    void readData();
    void readSettings();
    void writeSettings();
    QSlider *createThresholdSlider(std::function<void(int)> callback, int initVal, int maxThreshold = 4946);

    QFuture<void> readDataProcess;
    QSlider *opacityThresholdSlider, *colorThresholdSlider;
    RawReader *rawReader;
    RayCasting *rayCasting;
    VolumeData *volumeData;
    const int Z = 507, Y = 512, X = 512;
    glm::ivec3 dim{Z, Y, X};
    glm::vec3 spacing{0.3f, 0.3f, 0.3f};
   signals:
    void readVolumeDataFinished();
   private slots:
    void updateRayCasting();
};
