#pragma once

#include "models/ocrresult.h"
#include "viewmodels/mainwindowviewmodel.h"

#include <QMainWindow>

class QComboBox;
class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QProgressBar;
class QTableWidget;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void browseInputDir();
    void browseOutputExcel();
    void startBatch();
    void appendResult(const OcrResult &result);
    void clearResults();
    void setBusy(bool busy);
    void updateProgress(int current, int total);
    void log(const QString &message);

private:
    void buildUi();
    void connectViewModel();

    // View 只保存 UI 控制項，不保存批次流程狀態。
    // 辨識進度、結果與輸出都交給 MainWindowViewModel。
    QLineEdit *m_inputDirEdit = nullptr;
    QLineEdit *m_outputExcelEdit = nullptr;
    QComboBox *m_modelCombo = nullptr;
    QLineEdit *m_urlEdit = nullptr;
    QPushButton *m_browseInputButton = nullptr;
    QPushButton *m_browseOutputButton = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QTableWidget *m_table = nullptr;
    QPlainTextEdit *m_promptEdit = nullptr;
    QPlainTextEdit *m_logEdit = nullptr;
    QLabel *m_statusLabel = nullptr;

    MainWindowViewModel m_viewModel;
};
