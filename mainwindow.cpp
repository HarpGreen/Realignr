#include "mainwindow.h"
#include "least_squares.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <sstream>
#include <iomanip>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    tabWidget = new QTabWidget(this);

    setupCoordinateMappingTab();
    setupCodeGenerationTab();

    setCentralWidget(tabWidget);
    char title[128];
    sprintf(title, "Realignr - v%d.%d.%d", VERSION[0],VERSION[1],VERSION[2]);
    setWindowTitle(title);
    resize(800, 500);

    // 初始化时禁用代码生成板块
    setCodeGenerationEnabled(false);
}

MainWindow::~MainWindow()
{
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
        if(importPointsFromCSV(fileName, actualPointsText, actualPoints)) {
            QMessageBox::information(this, "成功", QString("成功导入 %1 个实际坐标点").arg(actualPoints.size()));
        }
    }
}

void MainWindow::onActualClearClicked()
{
    actualPointsText->clear();
    actualPoints.clear();
}

void MainWindow::onCalculateClicked()
{
    try {
        // 解析坐标点
        this->modelPoints = parsePointsFromText(modelPointsText->toPlainText());
        this->actualPoints = parsePointsFromText(actualPointsText->toPlainText());

        if(modelPoints.size() != actualPoints.size() || modelPoints.size() < 3) {
            QMessageBox::warning(this, "错误",
                                 QString("坐标点数量不匹配或不足\n模型点: %1, 实际点: %2\n至少需要3对点")
                                     .arg(modelPoints.size()).arg(actualPoints.size()));
            return;
        }

        // 计算转换矩阵
        transformationMatrix = LeastSquaresSolver::computeTransformation(modelPoints, actualPoints);

        // 显示转换矩阵
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                matrixTable->item(i, j)->setText(QString::number(transformationMatrix[i][j], 'f', 6));
            }
        }

        // 计算并显示误差
        double avg_error = LeastSquaresSolver::computeError(modelPoints, actualPoints, transformationMatrix);

        // 详细说明误差含义
        QString errorInfo = QString(
                                "转换矩阵计算完成!\n\n"
                                "平均配准误差: %1 mm\n\n"
                                "误差说明:\n"
                                "• 该误差表示模型点经过转换后与实际点之间的平均距离\n"
                                "• 计算方法: 对每个点计算转换后的坐标与实际坐标的欧氏距离，然后取平均值\n"
                                "• 此误差反映了坐标映射的拟合精度，并非加工精度"
                                ).arg(avg_error, 0, 'f', 4);

        QMessageBox::information(this, "计算成功", errorInfo);


        // 计算成功后激活代码生成板块
        setCodeGenerationEnabled(true);

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "计算错误", e.what());

        // 计算失败就禁用代码生成
        setCodeGenerationEnabled(false);
    }
}

