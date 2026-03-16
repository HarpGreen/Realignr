#include "least_squares.h"
#include "../lib/eigen-5.0.0/Eigen/Dense"
#include <cmath>

// 内部实现，使用Eigen但对外部代码不可见
LeastSquaresSolver::TransformationMatrix
LeastSquaresSolver::computeTransformationImpl(
    const std::vector<Point3D>& model_points,
    const std::vector<Point3D>& actual_points) {

    using namespace Eigen;

    size_t n = model_points.size();

    // 转换为Eigen矩阵
    MatrixXd model_mat(3, n);
    MatrixXd actual_mat(3, n);

    for (size_t i = 0; i < n; ++i) {
        model_mat(0, i) = model_points[i].x;
        model_mat(1, i) = model_points[i].y;
        model_mat(2, i) = model_points[i].z;

        actual_mat(0, i) = actual_points[i].x;
        actual_mat(1, i) = actual_points[i].y;
        actual_mat(2, i) = actual_points[i].z;
    }

    // 中心化
    Vector3d model_center = model_mat.rowwise().mean();
    Vector3d actual_center = actual_mat.rowwise().mean();

    MatrixXd model_centered = model_mat.colwise() - model_center;
    MatrixXd actual_centered = actual_mat.colwise() - actual_center;

    // 计算协方差矩阵
    Matrix3d H = model_centered * actual_centered.transpose();

    // SVD分解
    JacobiSVD<Matrix3d> svd(H, ComputeFullU | ComputeFullV);
    Matrix3d U = svd.matrixU();
    Matrix3d V = svd.matrixV();

    // 计算旋转矩阵 R = V * U^T
    Matrix3d R = V * U.transpose();

    // 确保右手坐标系
    if (R.determinant() < 0) {
        V.col(2) *= -1;
        R = V * U.transpose();
    }

    // 计算平移向量
    Vector3d t = actual_center - R * model_center;

    // 构建4x4齐次变换矩阵
    Matrix4d transform = Matrix4d::Identity();
    transform.block<3, 3>(0, 0) = R;
    transform.block<3, 1>(0, 3) = t;

    // 转换为标准vector格式
    TransformationMatrix result(4, std::vector<double>(4));
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = transform(i, j);
        }
    }

    return result;
}

// 公共接口实现
LeastSquaresSolver::TransformationMatrix
LeastSquaresSolver::computeTransformation(
    const std::vector<Point3D>& model_points,
    const std::vector<Point3D>& actual_points) {

    if (model_points.size() != actual_points.size()) {
        throw std::invalid_argument("模型点和实际点数量必须相同");
    }

    if (model_points.size() < 3) {
        throw std::invalid_argument("至少需要3对点来计算转换矩阵");
    }

    return computeTransformationImpl(model_points, actual_points);
}

LeastSquaresSolver::Point3D
LeastSquaresSolver::transformPoint(
    const TransformationMatrix& transform,
    const Point3D& point) {

    if (!isValidMatrix(transform)) {
        throw std::invalid_argument("无效的变换矩阵");
    }

    // 齐次坐标变换
    double x = transform[0][0] * point.x + transform[0][1] * point.y +
               transform[0][2] * point.z + transform[0][3];
    double y = transform[1][0] * point.x + transform[1][1] * point.y +
               transform[1][2] * point.z + transform[1][3];
    double z = transform[2][0] * point.x + transform[2][1] * point.y +
               transform[2][2] * point.z + transform[2][3];
    double w = transform[3][0] * point.x + transform[3][1] * point.y +
               transform[3][2] * point.z + transform[3][3];

    // 齐次坐标归一化
    if (std::abs(w) > 1e-10) {
        x /= w;
        y /= w;
        z /= w;
    }

    return Point3D(x, y, z);
}

double LeastSquaresSolver::computeError(
    const std::vector<Point3D>& model_points,
    const std::vector<Point3D>& actual_points,
    const TransformationMatrix& transform) {

    if (model_points.size() != actual_points.size()) {
        throw std::invalid_argument("点集数量不匹配");
    }

    if (!isValidMatrix(transform)) {
        throw std::invalid_argument("无效的变换矩阵");
    }

    double total_error = 0.0;
    size_t n = model_points.size();

    for (size_t i = 0; i < n; ++i) {
        Point3D transformed = transformPoint(transform, model_points[i]);
        Point3D actual = actual_points[i];

        double dx = transformed.x - actual.x;
        double dy = transformed.y - actual.y;
        double dz = transformed.z - actual.z;

        total_error += std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    return total_error / n;
}

LeastSquaresSolver::TransformationMatrix
LeastSquaresSolver::createIdentityMatrix() {
    TransformationMatrix identity(4, std::vector<double>(4, 0.0));
    identity[0][0] = 1.0;
    identity[1][1] = 1.0;
    identity[2][2] = 1.0;
    identity[3][3] = 1.0;
    return identity;
}

bool LeastSquaresSolver::isValidMatrix(const TransformationMatrix& matrix) {
    if (matrix.size() != 4) return false;
    for (const auto& row : matrix) {
        if (row.size() != 4) return false;
    }
    return true;
}
