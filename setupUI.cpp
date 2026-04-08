#include "mainwindow.h"
#include <QHeaderView>
#include <QMessageBox>
#include <sstream>
#include <iomanip>

//************ TAB： MACHINE PARAMS *****************

void MainWindow::setupMachineParametersTab()
{
    QWidget *machineTab = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(machineTab);

    // 轴定义区域
    QGroupBox *axisGroup = new QGroupBox("轴定义");
    QVBoxLayout *axisLayout = new QVBoxLayout(axisGroup);

    axisLayout->addWidget(new QLabel("机床形式:"));
    machineTypeCombo = new QComboBox;
    machineTypeCombo->addItem("请选择机床形式");
    machineTypeCombo->addItem("XYZAC");
    machineTypeCombo->addItem("XYZBC");
    machineTypeCombo->setCurrentIndex(0);
    axisLayout->addWidget(machineTypeCombo);

    axisLayout->addWidget(new QLabel("机床轴名称:"));
    machineAxisLabels = new QLineEdit;
    machineAxisLabels->setText("x,y,z,a,c");
    machineAxisLabels->setPlaceholderText("输入轴名称，如: x,y,z,a,c");
    axisLayout->addWidget(machineAxisLabels);

    // 旋转中心区域
    QGroupBox *rotationGroup = new QGroupBox("五轴旋转中心");
    QVBoxLayout *rotationLayout = new QVBoxLayout(rotationGroup);

    // 创建左右两列布局
    QHBoxLayout *columnsLayout = new QHBoxLayout;

    // 左侧列：cx, cy, cz
    QVBoxLayout *leftColumn = new QVBoxLayout;
    
    QHBoxLayout *cxLayout = new QHBoxLayout;
    cxLayout->addWidget(new QLabel("CX:"));
    rotationCenterCX = new QLineEdit;
    rotationCenterCX->setText("0.0");
    rotationCenterCX->setPlaceholderText("旋转中心CX");
    cxLayout->addWidget(rotationCenterCX);
    leftColumn->addLayout(cxLayout);

    QHBoxLayout *cyLayout = new QHBoxLayout;
    cyLayout->addWidget(new QLabel("CY:"));
    rotationCenterCY = new QLineEdit;
    rotationCenterCY->setText("0.0");
    rotationCenterCY->setPlaceholderText("旋转中心CY");
    cyLayout->addWidget(rotationCenterCY);
    leftColumn->addLayout(cyLayout);

    QHBoxLayout *czLayout = new QHBoxLayout;
    czLayout->addWidget(new QLabel("CZ:"));
    rotationCenterCZ = new QLineEdit;
    rotationCenterCZ->setText("0.0");
    rotationCenterCZ->setPlaceholderText("旋转中心CZ");
    czLayout->addWidget(rotationCenterCZ);
    leftColumn->addLayout(czLayout);

    // 右侧列：dx, dy, dz
    QVBoxLayout *rightColumn = new QVBoxLayout;

    QHBoxLayout *dxLayout = new QHBoxLayout;
    dxLayout->addWidget(new QLabel("DX:"));
    rotationCenterDX = new QLineEdit;
    rotationCenterDX->setText("0.0");
    rotationCenterDX->setPlaceholderText("旋转中心DX");
    dxLayout->addWidget(rotationCenterDX);
    rightColumn->addLayout(dxLayout);

    QHBoxLayout *dyLayout = new QHBoxLayout;
    dyLayout->addWidget(new QLabel("DY:"));
    rotationCenterDY = new QLineEdit;
    rotationCenterDY->setText("0.0");
    rotationCenterDY->setPlaceholderText("旋转中心DY");
    dyLayout->addWidget(rotationCenterDY);
    rightColumn->addLayout(dyLayout);

    QHBoxLayout *dzLayout = new QHBoxLayout;
    dzLayout->addWidget(new QLabel("DZ:"));
    rotationCenterDZ = new QLineEdit;
    rotationCenterDZ->setText("0.0");
    rotationCenterDZ->setPlaceholderText("旋转中心DZ");
    dzLayout->addWidget(rotationCenterDZ);
    rightColumn->addLayout(dzLayout);

    // 添加左右两列到主布局
    columnsLayout->addLayout(leftColumn);
    columnsLayout->addLayout(rightColumn);
    rotationLayout->addLayout(columnsLayout);

    // 添加到主布局
    mainLayout->addWidget(axisGroup);
    mainLayout->addWidget(rotationGroup);

    saveMachineParamsBtn = new QPushButton("保存参数");
    saveMachineParamsBtn->setMinimumHeight(36);
    mainLayout->addWidget(saveMachineParamsBtn);
    mainLayout->addStretch();

    tabWidget->addTab(machineTab, "机床参数");
}