void MainWindow::onVerifyClicked()
{
    QString input = verifyInput->text();
    QString cleaned = cleanCoordinateString(input);
    QStringList coords = cleaned.split(',', Qt::SkipEmptyParts);

    if(coords.size() != 3) {
        QMessageBox::warning(this, "错误", "请输入格式为 x,y,z 的坐标");
        return;
    }

    try {
        // 解析输入点
        Point3D input_point;
        input_point.x = coords[0].toDouble();
        input_point.y = coords[1].toDouble();
        input_point.z = coords[2].toDouble();

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

        verifyResult->setText(result);

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "验证错误", e.what());
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
    if (fileNameInput->text().isEmpty()) {
        QMessageBox::warning(this, "错误", "请选择输出文件路径");
        return;
    }

    if (processPoints.empty()) {
        QMessageBox::warning(this, "错误", "没有加工点数据");
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
        QString gcode = generateGCode(processPoints, axisDefinition->text());

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


// 控制代码生成板块激活状态的函数
void MainWindow::setCodeGenerationEnabled(bool enabled)
{
    // 禁用/启用代码生成Tab中的所有控件
    processPointsText->setEnabled(enabled);
    processImportBtn->setEnabled(enabled);
    processClearBtn->setEnabled(enabled);
    axisDefinition->setEnabled(enabled);
    fileNameInput->setEnabled(enabled);
    generateBtn->setEnabled(enabled);

    // 设置视觉提示
    if (enabled) {
        tabWidget->setTabText(1, "代码生成");
        processPointsText->setPlaceholderText("输入加工点坐标，每行一个点，格式: x,y,z,i，j,k,duration,processMethodSelection,comment");
    } else {
        processPointsText->setPlaceholderText("请先在坐标映射Tab中计算转换矩阵");
        processPointsText->clear();
    }
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

QString MainWindow::generateGCode(const std::vector<ProcessPoint>& points, const QString& axisDef)
{
    QStringList gcodeLines;

    // 添加文件头
    gcodeLines.append("; 由Realignr生成");
    gcodeLines.append(QString("; 加工点数量: %1").arg(points.size()));
    gcodeLines.append(QString("; 五轴定义: %1").arg(axisDef));
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
        QString rotationCode = convertDirectionToRotation(point.i, point.j, point.k, axisDef);

        QString line;

        if (!rotationCode.isEmpty()) {
            // 五轴加工指令
            line = QString("G01 X%1 Y%2 Z%3 %4")
                       .arg(transformedPos.x, 0, 'f', 3)
                       .arg(transformedPos.y, 0, 'f', 3)
                       .arg(transformedPos.z, 0, 'f', 3)
                       .arg(rotationCode);
        } else {
            // 三轴加工指令
            line = QString("G01 X%1 Y%2 Z%3")
                       .arg(transformedPos.x, 0, 'f', 3)
                       .arg(transformedPos.y, 0, 'f', 3)
                       .arg(transformedPos.z, 0, 'f', 3);
        }

        // 加工时间（使用G04）
        if (point.duration > 0 && point.processMethodSelection == 0) {
            line += QString(" G04 P%1").arg(point.duration, 0, 'f', 1);
        }

        // 添加注释
        line += QString(" ; (point%1) %2").arg(i + 1).arg(point.comment);

        gcodeLines.append(line);
    }

    // 文件尾
    gcodeLines.append("");
    gcodeLines.append("G00 Z50.000 ; 安全高度");
    gcodeLines.append("M30 ; 程序结束");

    return gcodeLines.join("\n");
}

// 转换方向向量（只旋转，不平移）
QString MainWindow::convertDirectionToRotation(double i, double j, double k, const QString& axisDef)
{
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

    // 解析轴定义
    QStringList axes = axisDef.toLower().split(',', Qt::SkipEmptyParts);

    if (axes.contains("a") && axes.contains("b")) {
        // AB五轴: A绕X轴, B绕Y轴
        double a_angle = atan2(transformed_j, transformed_k) * 180.0 / M_PI;
        double b_angle = atan2(transformed_i, transformed_k) * 180.0 / M_PI;
        rotationCode = QString("A%1 B%2").arg(a_angle, 0, 'f', 3).arg(b_angle, 0, 'f', 3);
    }
    else if (axes.contains("a") && axes.contains("c")) {
        // AC五轴: A绕X轴, C绕Z轴
        double a_angle = atan2(transformed_j, transformed_k) * 180.0 / M_PI;
        double c_angle = atan2(transformed_i, transformed_j) * 180.0 / M_PI;
        rotationCode = QString("A%1 C%2").arg(a_angle, 0, 'f', 3).arg(c_angle, 0, 'f', 3);
    }
    else if (axes.contains("b") && axes.contains("c")) {
        // BC五轴: B绕Y轴, C绕Z轴
        double b_angle = atan2(transformed_i, transformed_k) * 180.0 / M_PI;
        double c_angle = atan2(transformed_i, transformed_j) * 180.0 / M_PI;
        rotationCode = QString("B%1 C%2").arg(b_angle, 0, 'f', 3).arg(c_angle, 0, 'f', 3);
    }
    else if (axes.contains("a") && !axes.contains("b") && !axes.contains("c")) {
        // 单A轴
        double a_angle = atan2(transformed_j, transformed_k) * 180.0 / M_PI;
        rotationCode = QString("A%1").arg(a_angle, 0, 'f', 3);
    }
    else if (axes.contains("b") && !axes.contains("a") && !axes.contains("c")) {
        // 单B轴
        double b_angle = atan2(transformed_i, transformed_k) * 180.0 / M_PI;
        rotationCode = QString("B%1").arg(b_angle, 0, 'f', 3);
    }
    else if (axes.contains("c") && !axes.contains("a") && !axes.contains("b")) {
        // 单C轴
        double c_angle = atan2(transformed_i, transformed_j) * 180.0 / M_PI;
        rotationCode = QString("C%1").arg(c_angle, 0, 'f', 3);
    }

    return rotationCode;
}
