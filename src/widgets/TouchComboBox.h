#pragma once

#include <QComboBox>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QApplication>
#include <algorithm>

/**
 * @brief Кастомный выпадающий список для идеальной работы с Touch-экранами и адаптивным масштабированием.
 * Устраняет баги нативных выпадающих меню Qt Widgets на Android и iOS (QTBUG-127495).
 */
class TouchComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit TouchComboBox(QWidget* parent = nullptr);
    ~TouchComboBox() override;

    void showPopup() override;
    void hidePopup() override;

protected:
    void mousePressEvent(QMouseEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QFrame* m_popupOverlay = nullptr;
};