//************ TAB： COORDS MAP *****************

void MainWindow::setupCoordinateMappingTab()

{
    QWidget *coordTab = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(coordTab);

    // 左侧模型坐标区域
    QGroupBox *modelGroup = new QGroupBox("模型坐标点");
    QVBoxLayout *modelLayout = new QVBoxLayout(modelGroup);

    modelPointsText = new QTextEdit;
    modelPointsText->setPlaceholderText("输入模型坐标点，每行一个点，格式: x,y,z\n例如:\n0,0,0\n100,0,0\n0,100,0");
    modelLayout->addWidget(modelPointsText); // 去掉高度限制

    QHBoxLayout *modelBtnLayout = new QHBoxLayout;
    modelImportBtn = new QPushButton("导入CSV");
    modelClearBtn = new QPushButton("清空");
    modelBtnLayout->addWidget(modelImportBtn);
    modelBtnLayout->addWidget(modelClearBtn);
    modelLayout->addLayout(modelBtnLayout);

    // 右侧实际坐标区域
    QGroupBox *actualGroup = new QGroupBox("实际坐标点");
    QVBoxLayout *actualLayout = new QVBoxLayout(actualGroup);

    actualPointsText = new QTextEdit;
    actualPointsText->setPlaceholderText("输入实际坐标点，每行一个点，格式: x,y,z\n例如:\n0.1,0.2,0.05\n99.9,0.3,0.05\n0.2,100.1,0.05");
    actualLayout->addWidget(actualPointsText); // 去掉高度限制

    QHBoxLayout *actualBtnLayout = new QHBoxLayout;
    actualImportBtn = new QPushButton("导入CSV");
    actualClearBtn = new QPushButton("清空");
    actualBtnLayout->addWidget(actualImportBtn);
    actualBtnLayout->addWidget(actualClearBtn);
    actualLayout->addLayout(actualBtnLayout);

    // 计算和验证区域 - 给更多空间
    QGroupBox *calcGroup = new QGroupBox("计算与验证");
    QVBoxLayout *calcLayout = new QVBoxLayout(calcGroup);

    // 算法选择行
    QHBoxLayout *algoLayout = new QHBoxLayout;
    algoLayout->addWidget(new QLabel("算法:"));
    algorithmCombo = new QComboBox;
    algorithmCombo->addItem("最小二乘 Ax=B");
    algorithmCombo->addItem("点云精配准 ICP");
    algoLayout->addWidget(algorithmCombo);
    calculateBtn = new QPushButton("计算转换矩阵");
    algoLayout->addWidget(calculateBtn);
    algoLayout->addStretch();
    calcLayout->addLayout(algoLayout);

    // 转换矩阵表格 - 紧凑但不限制大小
    calcLayout->addWidget(new QLabel("转换矩阵:"));
    setupTransformationMatrixTable();
    calcLayout->addWidget(matrixTable);

    // 验证区域 - 充分利用垂直空间
    calcLayout->addWidget(new QLabel("验证坐标输入:"));
    QHBoxLayout *verifyInputLayout = new QHBoxLayout;
    verifyInput = new QLineEdit;
    verifyInput->setPlaceholderText("输入模型坐标，格式: x,y,z");
    verifyBtn = new QPushButton("验证");
    verifyInputLayout->addWidget(verifyInput);
    verifyInputLayout->addWidget(verifyBtn);
    calcLayout->addLayout(verifyInputLayout);

    calcLayout->addWidget(new QLabel("验证结果:"));
    verifyResult = new QTextEdit;
    verifyResult->setReadOnly(true);
    calcLayout->addWidget(verifyResult); // 去掉高度限制，让它自动扩展

    // 添加到主布局，调整比例
    mainLayout->addWidget(modelGroup, 1);    // 左侧模型区域占1份
    mainLayout->addWidget(actualGroup, 1);   // 中间实际区域占1份
    mainLayout->addWidget(calcGroup, 2);     // 右侧计算区域占2份

    tabWidget->addTab(coordTab, "坐标映射");

    // 连接信号槽
    connect(modelImportBtn, &QPushButton::clicked, this, &MainWindow::onModelImportClicked);
    connect(modelClearBtn, &QPushButton::clicked, this, &MainWindow::onModelClearClicked);
    connect(actualImportBtn, &QPushButton::clicked, this, &MainWindow::onActualImportClicked);
    connect(actualClearBtn, &QPushButton::clicked, this, &MainWindow::onActualClearClicked);
    connect(calculateBtn, &QPushButton::clicked, this, &MainWindow::onCalculateClicked);
    connect(verifyBtn, &QPushButton::clicked, this, &MainWindow::onVerifyClicked);
}

