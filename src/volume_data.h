#pragma once
#include <glm/glm.hpp>
#include <limits>

class VolumeData {
   public:
    VolumeData(const unsigned short* data, const glm::ivec3 dim, const glm::vec3 spacing, const bool reverseGradientDirection, bool normalize = false)
        : data(data),
          dim(dim),
          spacing(spacing),
          reverseGradientDirection(reverseGradientDirection) {
        if (normalize) {
            DATA_MIN = std::numeric_limits<unsigned short>::max();
            DATA_MAX = std::numeric_limits<unsigned short>::min();
            for (int i = 0; i < dim[0]; i++) {
                for (int j = 0; j < dim[1]; j++) {
                    for (int k = 0; k < dim[2]; k++) {
                        DATA_MIN = std::min(DATA_MIN, data[i * dim[1] * dim[2] + j * dim[2] + k]);
                        DATA_MAX = std::max(DATA_MAX, data[i * dim[1] * dim[2] + j * dim[2] + k]);
                    }
                }
            }
            this->normailzedData = new float[dim[0] * dim[1] * dim[2]];
            for (int i = 0; i < dim[0]; i++) {
                for (int j = 0; j < dim[1]; j++) {
                    for (int k = 0; k < dim[2]; k++) {
                        this->normailzedData[i * dim[1] * dim[2] + j * dim[2] + k] = (float)(data[i * dim[1] * dim[2] + j * dim[2] + k] - DATA_MIN) / (DATA_MAX - DATA_MIN);
                    }
                }
            }
        }

        std::cout << "VolumeData initialized" << std::endl;
    }
    ~VolumeData() {
        delete[] this->normailzedData;
    }

    const unsigned short* data;
    unsigned short DATA_MIN, DATA_MAX;
    // 归一化到 0-1 之间之后的数据
    float* normailzedData = nullptr;
    glm::ivec3 dim;
    glm::vec3 spacing{1.f, 1.f, 1.f};
    bool reverseGradientDirection = false;
};
