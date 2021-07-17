#include "ray_casting.h"

// https://www.codenong.com/cs106436180/
static void GLClearError() {
    while (glGetError() != GL_NO_ERROR)
        ;
}
static void GLCheckError() {
    while (GLenum error = glGetError()) {
        std::cout << "OpenGL Error(" << error << ")" << std::endl;
    }
}

RayCasting::RayCasting(VolumeData* volumeData) : indexBuf(QOpenGLBuffer::IndexBuffer) {
    setVolumeData(volumeData);
}

RayCasting::~RayCasting() {
    arrayBuf.destroy();
    indexBuf.destroy();
}

void RayCasting::setVolumeData(VolumeData* volumeData) {
    this->volumeData = volumeData;
    if (volumeData != nullptr) {
        std::cout << "binding texture 3D image" << std::endl;
        // 重新绑定 3D 纹理
        glDeleteTextures(1, &volumeTexture);
        glGenTextures(1, &volumeTexture);
        glBindTexture(GL_TEXTURE_3D, volumeTexture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // 注意 depth 是最外面一层
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R16UI, volumeData->dim[1], volumeData->dim[2], volumeData->dim[0], 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, volumeData->data);
        glGenerateMipmap(GL_TEXTURE_3D);
        glBindTexture(GL_TEXTURE_3D, 0);

        update();
    }
}

void RayCasting::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF(), backgroundColor.alphaF());

    initShaders();

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    // Disable back face culling
    glDisable(GL_CULL_FACE);

    if (!arrayBuf.create()) {
        std::cout << "arrayBuf create failed" << std::endl;
    }
    if (!arrayBuf.bind()) {
        std::cout << "arrayBuf bind failed" << std::endl;
    }
    arrayBuf.allocate(vertices, sizeof(vertices));

    if (!indexBuf.create()) {
        std::cout << "indexBuf create failed" << std::endl;
    }
    if (!indexBuf.bind()) {
        std::cout << "indexBuf bind failed" << std::endl;
    }
    indexBuf.allocate(indices, sizeof(indices));
}
void RayCasting::resizeGL(int w, int h) {
}
void RayCasting::paintGL() {
    if (!volumeData) return;

    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 model, view, projection;

    auto physicalSize = glm::vec3(volumeData->dim) * volumeData->spacing;
    auto identityCubeSize = physicalSize / std::max({physicalSize.x, physicalSize.y, physicalSize.z});
    // 将边长为 2 的立方体变成满足 ratio2LogestSide 比例的长方体
    QVector3D sideLen = QVector3D(identityCubeSize.x, identityCubeSize.y, identityCubeSize.z) / 2.f;
    model.scale(sideLen);

    view.rotate(QQuaternion(curr_quat[3], -curr_quat[0], -curr_quat[1], -curr_quat[2]));
    auto lookAtMatrix = glm::lookAt(eye, lookat, up);
    auto p = glm::value_ptr(lookAtMatrix);
    view = QMatrix4x4(p).transposed() * view;

    float aspectRatio = (float)width() / height();
    projection.perspective(fov, aspectRatio, 0.1f, 100.0f);

    auto mvpMatrix = projection * view * model;
    program.setUniformValue("modelMatrix", model);
    program.setUniformValue("viewMatrix", view);
    program.setUniformValue("projectionMatrix", projection);
    program.setUniformValue("mvpMatrix", mvpMatrix);

    program.setUniformValue("viewportSize", QVector2D{(float)width(), (float)height()});
    // 物体进行变换等价于视点进行逆变换
    program.setUniformValue("rayOrigin", view.inverted() * QVector3D({0.0, 0.0, 0.0}));
    program.setUniformValue("aspectRatio", aspectRatio);
    program.setUniformValue("focalLength", focalLength);

    program.setUniformValue("top", sideLen);
    program.setUniformValue("bottom", -sideLen);
    program.setUniformValue("stepLength", 0.001f);
    program.setUniformValue("backgroundColor", QVector3D(backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF()));
    program.setUniformValue("gamma", 2.2f);

    program.setUniformValue("normalMatrix", (view * model).normalMatrix());
    program.setUniformValue("light.position", 1.2f, 1.0f, 2.0f);
    program.setUniformValue("light.ambient", 0.1f, 0.1f, 0.1f, 0.1f);
    program.setUniformValue("light.diffuse", 1.0f, 1.0f, 1.0f, 1.0f);
    program.setUniformValue("light.specular", 1.0f, 1.0f, 1.0f, 1.0f);
    // material properties
    program.setUniformValue("material.specular", 1.f, 1.f, 1.f, 0.f);
    program.setUniformValue("material.shininess", 32.0f);
    program.setUniformValue("reverseGradient", volumeData->reverseGradientDirection);

    program.setUniformValue("opacityThreshold", opacityThreshold);
    program.setUniformValue("colorThreshold", colorThreshold);

    // Tell OpenGL which VBOs to use
    if (!arrayBuf.bind()) {
        std::cout << "arrayBuf bind failed" << std::endl;
    }
    if (!indexBuf.bind()) {
        std::cout << "indexBuf bind failed" << std::endl;
    }
    program.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);
    program.enableAttributeArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, volumeTexture);

    // glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    GLCheckError();
    // type: Must be one of GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, or GL_UNSIGNED_INT.
    glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(int), GL_UNSIGNED_INT, nullptr);
    GLCheckError();
}

