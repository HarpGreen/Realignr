#include "OM5A.h"

OM5A::OM5A(OM5A_Mill_Type type, 
        OM5A_Axis_Property *axisProps, 
        OM5A_Rotation_Property rotationProp,
        double *initialPos) : millType(type), rotationProps(rotationProp)
{
    // 初始化轴属性
    for(int i = 0; i < 5; ++i)
    {
        axes[i] = axisProps[i];
    }

    // 初始化轴位置
    for(int i = 0; i < 5; ++i)
    {
        pos[i] = initialPos[i];
    }

    // 设置T2C变换矩阵
    setupT2C();
}

int OM5A::setupT2C()
{
    if(millType == OM5A_Mill_Type_XYZAC)
    {
        // 清空并重新设置T2C向量
        T2C.clear();
        T2C.resize(3);
        
        // T2C[0]: 从机床坐标系到摇篮旋转中心
        T2C[0] = Eigen::Affine3d::Identity();
        T2C[0].translation() = Eigen::Vector3d(rotationProps.Cx, rotationProps.Cy, rotationProps.Cz);
        T2C[0].rotate(Eigen::AngleAxisd(pos[3] * TO_RAD, Eigen::Vector3d::UnitX())); // A轴旋转

        // T2C[1]: 从摇篮旋转中心到转盘旋转中心
        T2C[1] = Eigen::Affine3d::Identity();
        T2C[1].translation() = Eigen::Vector3d(rotationProps.Dx, rotationProps.Dy, rotationProps.Dz);
        T2C[1].rotate(Eigen::AngleAxisd(pos[4] * TO_RAD, Eigen::Vector3d::UnitZ())); // C轴旋转

        // T2C[2]: 恒等变换，从转盘中心到转台坐标系（通常是1）
        T2C[2] = Eigen::Affine3d::Identity();
    }

    else if(millType == OM5A_Mill_Type_XYZBC)
    {
        // 清空并重新设置T2C向量
        T2C.clear();
        T2C.resize(3);
        
        // T2C[0]: 从机床坐标系到摇篮旋转中心
        T2C[0] = Eigen::Affine3d::Identity();
        T2C[0].translation() = Eigen::Vector3d(rotationProps.Cx, rotationProps.Cy, rotationProps.Cz);
        T2C[0].rotate(Eigen::AngleAxisd(pos[3] * TO_RAD, Eigen::Vector3d::UnitY())); // B轴旋转

        // T2C[1]: 从摇篮旋转中心到转盘旋转中心
        T2C[1] = Eigen::Affine3d::Identity();
        T2C[1].translation() = Eigen::Vector3d(rotationProps.Dx, rotationProps.Dy, rotationProps.Dz);
        T2C[1].rotate(Eigen::AngleAxisd(pos[4] * TO_RAD, Eigen::Vector3d::UnitZ())); // C轴旋转

        // T2C[2]: 恒等变换，从转盘中心到转台坐标系（通常是1）
        T2C[2] = Eigen::Affine3d::Identity();
    }

    else
    {
        // 不支持的机床类型
        return -1; // 错误
    }
    

    

    return 0; // 成功
}

std::vector<Eigen::Affine3d> OM5A::getC2T()
{
    std::vector<Eigen::Affine3d> C2T(T2C.size());
    for(int i = 0; i < T2C.size(); ++i)
    {
        C2T[i] = T2C[i].inverse();
    }
    // 颠倒C2T vector的顺序
    std::reverse(C2T.begin(), C2T.end());
    return C2T;
}