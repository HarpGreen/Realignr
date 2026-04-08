#include "mainwindow.h"
#include "least_squares.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDateTime>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <sstream>
#include <iomanip>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , mill_p(nullptr)
    , machineParamsSaved(false)
{
    tabWidget = new QTabWidget(this);

    setupMachineParametersTab();
    onMachineTypeChanged();  // 初始化默认轴名称
    setupCoordinateMappingTab();
    setupCodeGenerationTab();
    setupVisualizationTab();

    setCentralWidget(tabWidget);
    char title[128];
    sprintf(title, "Realignr - v%d.%d.%d", VERSION[0],VERSION[1],VERSION[2]);
    setWindowTitle(title);
    setWindowIcon(QIcon(":/icons/Realignr.ico"));
    resize(800, 500);

    // 初始化时禁用后续Tab
    tabWidget->setTabEnabled(1, false);
    tabWidget->setTabEnabled(2, false);
    tabWidget->setTabEnabled(3, false);
}

MainWindow::~MainWindow()
{
    if (mill_p) {
        delete mill_p;
        mill_p = nullptr;
    }
}

// 添加带时间戳的信息消息到输出框
void MainWindow::appendInfoMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ");
    infoOutput->append(timestamp + message);
}

// 坐标映射Tab槽函数
void MainWindow::onModelImportClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "导入模型坐标CSV", "", "CSV文件 (*.csv)");
    if(!fileName.isEmpty()) {
        if(importPointsFromCSV(fileName, modelPointsText, modelPoints)) {
            QMessageBox::information(this, "成功", QString("成功导入 %1 个模型坐标点").arg(modelPoints.size()));
        }
    }
}

void MainWindow::onModelClearClicked()
{
    modelPointsText->clear();
    modelPoints.clear();
}

void MainWindow::onActualImportClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "导入实际坐标CSV", "", "CSV文件 (*.csv)");
    if(!fileName.isEmpty()) {
        if(importMachinePointsFromCSV(fileName, actualPointsText, actualMachinePoints)) {
            QMessageBox::information(this, "成功", QString("成功导入 %1 个实际坐标点").arg(actualMachinePoints.size()));
        }
    }
}

void MainWindow::onActualClearClicked()
{
    actualPointsText->clear();
    actualPoints.clear();
    actualMachinePoints.clear();
}

void MainWindow::onMachineTypeChanged()
{
    if (machineTypeCombo->currentIndex() == 0) {
        // XYZAC
        machineAxisLabels->setText("x,y,z,a,c");
    } else {
        // XYZBC
        machineAxisLabels->setText("x,y,z,b,c");
    }
}

void MainWindow::onSaveMachineParametersClicked()
{
    if (!machineParamsSaved) {
        // 保存模式
        QString axisText = machineAxisLabels->text().trimmed();
        QStringList axisLabels = axisText.split(',', Qt::SkipEmptyParts);
        if (axisLabels.size() != 5) {
            QMessageBox::warning(this, "错误", "请输入5个轴名称，格式示例: x,y,z,a,c");
            return;
        }

        OM5A_Axis_Property axisProps[5];
        for (int i = 0; i < 5; ++i) {
            QString label = axisLabels[i].trimmed();
            if (label.isEmpty()) {
                QMessageBox::warning(this, "错误", "轴名称不能为空");
                return;
            }

            if (!parseAxisLabel(label, axisProps[i])) {
                QMessageBox::warning(this, "错误", "轴名称格式不正确，请检查输入");
                return;
            }
        }

        OM5A_Rotation_Property rotationProps;
        bool ok = true;
        double cx = rotationCenterCX->text().toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "错误", "请输入有效的 CX");
            return;
        }
        double cy = rotationCenterCY->text().toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "错误", "请输入有效的 CY");
            return;
        }
        double cz = rotationCenterCZ->text().toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "错误", "请输入有效的 CZ");
            return;
        }
        double dx = rotationCenterDX->text().toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "错误", "请输入有效的 DX");
            return;
        }
        double dy = rotationCenterDY->text().toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "错误", "请输入有效的 DY");
            return;
        }
        double dz = rotationCenterDZ->text().toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "错误", "请输入有效的 DZ");
            return;
        }

        rotationProps.Cx = cx;
        rotationProps.Cy = cy;
        rotationProps.Cz = cz;
        rotationProps.Dx = dx;
        rotationProps.Dy = dy;
        rotationProps.Dz = dz;

        OM5A_Mill_Type millType = (machineTypeCombo->currentIndex() == 0)
                ? OM5A_Mill_Type_XYZAC
                : OM5A_Mill_Type_XYZBC;

        double initialPos[5] = {0.0, 0.0, 0.0, 0.0, 0.0};

        if (mill_p) {
            delete mill_p;
            mill_p = nullptr;
        }
        mill_p = new OM5A(millType, axisProps, rotationProps, initialPos);

        QMessageBox::information(this, "保存成功", "机床参数已保存");

        tabWidget->setTabEnabled(1, true);

        // 切换到修改模式
        machineParamsSaved = true;
        saveMachineParamsBtn->setText("修改参数");
        machineTypeCombo->setEnabled(false);
        machineAxisLabels->setEnabled(false);
        rotationCenterCX->setEnabled(false);
        rotationCenterCY->setEnabled(false);
        rotationCenterCZ->setEnabled(false);
        rotationCenterDX->setEnabled(false);
        rotationCenterDY->setEnabled(false);
        rotationCenterDZ->setEnabled(false);
    } else {
        // 修改模式 - 切换回编辑状态
        machineParamsSaved = false;
        saveMachineParamsBtn->setText("保存参数");
        machineTypeCombo->setEnabled(true);
        machineAxisLabels->setEnabled(true);
        rotationCenterCX->setEnabled(true);
        rotationCenterCY->setEnabled(true);
        rotationCenterCZ->setEnabled(true);
        rotationCenterDX->setEnabled(true);
        rotationCenterDY->setEnabled(true);
        rotationCenterDZ->setEnabled(true);
    }
}

