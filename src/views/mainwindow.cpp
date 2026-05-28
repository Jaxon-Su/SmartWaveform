#include "mainwindow.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    connectViewModel();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    auto *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(new QLabel(tr("Image root:"), this));
    m_inputDirEdit = new QLineEdit(QStringLiteral("C:/Users/"), this);
    m_browseInputButton = new QPushButton(tr("Browse..."), this);
    inputLayout->addWidget(m_inputDirEdit, 1);
    inputLayout->addWidget(m_browseInputButton);
    mainLayout->addLayout(inputLayout);

    auto *outputLayout = new QHBoxLayout;
    outputLayout->addWidget(new QLabel(tr("Output Excel:"), this));
    m_outputExcelEdit = new QLineEdit(QStringLiteral("C:/Users/smart_waveform_results.xlsx"), this);
    m_browseOutputButton = new QPushButton(tr("Browse..."), this);
    outputLayout->addWidget(m_outputExcelEdit, 1);
    outputLayout->addWidget(m_browseOutputButton);
    mainLayout->addLayout(outputLayout);

    auto *settingsLayout = new QHBoxLayout;
    settingsLayout->addWidget(new QLabel(tr("Model:"), this));
    m_modelCombo = new QComboBox(this);
    m_modelCombo->setEditable(true);
    m_modelCombo->addItem(QStringLiteral("glm-ocr:latest"));
    m_modelCombo->setCurrentText(QStringLiteral("glm-ocr:latest"));
    settingsLayout->addWidget(m_modelCombo);
    settingsLayout->addWidget(new QLabel(tr("Ollama URL:"), this));
    m_urlEdit = new QLineEdit(QStringLiteral("http://127.0.0.1:11434/api/generate"), this);
    settingsLayout->addWidget(m_urlEdit, 1);
    mainLayout->addLayout(settingsLayout);

    auto *promptLayout = new QVBoxLayout;
    promptLayout->addWidget(new QLabel(tr("Prompt:"), this));
    m_promptEdit = new QPlainTextEdit(this);
    m_promptEdit->setPlainText(QStringLiteral("Text Recognition:"));
    m_promptEdit->setMaximumHeight(72);
    promptLayout->addWidget(m_promptEdit);
    mainLayout->addLayout(promptLayout);
    auto *controlLayout = new QHBoxLayout;
    m_startButton = new QPushButton(tr("Start"), this);
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_stopButton->setEnabled(false);
    m_progressBar = new QProgressBar(this);
    m_statusLabel = new QLabel(tr("Ready"), this);
    controlLayout->addWidget(m_startButton);
    controlLayout->addWidget(m_stopButton);
    controlLayout->addWidget(m_progressBar, 1);
    controlLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(controlLayout);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({
        tr("Folder"), tr("Filename"), tr("Max"), tr("Min"), tr("RMS"), tr("Mean"), tr("Peak"), tr("Status"), tr("Raw Response")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_table, 1);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(1000);
    m_logEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_logEdit, &QPlainTextEdit::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu *menu = m_logEdit->createStandardContextMenu();
        menu->addSeparator();
        QAction *clearAction = menu->addAction(tr("Clear"));
        connect(clearAction, &QAction::triggered, m_logEdit, &QPlainTextEdit::clear);
        menu->exec(m_logEdit->mapToGlobal(pos));
        delete menu;
    });
    mainLayout->addWidget(m_logEdit, 1);

    setCentralWidget(central);
    setWindowTitle(tr("SmartWaveform - Ollama Oscilloscope OCR"));
}


void MainWindow::connectViewModel()
{
    connect(m_browseInputButton, &QPushButton::clicked, this, &MainWindow::browseInputDir);
    connect(m_browseOutputButton, &QPushButton::clicked, this, &MainWindow::browseOutputExcel);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startBatch);
    connect(m_stopButton, &QPushButton::clicked, &m_viewModel, &MainWindowViewModel::stopBatch);

    connect(&m_viewModel, &MainWindowViewModel::resultsCleared, this, &MainWindow::clearResults);
    connect(&m_viewModel, &MainWindowViewModel::resultReady, this, &MainWindow::appendResult);
    connect(&m_viewModel, &MainWindowViewModel::logMessage, this, &MainWindow::log);
    connect(&m_viewModel, &MainWindowViewModel::progressChanged, this, &MainWindow::updateProgress);
    connect(&m_viewModel, &MainWindowViewModel::statusChanged, m_statusLabel, &QLabel::setText);
    connect(&m_viewModel, &MainWindowViewModel::busyChanged, this, &MainWindow::setBusy);
}

void MainWindow::browseInputDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select image root"), m_inputDirEdit->text());
    if (dir.isEmpty())
        return;

    m_inputDirEdit->setText(QDir::toNativeSeparators(dir));
    m_outputExcelEdit->setText(QDir::toNativeSeparators(QDir(dir).filePath(QStringLiteral("smart_waveform_results.xlsx"))));
}

void MainWindow::browseOutputExcel()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Select output Excel"),
        m_outputExcelEdit->text(),
        tr("Excel Files (*.xlsx);;All Files (*)"));

    if (!path.isEmpty())
        m_outputExcelEdit->setText(QDir::toNativeSeparators(path));
}

void MainWindow::startBatch()
{
    m_viewModel.startBatch(
        m_inputDirEdit->text(),
        m_outputExcelEdit->text(),
        m_modelCombo->currentText(),
        m_urlEdit->text(),
        m_promptEdit->toPlainText());
}

void MainWindow::appendResult(const OcrResult &result)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    const QStringList values = {
        result.folder, result.filename, result.max, result.min, result.rms, result.mean, result.peak, result.status, result.rawResponse
    };

    for (int col = 0; col < values.size(); ++col)
        m_table->setItem(row, col, new QTableWidgetItem(values.at(col)));
}

void MainWindow::clearResults()
{
    m_table->setRowCount(0);
    m_logEdit->clear();
}

void MainWindow::setBusy(bool busy)
{
    m_startButton->setEnabled(!busy);
    m_stopButton->setEnabled(busy);
    m_browseInputButton->setEnabled(!busy);
    m_browseOutputButton->setEnabled(!busy);
    m_inputDirEdit->setEnabled(!busy);
    m_outputExcelEdit->setEnabled(!busy);
    m_modelCombo->setEnabled(!busy);
    m_urlEdit->setEnabled(!busy);
    m_promptEdit->setEnabled(!busy);
}

void MainWindow::updateProgress(int current, int total)
{
    m_progressBar->setRange(0, total);
    m_progressBar->setValue(current);
}

void MainWindow::log(const QString &message)
{
    const QString line = QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), message);
    m_logEdit->appendPlainText(line);
}
