#include "TouchComboBox.h"
#include <QWindow>

TouchComboBox::TouchComboBox(QWidget* parent) : QComboBox(parent) {
    setFocusPolicy(Qt::NoFocus);
}

TouchComboBox::~TouchComboBox() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
    if (m_popupOverlay) {
        m_popupOverlay->deleteLater();
        m_popupOverlay = nullptr;
    }
}

void TouchComboBox::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        if (m_popupOverlay && m_popupOverlay->isVisible()) {
            hidePopup();
            return;
        }
        showPopup();
        return;
    }
    QComboBox::mousePressEvent(e);
}

void TouchComboBox::showPopup() {
    QWidget* topWin = window();
    if (!topWin) return;

    // Расчет адаптивного масштабирования под физический размер окна
    const qreal s = std::clamp(std::min(topWin->width() / 1280.0, topWin->height() / 720.0), 0.6, 1.4);
    const int padH = qMax(6, qRound(10 * s));
    const int padV = qMax(2, qRound(4 * s));
    const int radius = qMax(4, qRound(6 * s));
    const int overlayRadius = qMax(6, qRound(8 * s));
    const int margin = qMax(2, qRound(4 * s));
    const int spacing = qMax(1, qRound(2 * s));
    const int itemH = height();

    const QString accentCol = palette().highlight().color().name();

    if (!m_popupOverlay) {
        m_popupOverlay = new QFrame(topWin);
        m_popupOverlay->setObjectName("comboPopupOverlay");

        auto* layout = new QVBoxLayout(m_popupOverlay);
        layout->setContentsMargins(margin, margin, margin, margin);
        layout->setSpacing(spacing);

        for (int i = 0; i < count(); ++i) {
            auto* btn = new QPushButton(itemText(i), m_popupOverlay);
            btn->setCursor(Qt::PointingHandCursor);
            connect(btn, &QPushButton::clicked, this, [this, i]() {
                setCurrentIndex(i);
                emit activated(i);
                emit currentIndexChanged(i);
                hidePopup();
            });
            layout->addWidget(btn);
        }
    } else {
        if (auto* l = m_popupOverlay->layout()) {
            l->setContentsMargins(margin, margin, margin, margin);
            l->setSpacing(spacing);
        }
    }
    qApp->installEventFilter(this);

    m_popupOverlay->setStyleSheet(QString(R"(
        QFrame#comboPopupOverlay { background-color: #182234; border: 1px solid rgba(251, 191, 36, 0.4); border-radius: %1px; }
        QPushButton { background-color: transparent; color: #F8FAFC; border: none; border-radius: %2px; padding: %3px %4px; font-weight: bold; text-align: left; }
        QPushButton:hover, QPushButton:pressed { background-color: %5; color: #FFFFFF; }
    )").arg(overlayRadius).arg(radius).arg(padV).arg(padH).arg(accentCol));

    const QFont f = font();
    auto btns = m_popupOverlay->findChildren<QPushButton*>();
    for (int i = 0; i < btns.size() && i < count(); ++i) {
        btns[i]->setText(itemText(i));
        btns[i]->setFont(f);
        btns[i]->setFixedHeight(itemH);
    }

    // Позиционируем оверлей строго по ширине родительского комбобокса
    QPoint posInWin = mapTo(topWin, QPoint(0, height() + 3));
    const int popupW = width();
    const int popupH = m_popupOverlay->sizeHint().height();

    // Если всплывающее меню выходит за нижний край экрана - открываем его над кнопкой
    if (posInWin.y() + popupH > topWin->height() - 10) {
        posInWin.setY(mapTo(topWin, QPoint(0, -popupH - 3)).y());
    }

    m_popupOverlay->setGeometry(posInWin.x(), posInWin.y(), popupW, popupH);
    m_popupOverlay->raise();
    m_popupOverlay->show();

    #if defined(Q_OS_ANDROID)
    if (topWin->windowHandle()) {
        topWin->windowHandle()->requestUpdate();
    }
    #endif
}

void TouchComboBox::hidePopup() {
    if (m_popupOverlay && m_popupOverlay->isVisible()) {
        qApp->removeEventFilter(this);
        m_popupOverlay->hide();
        update();
        #if defined(Q_OS_ANDROID)
        if (window() && window()->windowHandle()) {
            window()->windowHandle()->requestUpdate();
        }
        #endif
    }
}

bool TouchComboBox::eventFilter(QObject* watched, QEvent* event) {
    if (m_popupOverlay && m_popupOverlay->isVisible()) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
            QPoint globalPos;
            if (event->type() == QEvent::MouseButtonPress) {
                globalPos = static_cast<QMouseEvent*>(event)->globalPosition().toPoint();
            } else {
                auto* te = static_cast<QTouchEvent*>(event);
                if (!te->points().isEmpty()) {
                    globalPos = te->points().first().globalPosition().toPoint();
                }
            }
            const QRect overlayRect(m_popupOverlay->mapToGlobal(QPoint(0, 0)), m_popupOverlay->size());
            const QRect comboRect(mapToGlobal(QPoint(0, 0)), size());

            // Закрытие оверлея при тапе мимо меню
            if (!overlayRect.contains(globalPos)) {
                hidePopup();
                if (comboRect.contains(globalPos)) {
                    return true; // Поглощаем повторный клик по самому комбобоксу
                }
            }
        }
    }
    return QComboBox::eventFilter(watched, event);
}