void MainWindow::onCalculateClicked()
{
    if (!machineParamsSaved) {
        QMessageBox::warning(this, "错误", "请先保存机床参数");
        return;
    }

    try {
        // 解析坐标点
        this->modelPoints = parsePointsFromText(modelPointsText->toPlainText());
        this->actualMachinePoints = parseMachinePointsFromText(actualPointsText->toPlainText());
        this->actualPoints.clear();

        if (mill_p) {
            for (const auto& machinePoint : actualMachinePoints) {
                double newPos[5] = {
                    machinePoint.x,
                    machinePoint.y,
                    machinePoint.z,
                    machinePoint.a,
                    machinePoint.c
                };
                mill_p->setPos(newPos);
                Eigen::Vector3d cPoint = mill_p->currentPosToC();
                actualPoints.emplace_back(cPoint.x(), cPoint.y(), cPoint.z());
            }
        }

        const bool useICP = (algorithmCombo->currentIndex() == 1);

        if (useICP) {
            if (modelPoints.empty() || actualPoints.empty()) {
                QMessageBox::warning(this, "错误", "请先输入模型坐标点和实际坐标点");
                return;
            }
            if (modelPoints.size() < 3 || actualPoints.size() < 3) {
                QMessageBox::warning(this, "错误",
                                     QString("ICP需要至少3个模型点和3个实际点\n模型点: %1, 实际点: %2")
                                         .arg(modelPoints.size()).arg(actualPoints.size()));
                return;
            }
        } else {
            if (modelPoints.empty() || actualPoints.empty()) {
                QMessageBox::warning(this, "错误", "请先输入模型坐标点和实际坐标点");
                return;
            }
            if (modelPoints.size() != actualPoints.size() || modelPoints.size() < 3) {
                QMessageBox::warning(this, "错误",
                                     QString("坐标点数量不匹配或不足\n模型点: %1, 实际点: %2\n至少需要3对点")
                                         .arg(modelPoints.size()).arg(actualPoints.size()));
                return;
            }
        }

        // 计算转换矩阵
        double avg_error = 0.0;
        if (algorithmCombo->currentIndex() == 1) {
            // ICP 配准
            Eigen::MatrixXd modelMat(modelPoints.size(), 3);
            Eigen::MatrixXd actualMat(actualPoints.size(), 3);
            for (int i = 0; i < (int)modelPoints.size(); ++i) {
                modelMat(i, 0) = modelPoints[i].x;
                modelMat(i, 1) = modelPoints[i].y;
                modelMat(i, 2) = modelPoints[i].z;
            }
            for (int i = 0; i < (int)actualPoints.size(); ++i) {
                actualMat(i, 0) = actualPoints[i].x;
                actualMat(i, 1) = actualPoints[i].y;
                actualMat(i, 2) = actualPoints[i].z;
            }

            ICP_OUT icp_result = icp(modelMat, actualMat, 100, 0.000001);

            // 从 icp 结果填充齐次矩阵格式
            transformationMatrix = LeastSquaresSolver::TransformationMatrix(4, std::vector<double>(4, 0.0));
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    transformationMatrix[i][j] = icp_result.trans(i, j);
                }
            }

            if (!icp_result.distances.empty()) {
                avg_error = std::accumulate(icp_result.distances.begin(), icp_result.distances.end(), 0.0) / icp_result.distances.size();
            }

        } else {
            // 最小二乘 Ax=B
            transformationMatrix = LeastSquaresSolver::computeTransformation(modelPoints, actualPoints);
            avg_error = LeastSquaresSolver::computeError(modelPoints, actualPoints, transformationMatrix);
        }

        // 显示转换矩阵
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                matrixTable->item(i, j)->setText(QString::number(transformationMatrix[i][j], 'f', 6));
            }
        }

        // 详细说明误差含义
        QString errorInfo = QString(
                                "转换矩阵计算完成!\n\n"
                                "平均配准误差: %1 mm\n\n"
                                "误差说明:\n"
                                "• 该误差表示模型点经过转换后与实际点之间的平均距离\n"
                                "• 计算方法: 对每个点计算转换后的坐标与实际坐标的欧氏距离，然后取平均值\n"
                                "• 此误差反映了坐标映射的拟合精度，并非加工精度"
                                ).arg(avg_error, 0, 'f', 4);

        appendInfoMessage(errorInfo);


        // 计算成功后解锁代码生成tab和可视化验证tab
        tabWidget->setTabEnabled(2, true);
        tabWidget->setTabEnabled(3, true);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "计算错误", e.what());
    }
}

