#pragma once
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QtMath>
#include <QtWidgets>
#include <glm/glm.hpp>
#include <iostream>

#include "trackball.h"
#include "volume_data.h"

class RayCasting : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    Q_OBJECT
   public:
    explicit RayCasting(VolumeData* volumeData = nullptr);
    ~RayCasting();
    void setVolumeData(VolumeData* volumeData);

   protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void initShaders();

   private:
    QPointF pixel_pos_to_view_pos(const QPointF& p);

    VolumeData* volumeData;
    GLuint volumeTexture = 0;
    QColor backgroundColor = QColor(41, 65, 71);

    QOpenGLShaderProgram program;
    QMatrix4x4 projection;
    float zNear = 0.1f, zFar = 100.0f, fov = 60.0f;
    float focalLength = 1.0 / qTan(M_PI / 180.0 * fov / 2.0);

    QPointF prevMouse;
    bool mouseLeftPressed = false, mouseRightPressed = false, mouseMiddlePressed = false;
    TrackBall trackball;
    TrackBall sceneTrackball;
    // model 放在原点并且缩放到 [0, 1] 区间，我们从 z=3 往 -z 方向看，确保能够看到 model 整体全貌，不管它有多大
    glm::vec3 eye = {0, 0, 3},
              lookat = {0, 0, 0}, up = {0, 1, 0};

    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;

    // 边长为 2，正中心在原点的一个立方体
    float vertices[24] = {
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        //
    };
    int indices[36] = {
        // front
        0, 1, 2,
        0, 2, 3,
        // right
        1, 5, 6,
        1, 6, 2,
        // back
        5, 4, 7,
        5, 7, 6,
        // left
        4, 0, 3,
        4, 3, 7,
        // top
        2, 6, 7,
        2, 7, 3,
        // bottom
        4, 5, 1,
        4, 1, 0,
        //
    };
};