void RayCasting::initShaders() {
    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/alpha_blending.vs"))
        close();

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/alpha_blending.fs"))
        close();

    // Link shader pipeline
    if (!program.link())
        close();

    // Bind shader pipeline for use
    if (!program.bind())
        close();
}

QPointF RayCasting::pixel_pos_to_view_pos(const QPointF& p) {
    return QPointF(2.0 * float(p.x()) / width() - 1.0,
                   1.0 - 2.0 * float(p.y()) / height());
}

void RayCasting::mousePressEvent(QMouseEvent* e) {
    // Save mouse press position
    prevMouse = e->localPos();
    if (e->button() == Qt::MouseButton::LeftButton) {
        mouseLeftPressed = true;
        trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
    } else if (e->button() == Qt::MouseButton::MiddleButton) {
        mouseMiddlePressed = true;
    } else if (e->button() == Qt::MouseButton::RightButton) {
        mouseRightPressed = true;
    }
}

void RayCasting::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::MouseButton::LeftButton) {
        mouseLeftPressed = false;
    } else if (e->button() == Qt::MouseButton::MiddleButton) {
        mouseMiddlePressed = false;
    } else if (e->button() == Qt::MouseButton::RightButton) {
        mouseRightPressed = false;
    }
}
void RayCasting::mouseMoveEvent(QMouseEvent* e) {
    float rotScale = 1.0f;
    float transScale = 2.0f;

    auto mouse = e->localPos();
    // 左键旋转
    if (mouseLeftPressed) {
        trackball(
            prev_quat,
            rotScale * (2.0f * prevMouse.x() - width()) / (float)width(),
            rotScale * (height() - 2.0f * prevMouse.y()) / (float)height(),
            rotScale * (2.0f * mouse.x() - width()) / (float)width(),
            rotScale * (height() - 2.0f * mouse.y()) / (float)height());

        add_quats(prev_quat, curr_quat, curr_quat);
    }
    // 中间键移动
    else if (mouseMiddlePressed) {
        eye[0] -= transScale * (mouse.x() - prevMouse.x()) / (float)width();
        lookat[0] -= transScale * (mouse.x() - prevMouse.x()) / (float)width();
        eye[1] += transScale * (mouse.y() - prevMouse.y()) / (float)height();
        lookat[1] += transScale * (mouse.y() - prevMouse.y()) / (float)height();
    }
    // 右键缩放
    else if (mouseRightPressed) {
        eye[2] += transScale * (mouse.y() - prevMouse.y()) / (float)height();
        lookat[2] += transScale * (mouse.y() - prevMouse.y()) / (float)height();
    }
    prevMouse = mouse;
    // Request an update
    update();
}