void MainWindow::onVerifyClicked()
{
    QString input = verifyInput->text();
    QString cleaned = cleanCoordinateString(input);
    QStringList coords = cleaned.split(',', Qt::SkipEmptyParts);

    if (coords.size() != 3) {
        QMessageBox::warning(this, "错误", "请输入格式为 x,y,z 的坐标");
        return;
    }

    if (!LeastSquaresSolver::isValidMatrix(transformationMatrix)) {
        QMessageBox::warning(this, "错误", "请先完成坐标映射计算并生成有效的转换矩阵");
        appendInfoMessage("点验证失败：转换矩阵无效或尚未计算。");
        return;
    }

    try {
        // 解析输入点
        bool okX = false, okY = false, okZ = false;
        Point3D input_point;
        input_point.x = coords[0].toDouble(&okX);
        input_point.y = coords[1].toDouble(&okY);
        input_point.z = coords[2].toDouble(&okZ);

        if (!okX || !okY || !okZ) {
            QMessageBox::warning(this, "错误", "输入坐标必须是有效数字，格式示例: x,y,z");
            appendInfoMessage("点验证失败：输入坐标含非法数字。");
            return;
        }

        // 转换坐标
        Point3D transformed_point = LeastSquaresSolver::transformPoint(transformationMatrix, input_point);

        // 显示结果
        QString result = QString("输入坐标: (%1, %2, %3)\n"
                                 "转换结果: (%4, %5, %6)")
                             .arg(input_point.x, 0, 'f', 3)
                             .arg(input_point.y, 0, 'f', 3)
                             .arg(input_point.z, 0, 'f', 3)
                             .arg(transformed_point.x, 0, 'f', 3)
                             .arg(transformed_point.y, 0, 'f', 3)
                             .arg(transformed_point.z, 0, 'f', 3);

        appendInfoMessage(result);

    } catch (const std::exception& e) {
        QString err = QString("点验证异常: %1").arg(e.what());
        QMessageBox::critical(this, "验证错误", err);
        appendInfoMessage(err);
    }
}

// 代码生成Tab槽函数实现
void MainWindow::onProcessPointsImportClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "导入加工点CSV", "", "CSV文件 (*.csv)");
    if(!fileName.isEmpty()) {
        if(importProcessPointsFromCSV(fileName)) {
            QMessageBox::information(this, "成功", "加工点坐标导入成功");
        }
    }
}

void MainWindow::onProcessPointsClearClicked()
{
    processPointsText->clear();
    processPoints.clear();
}

void MainWindow::onGenerateCodeClicked()
{
    if (!mill_p) {
        QMessageBox::warning(this, "错误", "请先保存机床参数");
        return;
    }

    if (fileNameInput->text().isEmpty()) {
        QMessageBox::warning(this, "错误", "请选择输出文件路径");
        return;
    }

    try {
        // 从文本框重新解析数据，确保数据最新
        processPoints = parseProcessPoints(processPointsText->toPlainText());

        if (processPoints.empty()) {
            QMessageBox::warning(this, "错误", "无法解析加工点数据");
            return;
        }

        // 生成G代码
        QString gcode = generateGCode(processPoints);

        // 保存文件
        QFile file(fileNameInput->text());
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << gcode;
            file.close();

            // 生成成功后打开文件夹
            QFileInfo fileInfo(fileNameInput->text());
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));

            QMessageBox::information(this, "成功",
                                     QString("G代码生成完成!\n输出文件: %1\n加工点数量: %2")
                                         .arg(fileNameInput->text())
                                         .arg(processPoints.size()));
        } else {
            QMessageBox::critical(this, "错误", "无法保存文件");
        }

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "生成错误", e.what());
    }
}

