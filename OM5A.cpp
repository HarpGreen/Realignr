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
    updateT2C();
}

int OM5A::updateT2C()
{
    if(millType == OM5A_Mill_Type_XYZAC)
    {
        // 清空并重新设置T2C向量
        T2C.clear();
        T2C.resize(3);
        
        // T2C[0]: 从机床坐标系到摇篮旋转中心
        T2C[0] = Eigen::Affine3d::Identity();
        T2C[0].translation() = Eigen::Vector3d(-rotationProps.Cx, -rotationProps.Cy, -rotationProps.Cz);
        T2C[0].rotate(Eigen::AngleAxisd(pos[3] * TO_RAD, Eigen::Vector3d::UnitX())); // A轴旋转

        // T2C[1]: 从摇篮旋转中心到转盘旋转中心
        T2C[1] = Eigen::Affine3d::Identity();
        T2C[1].translation() = Eigen::Vector3d(-rotationProps.Dx, -rotationProps.Dy, -rotationProps.Dz);
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
        T2C[0].translation() = Eigen::Vector3d(-rotationProps.Cx, -rotationProps.Cy, -rotationProps.Cz);
        T2C[0].rotate(Eigen::AngleAxisd(-pos[3] * TO_RAD, Eigen::Vector3d::UnitY())); // B轴旋转

        // T2C[1]: 从摇篮旋转中心到转盘旋转中心
        T2C[1] = Eigen::Affine3d::Identity();
        T2C[1].translation() = Eigen::Vector3d(-rotationProps.Dx, -rotationProps.Dy, -rotationProps.Dz);
        T2C[1].rotate(Eigen::AngleAxisd(-pos[4] * TO_RAD, Eigen::Vector3d::UnitZ())); // C轴旋转

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

void OM5A::setPos(double *newPos)
{
    for(int i = 0; i < 5; ++i)
    {
        pos[i] = newPos[i];
    }
    updateT2C();
}

void OM5A::getPos(double *outPos)
{
    for(int i = 0; i < 5; ++i)
    {
        outPos[i] = pos[i];
    }
}

Eigen::Vector3d OM5A::currentPosToC()
{
    updateT2C();

    Eigen::Affine3d combined = Eigen::Affine3d::Identity();
    for (const auto& transform : T2C) {
        combined = transform * combined;
    }

    Eigen::Vector3d p_m(pos[0], pos[1], pos[2]);
    return combined * p_m;
}

int OM5A::computePoseFromRayInC(const Eigen::Vector3d& rayOrigin_c, const Eigen::Vector3d& rayDir_c)
{
    // 归一化方向向量
    Eigen::Vector3d dir = rayDir_c.normalized();
    
    // 1. 计算旋转轴角度
    // 目标：使机床Z轴正方向（刀具指向）与 dir 对齐
    // 机床Z轴正方向在世界坐标系中为 (0,0,1)，需要将其旋转到 dir
    // 旋转轴 = (0,0,1) × dir，角度 = acos((0,0,1)·dir)
    Eigen::Vector3d z_axis(0, 0, 1);
    double dot = z_axis.dot(dir);
    double angle_rad = std::acos(std::clamp(dot, -1.0, 1.0));
    Eigen::Vector3d rot_axis = z_axis.cross(dir);
    
    // 处理奇异情况：方向与Z轴平行
    if (angle_rad < 1e-8) {
        // 方向相同，旋转角为0
        pos[3] = 0.0; // A或B轴
        pos[4] = 0.0; // C轴
    } else if (std::abs(angle_rad - PI) < 1e-8) {
        // 方向相反，绕任意垂直轴旋转180度，这里取X轴
        pos[3] = 180.0; // A或B轴
        pos[4] = 0.0;
    } else {
        // 将旋转轴分解到机床旋转轴上
        // 根据机床类型计算 A/B 和 C 轴角度
        if (millType == OM5A_Mill_Type_XYZAC) {
            // XYZAC: A绕X，C绕Z
            // 旋转矩阵 R = Rz(C) * Rx(A)
            // 我们需要 R * (0,0,1) = dir
            // 设 dir = (dx, dy, dz)
            // 则 dx = sin(A)*sin(C)
            //    dy = -sin(A)*cos(C)
            //    dz = cos(A)
            double dx = dir.x();
            double dy = dir.y();
            double dz = dir.z();
            
            double A_rad = std::acos(std::clamp(dz, -1.0, 1.0));
            double C_rad = 0.0;
            if (std::abs(std::sin(A_rad)) > 1e-8) {
                C_rad = std::atan2(dx, -dy);
            }
            pos[3] = A_rad * TO_RAD_INV; // A轴角度（度）
            pos[4] = C_rad * TO_RAD_INV; // C轴角度（度）
        } 
        else if (millType == OM5A_Mill_Type_XYZBC) {
            // XYZBC: B绕Y，C绕Z
            // 旋转矩阵 R = Rz(C) * Ry(B)
            // R*(0,0,1) = ( sin(B)*sin(C), -sin(B)*cos(C), cos(B) )
            double dx = dir.x();
            double dy = dir.y();
            double dz = dir.z();
            
            double B_rad = std::acos(std::clamp(dz, -1.0, 1.0));
            double C_rad = 0.0;
            if (std::abs(std::sin(B_rad)) > 1e-8) {
                C_rad = std::atan2(dx, -dy);
            }
            pos[3] = B_rad * TO_RAD_INV; // B轴角度（度）
            pos[4] = C_rad * TO_RAD_INV; // C轴角度（度）
        }
        else {
            return -1; // 不支持的机床类型
        }
    }
    
    // 2. 计算线性轴位置（X,Y,Z）
    // 已知射线起点在C轴盘坐标系下为 rayOrigin_c
    // 刀具尖端在C轴盘坐标系下的位置 = T2C * (X,Y,Z,1)
    // 我们需要 (X,Y,Z) 使得 T2C * (X,Y,Z,1) = rayOrigin_c
    // 即 (X,Y,Z) = C2T * rayOrigin_c
    
    // 先更新T2C以使用当前刚计算的旋转轴角度
    updateT2C();
    
    // 获取C2T变换（从C盘坐标系到机床坐标系）
    std::vector<Eigen::Affine3d> C2T = getC2T();
    Eigen::Affine3d combinedC2T = Eigen::Affine3d::Identity();
    for (const auto& t : C2T) {
        combinedC2T = t * combinedC2T;
    }
    
    // 计算机床坐标系下的刀具尖端位置
    Eigen::Vector3d p_m = combinedC2T * rayOrigin_c;
    pos[0] = p_m.x();
    pos[1] = p_m.y();
    pos[2] = p_m.z();
    
    // 可选：限制旋转轴角度范围到 [0, 360) 或 [-180, 180)
    for (int i = 3; i < 5; ++i) {
        pos[i] = std::fmod(pos[i], 360.0);
        if (pos[i] > 180.0) pos[i] -= 360.0;
        if (pos[i] < -180.0) pos[i] += 360.0;
    }
    
    // 最终更新内部变换矩阵
    updateT2C();
    
    return 0;
}