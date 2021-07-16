#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>

class VolumeRendering {
   public:
    VolumeRendering(const unsigned short* data, const glm::ivec3 dim, const glm::vec3 spacing, const bool reverseGradientDirection = false, const bool front2Back = true);
    ~VolumeRendering();

    /**
     * 运行 Ray Casting 算法，生成一张二维图片
     * 最简单的情况，假定观察平面完全平行于体数据
     */
    std::vector<std::vector<glm::vec4>> runAlgorithm();

   private:
    const unsigned short* data;
    glm::ivec3 dim;
    glm::vec3 spacing;
    bool reverseGradientDirection = false;
    bool front2Back = true;
    unsigned short DATA_MIN = std::numeric_limits<unsigned short>::max(), DATA_MAX = std::numeric_limits<unsigned short>::min();

    inline unsigned short getData(glm::ivec3 pos) const {
        return data[pos.x * dim[1] * dim[2] + pos.y * dim[2] + pos.z];
    };
    /**
     * 传输函数，将体数据的值转换为 RGBA 值
     * 红色通道: 随着体素值线性下降 (1-0)
     * 绿色通道：随着体素值先线性上升再线性下降 (0-1-0)
     * 蓝色通道：随着体素值线性上升 (0-1)
     * 不同明度：随着体素值线性上升 (0-0.08)
     */
    glm::vec4 transferFunction(float scalarValue) const;
    glm::vec4 getTransferedData(glm::ivec3 pos) const;
};