// 可视化验证Tab槽函数实现
void MainWindow::onVisualizeClicked()
{
    try {
        // 检查是否有计算结果
        if (modelPoints.empty() || actualPoints.empty()) {
            QMessageBox::warning(this, "错误", "请先完成坐标映射计算");
            return;
        }

        // 准备数据
        QJsonObject dataObj;

        // 模型坐标点
        QJsonArray modelArray;
        for (const auto& point : modelPoints) {
            QJsonArray pointArray;
            pointArray.append(point.x);
            pointArray.append(point.y);
            pointArray.append(point.z);
            modelArray.append(pointArray);
        }
        dataObj["model_points"] = modelArray;

        // 实际坐标点
        QJsonArray actualArray;
        for (const auto& point : actualPoints) {
            QJsonArray pointArray;
            pointArray.append(point.x);
            pointArray.append(point.y);
            pointArray.append(point.z);
            actualArray.append(pointArray);
        }
        dataObj["actual_points"] = actualArray;

        auto round3 = [](double value) {
            return QString::number(value, 'f', 3).toDouble();
        };

        // 加工点变换前
        QJsonArray processBeforeArray;
        for (const auto& processPoint : processPoints) {
            QJsonArray pointArray;
            pointArray.append(round3(processPoint.x));
            pointArray.append(round3(processPoint.y));
            pointArray.append(round3(processPoint.z));
            processBeforeArray.append(pointArray);
        }
        dataObj["process_points_before"] = processBeforeArray;

        // 加工点变换后（与G代码生成输出坐标一致）
        QJsonArray processAfterArray;
        for (const auto& processPoint : processPoints) {
            Point3D transformed = LeastSquaresSolver::transformPoint(transformationMatrix,
                                                                    Point3D(processPoint.x, processPoint.y, processPoint.z));
            QJsonArray pointArray;
            pointArray.append(round3(transformed.x));
            pointArray.append(round3(transformed.y));
            pointArray.append(round3(transformed.z));
            processAfterArray.append(pointArray);
        }
        dataObj["process_points_after"] = processAfterArray;

        // 转换为JSON字符串
        QJsonDocument doc(dataObj);
        QString jsonData = doc.toJson(QJsonDocument::Compact);

        // 调用Python脚本
        const QString appDir = QCoreApplication::applicationDirPath();
        const QStringList pythonScriptCandidates = {
            QDir(appDir).filePath("visualize/visualize.py"),
            QDir(appDir).absoluteFilePath("../visualize/visualize.py"),
            QDir(appDir).absoluteFilePath("../../visualize/visualize.py"),
            QDir(appDir).absoluteFilePath("../../../visualize/visualize.py")
        };

        QString pythonScript;
        for (const QString &candidate : pythonScriptCandidates) {
            if (QFileInfo(candidate).exists()) {
                pythonScript = candidate;
                break;
            }
        }

        if (pythonScript.isEmpty()) {
            QString err = QString("找不到 Python 脚本: %1").arg(pythonScriptCandidates.join(", "));
            QMessageBox::critical(this, "可视化错误", err);
            appendInfoMessage(err);
            return;
        }

        QFileInfo scriptInfo(pythonScript);
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(scriptInfo.absolutePath());
        process->setProcessChannelMode(QProcess::SeparateChannels);
        process->setProperty("stderrOutput", QString());

        connect(process, &QProcess::readyReadStandardError, [process]() {
            QString current = process->property("stderrOutput").toString();
            current += QString::fromUtf8(process->readAllStandardError());
            process->setProperty("stderrOutput", current);
        });

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
            QString stderrOutput = process->property("stderrOutput").toString();
            QString stdoutOutput = QString::fromUtf8(process->readAllStandardOutput());
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                appendInfoMessage("可视化脚本执行成功。");
            } else {
                QString detail = stderrOutput.isEmpty() ? stdoutOutput : stderrOutput;
                if (detail.isEmpty()) {
                    detail = process->errorString();
                }
                QMessageBox::warning(this, "可视化错误",
                                     QString("Python脚本执行失败: %1").arg(detail));
                appendInfoMessage(QString("可视化失败: %1").arg(detail));
            }
            process->deleteLater();
        });

        // 启动进程，将JSON数据通过stdin传递
        process->start("python", QStringList() << pythonScript);
        if (process->waitForStarted(3000)) {
            process->write(jsonData.toUtf8());
            process->closeWriteChannel();
        } else {
            QString err = process->errorString();
            QMessageBox::critical(this, "错误", QString("无法启动Python进程: %1").arg(err));
            appendInfoMessage(QString("可视化启动失败: %1").arg(err));
            process->deleteLater();
        }

        appendInfoMessage("启动可视化验证...");

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "可视化错误", QString("准备数据时发生错误: %1").arg(e.what()));
    }
}

