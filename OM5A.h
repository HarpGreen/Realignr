#ifndef ORTHOGONALMILL5AXES_H
#define ORTHOGONALMILL5AXES_H

#include <string>
#include "../lib/eigen-5.0.0/Eigen/Dense"

#define PI      3.14159265358979323846
#define TO_RAD (PI/180.)
#define TO_RAD_INV (180./PI)

// 单轴属性定义
typedef struct OM5A_Axis_Property
{
    std::string axisLabel;   //轴名称,用户输入的文本去掉正负号的结果，比如输入Z2=那生成的时候就是Z2=0.123这样
    bool isReversed;   //是否反向(应对机械结构装反了的情况)

    OM5A_Axis_Property() : axisLabel(""), isReversed(false) {};
} OM5A_Axis_Property;

// 转台旋转中心属性定义
typedef struct OM5A_Rotation_Property
{
    // 摇篮的A或者B旋转轴上，与c轴空间最近点位置
    double Cx;
    double Cy;
    double Cz;

    // 与2025年写在笔记的版本相反，现在完全按正向
    // 顺序定义平移向量的方向，即从ab的中心指向c中心，
    // 以前的是反过来的
    double Dx;
    double Dy;
    double Dz;

    OM5A_Rotation_Property() : Cx(0), Cy(0), Cz(0), Dx(0), Dy(0), Dz(0) {};

} OM5A_Rotation_Property;

// 机床形制定义
typedef enum {
    OM5A_Mill_Type_XYZAC = 0x00,
    OM5A_Mill_Type_XYZBC
} OM5A_Mill_Type;


//
//
// 机床总体，包含了机床类型和轴属性，以及机床中的坐标变换方法
class OM5A
{

public:
/*用户提供的机床结构信息*/

    OM5A_Mill_Type millType; //机床类型，决定了旋转轴的定义方式

    OM5A_Axis_Property axes[5]; // 5轴属性，axes[0]和axes[1]axes[2]是线性轴、axes[3]、axes[4]、axes[5]是旋转轴
                                // 用户输入的轴名称其实只影响输出时候的轴标签，计算时仍然按照固定的顺序来处理，
                                // 比如axes[3]永远是a旋转轴，axes[4]永远是b旋转轴，这样就不需要在计算过程中反复分析用户输入的轴定义字符串了

    OM5A_Rotation_Property rotationProps; // 旋转中心信息

public:
/*机床各轴所处位置*/
    double pos[5]; // 5轴位置，单位mm或者度，线性轴是mm，旋转轴是度

    void setPos(double *newPos); // 更新轴位置，输入是一个长度为5的数组，单位同上
    void getPos(double *outPos); // 获取轴位置，输出是一个长度为5的数组，单位同上

public:
/*仿射变换*/

    std::vector<Eigen::Affine3d> T2C; // 机床坐标系到转台坐标系的一系列变换矩阵
        // 对于摇篮式，T2C[0]是坐标系从tool即机床坐标系平移到A/B轴旋转中心，并跟随A/B旋转的变换矩阵；
        // T2C[1]是下一步，从B轴旋转中心到c转盘中心，并跟随C轴旋转的变换矩阵；
        // T2C[2]是下一步，从c转盘中心到转台上的坐标系（通常是1，只是备用）；
    //    
    // T2C而不是T2WP，因为不包括从转台到工件坐标系的变换矩阵，因为这个变换矩阵是
    // 根据工件实际放置位置计算的，不是机床属性，应由用户用各种配准方法来确定C2WP


    int updateT2C(); // 根据用户输入的机床结构信息，计算出T2C中的变换矩阵
    std::vector<Eigen::Affine3d> getT2C() { return T2C; } // 获取T2C矩阵组
    std::vector<Eigen::Affine3d> getC2T(); // 获取C2T矩阵组，是T2C的逆矩阵并且顺序相反

    Eigen::Vector3d currentPosToC(); // 将当前 pos[0..2] 位置按当前 pos[3..4] 变换到 C 轴盘坐标系

    /* 根据C轴盘坐标系下的射线（起点+方向）计算机床位置
       输入: rayOrigin_c - C轴盘坐标系下的射线起点 (mm)
             rayDir_c   - C轴盘坐标系下的射线方向 (单位向量)
       输出: 更新 pos[5] 使刀具位于起点且Z轴指向射线方向
       返回: 0 成功, -1 失败（如奇异点） */
    int computePoseFromRayInC(const Eigen::Vector3d& rayOrigin_c, const Eigen::Vector3d& rayDir_c);

/*构*/
    OM5A(OM5A_Mill_Type type, 
        OM5A_Axis_Property *axisProps, 
        OM5A_Rotation_Property rotationProp,
        double *initialPos);
    ~OM5A() {};
};






#endif // ORTHOGONALMILL5AXES_H
