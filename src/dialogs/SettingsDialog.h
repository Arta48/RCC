#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * @brief Модальное диалоговое окно настроек игры (Звук, Визуал, Сеть и Правила).
 */
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    QLineEdit* nickInput    = nullptr;
    QLineEdit* portLineEdit = nullptr;
    QSpinBox*  portSpin     = nullptr;

    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
};