// 工具函数
// 解析坐标点函数
std::vector<Point3D> MainWindow::parsePointsFromText(const QString& text)
{
    std::vector<Point3D> points;
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QString cleaned = cleanCoordinateString(line);
        QStringList coords = cleaned.split(',', Qt::SkipEmptyParts);

        if (coords.size() >= 3) {
            bool conversionOk = true;
            Point3D point;

            point.x = coords[0].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.y = coords[1].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.z = coords[2].toDouble(&conversionOk);
            if (!conversionOk) continue;

            points.push_back(point);
        }
    }

    return points;
}

// 解析机床5轴实际点文本
std::vector<MachinePoint5D> MainWindow::parseMachinePointsFromText(const QString& text)
{
    std::vector<MachinePoint5D> points;
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QString cleaned = cleanCoordinateString(line);
        QStringList coords = cleaned.split(',', Qt::SkipEmptyParts);

        if (coords.size() >= 3) {
            bool conversionOk = true;
            MachinePoint5D point;

            point.x = coords[0].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.y = coords[1].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.z = coords[2].toDouble(&conversionOk);
            if (!conversionOk) continue;

            // 如果输入中包含角度值，则使用它们；否则默认 0
            point.a = (coords.size() >= 4) ? coords[3].toDouble(&conversionOk) : 0.0;
            if (!conversionOk) continue;
            point.c = (coords.size() >= 5) ? coords[4].toDouble(&conversionOk) : 0.0;
            if (!conversionOk) continue;

            points.push_back(point);
        }
    }

    return points;
}

// 批量解析加工点
std::vector<ProcessPoint> MainWindow::parseProcessPoints(const QString& text)
{
    std::vector<ProcessPoint> points;
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        ProcessPoint point;
        QString cleaned = cleanCoordinateString(line);
        QStringList coords = cleaned.split(',', Qt::SkipEmptyParts);

        if (coords.size() >= 9) {
            point.x = coords[0].toDouble();
            point.y = coords[1].toDouble();
            point.z = coords[2].toDouble();
            point.i = coords[3].toDouble();
            point.j = coords[4].toDouble();
            point.k = coords[5].toDouble();
            point.duration = coords[6].toDouble();
            point.processMethodSelection = coords[7].toInt();
            point.comment = (coords.size() > 8) ? coords[8] : "";

            // 验证方向向量是否为单位向量
            double length = sqrt(point.i*point.i + point.j*point.j + point.k*point.k);
            if (fabs(length - 1.0) > 0.1 && length > 0.01) { // 忽略零向量
                qWarning() << "警告: 方向向量不是单位向量, 长度:" << length << "点:" << point.comment;
            }

            points.push_back(point);
        } else if (coords.size() > 0) {
            qWarning() << "跳过无效数据行(列数不足):" << line;
        }
    }

    return points;
}

// 清理坐标字符串函数
QString MainWindow::cleanCoordinateString(const QString& input)
{
    QString result;
    bool lastWasComma = false;
    bool hasDecimal = false;

    for (int i = 0; i < input.length(); ++i) {
        QChar ch = input[i];

        if (ch.isDigit()) {
            result.append(ch);
            lastWasComma = false;
            hasDecimal = false;
        }
        else if (ch == '.' && !hasDecimal) {
            // 确保小数点前有数字或负号
            if (!result.isEmpty() && (result.back().isDigit() || result.back() == '-')) {
                result.append(ch);
                hasDecimal = true;
                lastWasComma = false;
            }
        }
        else if (ch == '-' && (result.isEmpty() || lastWasComma)) {
            result.append(ch);
            lastWasComma = false;
            hasDecimal = false;
        }
        else if (ch == ',' && !lastWasComma) {
            result.append(ch);
            lastWasComma = true;
            hasDecimal = false;
        }
        //用一串判断，逐个字符判断，只读入数字和,.-四种字符
        //如需增加，在这里加elseif
    }

    // 移除末尾的逗号
    while (!result.isEmpty() && (result.back() == ',' || result.back() == '.')) {
        result.chop(1);
    }

    return result;
}


