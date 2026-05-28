#pragma once

#include <QString>

// OCR 單張圖片的辨識結果。
// 這個 struct 是 View、ViewModel、Exporter 之間共用的資料模型；
// 未來要增加欄位，例如 peak-to-peak、frequency，可以先從這裡擴充。rawResponse 用來保留模型原始輸出，方便比較不同 model。
struct OcrResult
{
    QString folder;
    QString filename;
    QString max;
    QString min;
    QString rms;
    QString mean;
    QString peak;
    QString status;
    QString rawResponse;
};