void MainWindow::setupTransformationMatrixTable()
{
    matrixTable = new QTableWidget(4, 4);
    // 移除表头标签，让表格更紧凑
    matrixTable->horizontalHeader()->setVisible(false);
    matrixTable->verticalHeader()->setVisible(false);

    // 设置合适的表格尺寸，但不限制最大尺寸
    matrixTable->setMinimumSize(280, 120);

    // 设置列宽
    for(int i = 0; i < 4; i++) {
        matrixTable->setColumnWidth(i, 70);
    }

    // 设置行高
    for(int i = 0; i < 4; i++) {
        matrixTable->setRowHeight(i, 28);
    }

    matrixTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    matrixTable->setSelectionMode(QAbstractItemView::NoSelection);
    matrixTable->setFocusPolicy(Qt::NoFocus);

    // 初始化表格内容
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            QTableWidgetItem *item = new QTableWidgetItem("0.000000");
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            matrixTable->setItem(i, j, item);
        }
    }
}


//************ TAB： CODE GEN *****************

void MainWindow::setupCodeGenerationTab()
{
    QWidget *codeTab = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(codeTab);

    // 左侧加工点区域
    QGroupBox *pointsGroup = new QGroupBox("模型加工点坐标");
    QVBoxLayout *pointsLayout = new QVBoxLayout(pointsGroup);

    processPointsText = new QTextEdit;
    processPointsText->setPlaceholderText("输入模型提取的加工点坐标，每行一个点，格式: x,y,z,i,j,k,附加信息\n例如:\n0,0,0,0,0,0,程序1,10s\n50,50,0,45,0,0,程序2,15s");
    pointsLayout->addWidget(processPointsText); // 去掉高度限制

    QHBoxLayout *pointsBtnLayout = new QHBoxLayout;
    processImportBtn = new QPushButton("导入CSV");
    processClearBtn = new QPushButton("清空");
    pointsBtnLayout->addWidget(processImportBtn);
    pointsBtnLayout->addWidget(processClearBtn);
    pointsLayout->addLayout(pointsBtnLayout);

    // 右侧设置区域
    QGroupBox *settingsGroup = new QGroupBox("代码生成设置");
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);

    // 文件名称
    settingsLayout->addWidget(new QLabel("输出文件:"));
    QHBoxLayout *fileLayout = new QHBoxLayout;
    fileNameInput = new QLineEdit;
    fileNameInput->setText("./testpgm.txt");
    fileNameInput->setPlaceholderText("选择输出文件路径...");
    QPushButton *browseBtn = new QPushButton("浏览");
    fileLayout->addWidget(fileNameInput);
    fileLayout->addWidget(browseBtn);
    settingsLayout->addLayout(fileLayout);

    // 添加一些间距
    settingsLayout->addSpacing(20);

    generateBtn = new QPushButton("生成G代码");
    generateBtn->setMinimumHeight(40);
    settingsLayout->addWidget(generateBtn);

    settingsLayout->addStretch();

    // 添加到主布局
    mainLayout->addWidget(pointsGroup, 2);      // 左侧加工点区域占2份
    mainLayout->addWidget(settingsGroup, 1);    // 右侧设置区域占1份

    tabWidget->addTab(codeTab, "代码生成");

    // 连接信号槽
    connect(processImportBtn, &QPushButton::clicked, this, &MainWindow::onProcessPointsImportClicked);
    connect(processClearBtn, &QPushButton::clicked, this, &MainWindow::onProcessPointsClearClicked);
    connect(generateBtn, &QPushButton::clicked, this, &MainWindow::onGenerateCodeClicked);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "保存G代码文件", "", "NC文件 (*.nc);;所有文件 (*)");
        if(!fileName.isEmpty()) {
            fileNameInput->setText(fileName);
        }
    });
    connect(saveMachineParamsBtn, &QPushButton::clicked, this, &MainWindow::onSaveMachineParametersClicked);
}