// 通用坐标点导入函数
bool MainWindow::importPointsFromCSV(const QString& fileName, QTextEdit* textEdit, std::vector<Point3D>& points)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法打开文件: " + fileName);
        return false;
    }

    QTextStream in(&file);
    QStringList lines;
    std::vector<Point3D> importedPoints;

    // 读取文件内容
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            lines.append(line);
        }
    }
    file.close();

    if (lines.isEmpty()) {
        QMessageBox::warning(this, "警告", "文件为空");
        return false;
    }

    // 处理CSV内容
    bool hasHeader = false;
    QStringList firstLine = lines.first().split(',');

    // 检查是否有表头
    if (!firstLine.isEmpty()) {
        QString firstElement = firstLine[0].trimmed();
        bool isNumeric;
        firstElement.toDouble(&isNumeric);
        hasHeader = !isNumeric; // 第一个元素不是数字，则认为有表头
    }

    int startLine = hasHeader ? 1 : 0;
    int successCount = 0;
    QStringList displayLines;

    for (int i = startLine; i < lines.size(); ++i) {
        QString line = lines[i];
        QString cleanedLine = cleanCoordinateString(line);
        QStringList coords = cleanedLine.split(',', Qt::SkipEmptyParts);

        if (coords.size() >= 3) {
            bool conversionOk = true;
            Point3D point;

            point.x = coords[0].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.y = coords[1].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.z = coords[2].toDouble(&conversionOk);
            if (!conversionOk) continue;

            importedPoints.push_back(point);
            displayLines.append(QString("%1,%2,%3").arg(point.x).arg(point.y).arg(point.z));
            successCount++;
        }
    }

    if (successCount == 0) {
        QMessageBox::warning(this, "警告", "文件中未找到有效的坐标数据");
        return false;
    }

    // 更新UI和数据
    points = importedPoints;
    textEdit->setPlainText(displayLines.join("\n"));

    return true;
}

bool MainWindow::importMachinePointsFromCSV(const QString& fileName, QTextEdit* textEdit, std::vector<MachinePoint5D>& points)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法打开文件: " + fileName);
        return false;
    }

    QTextStream in(&file);
    QStringList lines;
    std::vector<MachinePoint5D> importedPoints;

    // 读取文件内容
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            lines.append(line);
        }
    }
    file.close();

    if (lines.isEmpty()) {
        QMessageBox::warning(this, "警告", "文件为空");
        return false;
    }

    bool hasHeader = false;
    QStringList firstLine = lines.first().split(',');
    if (!firstLine.isEmpty()) {
        QString firstElement = firstLine[0].trimmed();
        bool isNumeric;
        firstElement.toDouble(&isNumeric);
        hasHeader = !isNumeric;
    }

    int startLine = hasHeader ? 1 : 0;
    int successCount = 0;
    QStringList displayLines;

    for (int i = startLine; i < lines.size(); ++i) {
        QString line = lines[i];
        QString cleanedLine = cleanCoordinateString(line);
        QStringList coords = cleanedLine.split(',', Qt::SkipEmptyParts);

        if (coords.size() >= 3) {
            bool conversionOk = true;
            MachinePoint5D point;

            point.x = coords[0].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.y = coords[1].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.z = coords[2].toDouble(&conversionOk);
            if (!conversionOk) continue;

            point.a = (coords.size() >= 4) ? coords[3].toDouble(&conversionOk) : 0.0;
            if (!conversionOk) continue;

            point.c = (coords.size() >= 5) ? coords[4].toDouble(&conversionOk) : 0.0;
            if (!conversionOk) continue;

            importedPoints.push_back(point);
            displayLines.append(QString("%1,%2,%3,%4,%5")
                .arg(point.x).arg(point.y).arg(point.z).arg(point.a).arg(point.c));
            successCount++;
        }
    }

    if (successCount == 0) {
        QMessageBox::warning(this, "警告", "文件中未找到有效的5轴坐标数据");
        return false;
    }

    points = importedPoints;
    textEdit->setPlainText(displayLines.join("\n"));
    return true;
}

