#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "least_squares.h"
#include "icp.h"
#include "OM5A.h" // 包含机械轴属性定义,应对机械结构装反的情况

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <vector>

constexpr int VERSION[3] = {1,1,0};

//点定义
using Point3D = LeastSquaresSolver::Point3D;

// 5轴机床实际点类型，用于输入的机床坐标(x,y,z,a,c)
struct MachinePoint5D {
    double x, y, z, a, c;
    MachinePoint5D(double x = 0, double y = 0, double z = 0, double a = 0, double c = 0)
        : x(x), y(y), z(z), a(a), c(c) {}
};

//加工点定义，包含加工方向和其他加工信息
struct ProcessPoint {
    double x, y, z;
    double i, j, k;                 //加工方向单位向量
    double duration;                //加工时间，由用户在软件之外自行算出，比如excel
    // double diameter;                //加工直径，后续可以根据这个值来调整加工点位置，确保刀具中心在正确的位置
    int processMethodSelection;     //选择用户输入的加工过程，<0代表没有加工过程，
                                    //==0代表默认G04等待，>1的数表示选择用户输入的内容
    QString comment;                //备注，保留在点加工后面，以便在加工过程中判断是哪个点
};

struct ProcessPointJoint {
    double pos[5];
    double duration;                //加工时间，由用户在软件之外自行算出，比如excel
    int processMethodSelection;     //选择用户输入的加工过程，<0代表没有加工过程，
                                    //==0代表默认G04等待，>1的数表示选择用户输入的内容
    QString comment;               // 注释部分
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 机床参数Tab槽函数
    void onSaveMachineParametersClicked();  //保存机床参数并解锁后续Tab
    void onMachineTypeChanged();            //机床形式改变时更新默认轴名称

    // 坐标映射Tab槽函数
    void onModelImportClicked();            //理论参考点坐标导入
    void onModelClearClicked();
    void onActualImportClicked();           //实测参考点坐标导入
    void onActualClearClicked();
    void onCalculateClicked();              //计算矩阵键
    void onVerifyClicked();                 //验证计算键

    // 代码生成Tab槽函数
    void onProcessPointsImportClicked();    //加工点导入键
    void onProcessPointsClearClicked();
    void onGenerateCodeClicked();           //开始生成G代码键

    // 可视化验证Tab槽函数
    void onVisualizeClicked();              //可视化验证

private:
    // 机床
    OM5A *mill_p;
    bool machineParamsSaved;  // 机床参数是否已保存

    void appendInfoMessage(const QString& message);  // 添加信息消息到输出框

private:
    void setupMachineParametersTab();       //绘制 机床参数 Tab
    void setupCoordinateMappingTab();       //绘制 坐标映射 Tab
    void setupTransformationMatrixTable();  //绘制 转换矩阵显示框
    void setupCodeGenerationTab();          //绘制 代码生成 Tab
    void setupVisualizationTab();           //绘制 可视化验证 Tab

    ///////////////////////////// 机床参数Tab组件
    QComboBox *machineTypeCombo;
    QLineEdit *machineAxisLabels;
    QLineEdit *rotationCenterCX;
    QLineEdit *rotationCenterCY;
    QLineEdit *rotationCenterCZ;
    QLineEdit *rotationCenterDX;
    QLineEdit *rotationCenterDY;
    QLineEdit *rotationCenterDZ;
    QPushButton *saveMachineParamsBtn;

    ///////////////////////////// 坐标映射Tab组件
    QTabWidget *tabWidget;

    // 左侧模型坐标区域
    QTextEdit *modelPointsText;
    QPushButton *modelImportBtn;
    QPushButton *modelClearBtn;

    // 右侧实际坐标区域
    QTextEdit *actualPointsText;
    QPushButton *actualImportBtn;
    QPushButton *actualClearBtn;

    // 计算区域
    QComboBox *algorithmCombo;
    QPushButton *calculateBtn;
    QTableWidget *matrixTable;
    QLineEdit *verifyInput;
    QPushButton *verifyBtn;
    QTextEdit *infoOutput;


    ///////////////////////////// 代码生成Tab组件
    QTextEdit *processPointsText;
    QPushButton *processImportBtn;
    QPushButton *processClearBtn;
    QLineEdit *fileNameInput;
    QPushButton *generateBtn;

    ///////////////////////////// 可视化验证Tab组件
    QPushButton *visualizeBtn;

    // 机床轴标签解析
    bool parseAxisLabel(const QString& raw, OM5A_Axis_Property &prop);

    // 用户输入数据
    std::vector<Point3D> modelPoints;
    std::vector<MachinePoint5D> actualMachinePoints; // 用户输入的5轴机床实际坐标
    std::vector<ProcessPoint> processPoints;         // 用户输入的加工点坐标和加工信息
    // 处理后数据
    std::vector<Point3D> actualPoints;               // C转盘坐标系下的实际点
    std::vector<ProcessPointJoint> processPointJoints; // 加工点关节信息

    // 转换矩阵（在五轴情况下指的是工件/模型坐标系转换到C转盘坐标系下的转换矩阵，即WP2C）
    LeastSquaresSolver::TransformationMatrix transformationMatrix;

    // 工具函数
    std::vector<Point3D> parsePointsFromText(const QString& text);      //多行解析点
    std::vector<MachinePoint5D> parseMachinePointsFromText(const QString& text); // 解析5轴机床实际点
    std::vector<ProcessPoint> parseProcessPoints(const QString& text);  //多行解析加工点
    QString cleanCoordinateString(const QString& input);

    // 导入功能辅助函数
    bool importPointsFromCSV(const QString& fileName, QTextEdit* textEdit, std::vector<Point3D>& points);
    bool importMachinePointsFromCSV(const QString& fileName, QTextEdit* textEdit, std::vector<MachinePoint5D>& points);
    bool importProcessPointsFromCSV(const QString& fileName);

    // Gcode生成
    QString generateGCode(const std::vector<ProcessPoint>& points); // 生成G代码，同时更新处理后的加工点坐标到processPointsAfter中
    QString generateGCodeLine(const ProcessPointJoint& joint);
};

#endif // MAINWINDOW_H
