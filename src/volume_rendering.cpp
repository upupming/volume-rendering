#include "volume_rendering.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
//
#include <stb_image_write.h>

#include <ctime>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

VolumeRendering::VolumeRendering(const unsigned short* data, const glm::ivec3 dim, const glm::vec3 spacing, const bool reverseGradientDirection, const bool front2Back) {
    clock_t time = clock();

    this->data = data;
    this->dim = dim;
    this->spacing = spacing;
    this->reverseGradientDirection = reverseGradientDirection;
    this->front2Back = front2Back;

    auto size = dim.x * dim.y * dim.z;
    for (int i = 0; i < size; i++) {
        DATA_MIN = std::min(DATA_MIN, data[i]);
        DATA_MAX = std::max(DATA_MAX, data[i]);
    }

    printf("Volume Rendering initialized in %lf secs.\n", (float)(clock() - time) / CLOCKS_PER_SEC);
}
VolumeRendering::~VolumeRendering() {
}

std::vector<std::vector<glm::vec4>> VolumeRendering::runAlgorithm() {
    clock_t time = clock();
    std::vector<std::vector<glm::vec4>> imagePlane(
        dim.x,
        std::vector<glm::vec4>(
            dim.y,
            // 全黑的背景色
            {0, 0, 0, 0}));

    for (int i = 0; i < dim.x; i++) {
        for (int j = 0; j < dim.y; j++) {
            // 对于观察平面的一个像素，投射一条从 z = 0 到 z = dim.z 的光线
            float q0 = 0, q1 = dim.z;
            if (front2Back) {
                // front-to-back
                for (int k = 0; k < dim.z; k++) {
                    auto cRGBA = getTransferedData({i, j, k});
                    if (imagePlane[i][j].a > 0.95) break;

                    imagePlane[i][j].r = cRGBA.a * cRGBA.r + (1 - cRGBA.a) * imagePlane[i][j].a * imagePlane[i][j].r;
                    imagePlane[i][j].g = cRGBA.a * cRGBA.g + (1 - cRGBA.a) * imagePlane[i][j].a * imagePlane[i][j].g;
                    imagePlane[i][j].b = cRGBA.a * cRGBA.b + (1 - cRGBA.a) * imagePlane[i][j].a * imagePlane[i][j].b;

                    imagePlane[i][j].a = imagePlane[i][j].a + (1.f - imagePlane[i][j].a) * cRGBA.a;
                }
            } else {
                // back-to-front
                for (int k = dim.z - 1; k >= 0; k--) {
                    auto cRGBA = getTransferedData({i, j, k});
                    // 只需要累积颜色值即可，不需要累积不透明度
                    imagePlane[i][j] = cRGBA.a * cRGBA + (1 - cRGBA.a) * imagePlane[i][j];
                }
            }
        }
    }

    char* mem = (char*)malloc(sizeof(char) * 3 * dim.x * dim.y);
    for (int i = 0; i < dim.x; i++) {
        for (int j = 0; j < dim.y; j++) {
            mem[i * dim.y * 3 + j * 3] = imagePlane[i][j].r * 255;
            mem[i * dim.y * 3 + j * 3 + 1] = imagePlane[i][j].g * 255;
            mem[i * dim.y * 3 + j * 3 + 2] = imagePlane[i][j].b * 255;
        }
    }
    stbi_write_png("test.png", dim.x, dim.y, 3, mem, sizeof(char) * 3 * dim.y);

    printf("Volume Rendering ran in %lf secs.\n", (float)(clock() - time) / CLOCKS_PER_SEC);
    return imagePlane;
}

glm::vec4 VolumeRendering::transferFunction(float scalarValue) const {
    float ratio = (scalarValue - DATA_MIN) / (DATA_MAX - DATA_MIN);
    // 在 highlight ratio 附近的不透明度很高，其他区域基本为 0
    float highlightRatio = 0.8;
    float alpha = abs(ratio - highlightRatio) < 0.1 ? 0.9 : 0.01;
    return {
        // R
        (DATA_MAX - scalarValue) / (DATA_MAX - DATA_MIN),
        // G
        1.f - abs((DATA_MAX + DATA_MIN) / 2.f - scalarValue) * 2 / (DATA_MAX - DATA_MIN),
        // B
        (scalarValue - DATA_MIN) / (DATA_MAX - DATA_MIN),
        // A
        alpha,
    };
}
glm::vec4 VolumeRendering::getTransferedData(glm::ivec3 pos) const {
    auto val = getData(pos);
    return transferFunction(val);
}
