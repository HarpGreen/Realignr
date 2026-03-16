#ifndef VISUALIZEPARAMS_H
#define VISUALIZEPARAMS_H

/*可视化中参数太多，绘图需求也多，
 * 比如是否画方向，是否画深度，怎么选择加工类型，这些要很多参数结构传参
 */


#include "least_squares.h"
#include <vector>
#include <QTextEdit>

// 绘图总开关
typedef struct visGlobalParams
{
    // 作废标，传入表示作废
    bool enableVisualize = true;

    // 图元开关
    bool enablePoints = true;
    bool enableVectors = true; // 只表示方向，其长度只在length不启用的时候显示
    bool enableLength = true; // 与矢量自带的长度同时存在，但表示真正的加工深度

    // 显示样式设置
    QString defaultPointColor = "red";
    QString defaultVectorColor = "red";
    

} visGlobalParams;

// 三坐标点定义
typedef LeastSquaresSolver::Point3D visPoint;
typedef QList<visPoint> visPoints;

// 六坐标矢量数据
typedef struct {visPoint ori; visPoint dst;} visVector;
typedef QList<visVector> visVectors;


#endif // VISUALIZEPARAMS_H