// 加工点导入函数
bool MainWindow::importProcessPointsFromCSV(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法打开文件: " + fileName);
        return false;
    }

    QTextStream in(&file);
    QStringList lines;
    QStringList displayLines;

    // 读取文件内容
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            lines.append(line);
        }
    }
    file.close();

    if (lines.isEmpty()) {
        QMessageBox::warning(this, "警告", "文件为空");
        return false;
    }

    // 处理加工点CSV（包含角度和附加信息）
    bool hasHeader = false;
    QStringList firstLine = lines.first().split(',');

    // 检查是否有表头
    if (!firstLine.isEmpty()) {
        QString firstElement = firstLine[0].trimmed();
        bool isNumeric;
        firstElement.toDouble(&isNumeric);
        hasHeader = !isNumeric; // 第一个元素不是数字，则认为有表头
    }

    int startLine = hasHeader ? 1 : 0;
    int successCount = 0;

    // 清空现有数据
    processPointsText->clear();

    for (int i = startLine; i < lines.size(); ++i) {
        QString line = lines[i];
        QStringList parts = line.split(',');

        if (parts.size() >= 9) {
            // 清理纯数字部分
            QString cleanedCoords = cleanCoordinateString(parts[0] + "," + parts[1] + "," + parts[2] + "," +
                                                          parts[3] + "," + parts[4] + "," + parts[5] + "," +
                                                          parts[6] + "," + parts[7]);
            QStringList coordList = cleanedCoords.split(',', Qt::SkipEmptyParts);

            if (coordList.size() >= 8) {
                bool conversionOk = true;
                double x = coordList[0].toDouble(&conversionOk);
                double y = coordList[1].toDouble(&conversionOk);
                double z = coordList[2].toDouble(&conversionOk);
                double a = coordList[3].toDouble(&conversionOk);
                double b = coordList[4].toDouble(&conversionOk);
                double c = coordList[5].toDouble(&conversionOk);
                double d = coordList[6].toDouble(&conversionOk);
                int    s = coordList[7].toInt(&conversionOk);
                QString cmt = parts[8];

                if (conversionOk) {
                    // 构建显示行（保持原始格式，只清理坐标部分）
                    QString displayLine;
                    displayLine += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                                       .arg(x).arg(y).arg(z).arg(a).arg(b).arg(c).arg(d).arg(s).arg(cmt);

                    // 添加附加信息（如果存在）
                    for (int j = 9; j < parts.size(); ++j) {
                        displayLine += "," + parts[j];
                    }

                    displayLines.append(displayLine);
                    successCount++;
                }
            }
        }
    }

    if (successCount == 0) {
        QMessageBox::warning(this, "警告", "文件中未找到有效的加工点数据");
        return false;
    }

    processPointsText->setPlainText(displayLines.join("\n"));

    // 同时解析到processPoints向量中
    processPoints = parseProcessPoints(displayLines.join("\n"));

    return true;
}

QString MainWindow::generateGCode(
    const std::vector<ProcessPoint>& points)
{
    if (!mill_p) {
        return "";
    }

    const OM5A_Axis_Property* axisProps = mill_p->axes;
    QString axisDef;
    for (int i = 0; i < 5; ++i) {
        axisDef += QString::fromStdString(axisProps[i].axisLabel);
        if (i < 4) axisDef += ",";
    }

    QStringList gcodeLines;

    // 添加文件头
    gcodeLines.append("; 由Realignr生成");
    gcodeLines.append(QString("; 加工点数量: <%1>").arg(points.size()));
    gcodeLines.append(QString("; 五轴定义: <%1>").arg(axisDef));
    gcodeLines.append("; 开始加工");
    gcodeLines.append("G90 G54 ; 绝对坐标, 工作坐标系");
    gcodeLines.append("G00 Z50.000 ; 移动到安全高度");
    gcodeLines.append("");

    // 为每个加工点生成代码
    for (size_t i = 0; i < points.size(); ++i) {
        const ProcessPoint& point = points[i];

        // 转换坐标位置（使用之前的最小二乘转换）
        Point3D transformedPos = LeastSquaresSolver::transformPoint(transformationMatrix,
                                                                    Point3D(point.x, point.y, point.z));

        // 转换方向向量
        QString rotationCode = convertDirectionToRotation(point.i, point.j, point.k);

        /////////////////这里严重缺根据转台旋转角度改变xyz的代码，后续需要完善/////////////////
        // 需要根据旋转中心位置和旋转角度调整XYZ坐标，确保加工点在旋转后仍然正确
        // 目前的实现只是简单地旋转方向向量，没有考虑旋转对坐标位置的影响，这可能导致加工点位置不正确
        // 需要根据旋转轴和旋转角度计算新的坐标位置，确保加工点在旋转后仍然正确
        
        // 使用rotationProps中的旋转中心信息和旋转角度来调整坐标位置

        QString line;
        line = QString("G01 X%1 Y%2 Z%3 %4")
                    .arg(transformedPos.x, 0, 'f', 3)
                    .arg(transformedPos.y, 0, 'f', 3)
                    .arg(transformedPos.z, 0, 'f', 3)
                    .arg(rotationCode);

        // 加工时间（使用G04）
        if (point.duration > 0 && point.processMethodSelection == 0) {
            line += QString(" G04 P%1").arg(point.duration, 0, 'f', 1);
        }

        // 添加注释
        line += QString(" ; (%1) %2").arg(i + 1).arg(point.comment);

        gcodeLines.append(line);
    }

    // 文件尾
    gcodeLines.append("");
    gcodeLines.append("G00 Z50.000 ; 安全高度");
    gcodeLines.append("M30 ; 程序结束");

    return gcodeLines.join("\n");
}

