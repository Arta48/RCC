#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

/**
 * @brief Диалоговое окно справочника правил карточных игр (Покер, Дурак, Козёл, Уно).
 */
class RulesDialog : public QDialog {
    Q_OBJECT
public:
    explicit RulesDialog(int defaultTabIndex = 0, QWidget* parent = nullptr);
    ~RulesDialog() override = default;

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
};
