#ifndef LEAST_SQUARES_H
#define LEAST_SQUARES_H

#include <vector>
#include <stdexcept>

class LeastSquaresSolver {
public:
    // 转换矩阵类型：4x4矩阵，按行存储
    using TransformationMatrix = std::vector<std::vector<double>>;

    // 3D点类型
    struct Point3D {
        double x, y, z;
        Point3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    };

    /**
     * 计算从模型点到实际点的转换矩阵
     * @param model_points 模型坐标点集
     * @param actual_points 实际坐标点集
     * @return 4x4齐次变换矩阵
     */
    static TransformationMatrix computeTransformation(
        const std::vector<Point3D>& model_points,
        const std::vector<Point3D>& actual_points);

    /**
     * 使用转换矩阵变换点坐标
     * @param transform 4x4变换矩阵
     * @param point 输入点坐标
     * @return 变换后的点坐标
     */
    static Point3D transformPoint(
        const TransformationMatrix& transform,
        const Point3D& point);

    /**
     * 计算变换的平均误差
     * @param model_points 模型点集
     * @param actual_points 实际点集
     * @param transform 变换矩阵
     * @return 平均误差
     */
    static double computeError(
        const std::vector<Point3D>& model_points,
        const std::vector<Point3D>& actual_points,
        const TransformationMatrix& transform);

    /**
     * 创建单位矩阵
     */
    static TransformationMatrix createIdentityMatrix();

    /**
     * 验证矩阵是否为有效的4x4变换矩阵
     */
    static bool isValidMatrix(const TransformationMatrix& matrix);

private:
    // 内部使用Eigen的实现，对外隐藏
    static TransformationMatrix computeTransformationImpl(
        const std::vector<Point3D>& model_points,
        const std::vector<Point3D>& actual_points);
};

#endif // LEAST_SQUARES_H