// 转换方向向量（只旋转，不平移）
QString MainWindow::convertDirectionToRotation(
    double i, double j, double k)
{
    if (!mill_p) {
        return "";
    }

    const OM5A_Axis_Property* axisProps = mill_p->axes;
    QString rotationCode;

    // 从转换矩阵中提取旋转部分（3x3子矩阵）
    double r00 = transformationMatrix[0][0];
    double r01 = transformationMatrix[0][1];
    double r02 = transformationMatrix[0][2];
    double r10 = transformationMatrix[1][0];
    double r11 = transformationMatrix[1][1];
    double r12 = transformationMatrix[1][2];
    double r20 = transformationMatrix[2][0];
    double r21 = transformationMatrix[2][1];
    double r22 = transformationMatrix[2][2];

    // 应用旋转矩阵到方向向量
    double transformed_i = r00 * i + r01 * j + r02 * k;
    double transformed_j = r10 * i + r11 * j + r12 * k;
    double transformed_k = r20 * i + r21 * j + r22 * k;

    // 归一化确保仍是单位向量
    double length = sqrt(transformed_i * transformed_i +
                         transformed_j * transformed_j +
                         transformed_k * transformed_k);
    if (length > 1e-10) {
        transformed_i /= length;
        transformed_j /= length;
        transformed_k /= length;
    }

    // 解析旋转轴定义
    QStringList axes = QStringList()
        << QString::fromStdString(axisProps[3].axisLabel)
        << QString::fromStdString(axisProps[4].axisLabel);

    // 判断axes的两个旋转轴是哪两个，并计算对应的旋转角度
    double angle1 = 0.0;
    double angle2 = 0.0;
    QString label1 = axes[0].toLower();
    QString label2 = axes[1].toLower();

    // 轴1
    char ax1;
    for (ax1 = 'a'; ax1 <= 'c'; ax1++) {
        if (label1.contains(QString(ax1))) {
            break;
        }
    }
    switch (ax1)
    {
    case 'a':
        angle1 = atan2(transformed_j, transformed_k) * 180.0 / M_PI;
        break;
    case 'b':
        angle1 = atan2(transformed_i, transformed_k) * 180.0 / M_PI;
        break;
    case 'c':
        angle1 = atan2(transformed_i, transformed_j) * 180.0 / M_PI;
        break;
    default:
        qWarning() << "不支持的旋转轴标签:" << label1;
        return "#label1 Error#";
    }

    // 轴2
    char ax2;
    for (ax2 = 'a'; ax2 <= 'c'; ax2++) {
        if (label2.contains(QString(ax2))) {
            break;
        }
    }
    switch (ax2)
    {
    case 'a':
        angle2 = atan2(transformed_j, transformed_k) * 180.0 / M_PI;
        break;
    case 'b':
        angle2 = atan2(transformed_i, transformed_k) * 180.0 / M_PI;
        break;
    case 'c':
        angle2 = atan2(transformed_i, transformed_j) * 180.0 / M_PI;
        break;
    default:
        qWarning() << "不支持的旋转轴标签:" << label2;
        return "#label2 Error#";
    }

    if (axisProps[3].isReversed) {
        angle1 = -angle1;
    }
    if (axisProps[4].isReversed) {
        angle2 = -angle2;
    }

    rotationCode = QString("%1%2 %3%4")
        .arg(axes[0]).arg(angle1, 0, 'f', 3)
        .arg(axes[1]).arg(angle2, 0, 'f', 3);

    return rotationCode;
}

bool MainWindow::parseAxisLabel(const QString& raw, OM5A_Axis_Property &prop)
{
    QString axis = raw.trimmed();
    if (axis.isEmpty()) {
        return false;
    }

    bool reversed = false;
    // 支持负号或加号出现在开头或结尾
    bool stripped = true;
    while (stripped && !axis.isEmpty()) {
        stripped = false;
        if (axis.startsWith('+')) {
            axis = axis.mid(1).trimmed();
            stripped = true;
        } else if (axis.startsWith('-')) {
            axis = axis.mid(1).trimmed();
            reversed = !reversed;
            stripped = true;
        }
        if (axis.endsWith('+')) {
            axis.chop(1);
            axis = axis.trimmed();
            stripped = true;
        } else if (axis.endsWith('-')) {
            axis.chop(1);
            axis = axis.trimmed();
            reversed = !reversed;
            stripped = true;
        }
    }

    if (axis.isEmpty()) {
        return false;
    }

    prop.axisLabel = axis.toStdString();
    prop.isReversed = reversed;
    return true;
}


bool MainWindow::machine2WorkpieceTransformation(
    const Point3D& machinePoint, const Point3D& direction, 
    const OM5A_Rotation_Property& rotationProps, 
    Point3D& workpiecePoint, Point3D& workpieceDirection) 
{
    

    return true;
}