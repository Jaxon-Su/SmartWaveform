# SmartWaveform

SmartWaveform 是一個小工具，用來批次讀取示波器截圖，透過本機 Ollama 做 OCR，再把量測結果整理成 Excel。

目前主要支援從圖片文字中抓出：

```text
max, min, rms, mean, peak
```

其中 `peak` 是由 `max` 和 `min` 比較絕對值後得到，會保留原本正負號。例如 `max = 100`、`min = -103` 時，`peak = -103`。

## 專案結構

```text
src/models/
  ocrresult.h                  單張圖片的 OCR 結果資料結構

src/views/
  mainwindow.h/.cpp            UI，包含按鈕、輸入欄位、表格與 log

src/viewmodels/
  mainwindowviewmodel.h/.cpp   批次流程控制，負責掃描、送 OCR、更新進度、輸出檔案

src/services/
  imagescanner.h/.cpp          遞迴掃描圖片檔
  ollamaclient.h/.cpp          呼叫 Ollama /api/generate
  waveformocrparser.h/.cpp     解析 Ollama 回傳文字，抓出量測欄位

src/exporters/
  iresultwriter.h              輸出介面
  excelresultwriter.h/.cpp     XLSX 輸出實作

third_party/QXlsx/
  QXlsx library，用來產生 .xlsx
```

簡單分工：

```text
ImageScanner        找圖片
OllamaClient        送圖片給 Ollama
WaveformOcrParser   解析 OCR 結果
ExcelResultWriter   輸出 Excel
MainWindowViewModel 串起整個批次流程
MainWindow          顯示 UI
```

## 使用流程

1. 先啟動 Ollama。
2. 確認模型存在，例如 `glm-ocr:latest`。
3. 開啟 `SmartWaveform.exe`。
4. 選擇圖片根目錄。
5. 設定輸出 Excel 路徑。
6. 按 `Start` 開始批次處理。

Ollama URL 預設是：

```text
http://127.0.0.1:11434/api/generate
```

如果 Ollama 跑在另一台電腦，或 port 有改，請在 UI 裡改成對應 URL。

## 圖片資料夾

程式會遞迴掃描根目錄底下的圖片，支援：

```text
png, jpg, jpeg, bmp, webp
```

例如：

```text
C:\Users\su622\Desktop\HP waveform
  Q1\
    current\
      1.png
      2.png
    Voltage\
      1.png
      2.png
  Q2\
    current\
    Voltage\
  Q3\
    current\
    Voltage\
```

Excel 會依照第一層和第二層資料夾拆 sheet，例如：

```text
Q1_current_sheet
Q1_voltage_sheet
Q2_current_sheet
Q2_voltage_sheet
Q3_current_sheet
Q3_voltage_sheet
```

## Excel 輸出

輸出檔為 `.xlsx`，預設檔名：

```text
smart_waveform_results.xlsx
```

每個 sheet 欄位如下：

```text
filename, max, min, rms, mean, peak, status, raw_response
```

`raw_response` 會保留 Ollama 原始回應，方便之後比對 OCR 結果。

## Prompt

UI 內可以直接修改 prompt。預設值是：

```text
Text Recognition:
```

如果 prompt 欄位是空的，程式會自動使用這個預設值。

## 第三方套件

使用 QXlsx 產生 Excel 檔：

```text
third_party/QXlsx
```

QXlsx 授權為 MIT License。Qt 本身授權需另外依照實際使用版本確認。
