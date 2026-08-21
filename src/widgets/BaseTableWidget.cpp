#include "BaseTableWidget.h"
#include "Audio.h"

BaseTableWidget::BaseTableWidget(QWidget* parent) : QWidget(parent) {
    #if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    setMinimumSize(640, 360);
    #endif
    setMouseTracking(true);

    btnBackMenu = new QPushButton(getLocalizedText("← В Меню", "← Menu"), this);
    lblStatus   = new QLabel("", this);
    btnNextHand = new QPushButton(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"), this);
    btnSettings = new QPushButton("⚙", this);
    btnRules    = new QPushButton("📖", this);

    btnBackMenu->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; padding: 5px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: white; }");
    lblStatus->setStyleSheet("QLabel { color: #F3F4F6; font-weight: bold; }");
    btnNextHand->setStyleSheet("QPushButton { background: #10B981; color: white; font-weight: bold; border-radius: 8px; padding: 10px; } QPushButton:hover { background: #34D399; }");
    btnSettings->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: #FBBF24; }");
    btnRules->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: #FBBF24; }");

    btnBackMenu->setCursor(Qt::PointingHandCursor);
    btnNextHand->setCursor(Qt::PointingHandCursor);
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnRules->setCursor(Qt::PointingHandCursor);
    btnNextHand->hide();

    connect(btnBackMenu, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playSound(SoundEffect::ButtonClick);
        if (onBackToMenuCallback) onBackToMenuCallback();
    });

        connect(btnSettings, &QPushButton::clicked, this, [this]() {
            AudioManager::instance().playSound(SoundEffect::ButtonClick);
            if (onOpenSettingsCallback) onOpenSettingsCallback();
        });

            connect(btnRules, &QPushButton::clicked, this, [this]() {
                AudioManager::instance().playSound(SoundEffect::ButtonClick);
                if (onOpenRulesCallback) onOpenRulesCallback();
            });
}

void BaseTableWidget::resizeEvent(QResizeEvent* ev) {
    QWidget::resizeEvent(ev);
    const qreal s = getScale();

    const int btnH = qRound(36 * s);
    const int btnBackW = qRound(110 * s);
    const int iconBtnW = qRound(40 * s);
    const int leftOffset = getSafeLeftMargin();

    btnBackMenu->setGeometry(leftOffset, qRound(15 * s), btnBackW, btnH);

    btnSettings->setGeometry(width() - iconBtnW - qRound(15 * s), qRound(15 * s), iconBtnW, btnH);
    btnSettings->setFont(QFont(font().family(), qMax(12, qRound(20 * s)), QFont::Bold));

    btnRules->setGeometry(width() - iconBtnW * 2 - qRound(25 * s), qRound(15 * s), iconBtnW, btnH);
    btnRules->setFont(QFont(font().family(), qMax(12, qRound(20 * s)), QFont::Bold));

    lblStatus->setGeometry(leftOffset + btnBackW + qRound(15 * s), qRound(15 * s), width() - leftOffset - btnBackW - iconBtnW * 2 - qRound(75 * s), btnH);
    lblStatus->setFont(QFont(font().family(), qMax(9, qRound(15 * s)), QFont::Bold));

    const int nextW = qRound(260 * s);
    const int nextH = qRound(55 * s);
    btnNextHand->setGeometry(width() / 2 - nextW / 2, height() / 2 + qRound(60 * s), nextW, nextH);
    btnNextHand->setFont(QFont(font().family(), qMax(9, qRound(14 * s)), QFont::Bold));
}

void BaseTableWidget::drawTableFelt(QPainter& p) {
    p.fillRect(rect(), QColor(20, 20, 20));
    const qreal s = getScale();
    const int margin = qMax(6, qRound(16 * s));
    const QRect feltRect = rect().adjusted(margin, margin, -margin, -margin);

    const int cornerRadius = qMin(feltRect.width() / 4, feltRect.height() / 2);
    QPainterPath tablePath;
    tablePath.addRoundedRect(feltRect, cornerRadius, cornerRadius);

    QColor c1, c2;
    switch (AppSettings::instance().getTableColor()) {
        case TableColor::BurgundyRed: c1 = QColor(140, 20, 40); c2 = QColor(60, 5, 15); break;
        case TableColor::DarkBlue:   c1 = QColor(25, 60, 120); c2 = QColor(10, 25, 55); break;
        case TableColor::PokerBlack: c1 = QColor(45, 45, 50); c2 = QColor(15, 15, 20); break;
        default:                     c1 = QColor(30, 130, 60); c2 = QColor(10, 60, 20); break;
    }

    QRadialGradient bgGrad(width() / 2.0, height() / 2.0, qMax(width(), height()));
    bgGrad.setColorAt(0, c1);
    bgGrad.setColorAt(1, c2);
    p.fillPath(tablePath, bgGrad);

    p.setPen(QPen(QColor(0, 0, 0, 120), qMax(2, qRound(4 * s))));
    p.drawPath(tablePath);
}

void BaseTableWidget::drawGameOverBanner(QPainter& p, const QString& message) {
    const qreal s = getScale();
    #if defined(Q_OS_ANDROID)
    p.fillRect(rect(), QColor(0, 0, 0, 180));
    p.setBrush(QColor(15, 23, 42));
    #else
    p.fillRect(rect(), QColor(0, 0, 0, 110));
    p.setBrush(QColor(15, 23, 42, 240));
    #endif

    const int bannerW = qRound(440 * s);
    const int bannerH = qRound(65 * s);
    const QRect bannerRect(width() / 2 - bannerW / 2, height() / 2 - qRound(125 * s), bannerW, bannerH);

    p.setPen(QPen(QColor(251, 191, 36, 220), qMax(1, qRound(2 * s))));
    p.drawRoundedRect(bannerRect, qRound(10 * s), qRound(10 * s));

    p.setPen(QColor(252, 211, 77));
    p.setFont(QFont(font().family(), qMax(10, qRound(16 * s)), QFont::Bold));
    p.drawText(bannerRect, Qt::AlignCenter | Qt::TextWordWrap, message);
}

QVector<QPoint> BaseTableWidget::getSeatPositions(int numPlayers, int width, int height, int bottomYOffset, int topYOffset) {
    const qreal s = std::clamp(std::min(width / 1280.0, height / 720.0), 0.45, 2.5);
    const int sideX = qMax(80, qRound(120 * s));
    const int sideY = height / 2 - qRound(70 * s);

    if (numPlayers == 2) {
        return {
            QPoint(width / 2, height - bottomYOffset),
            QPoint(width / 2, topYOffset)
        };
    }
    if (numPlayers == 3) {
        return {
            QPoint(width / 2, height - bottomYOffset),
            QPoint(sideX, sideY),
            QPoint(width - sideX, sideY)
        };
    }
    return {
        QPoint(width / 2, height - bottomYOffset),
        QPoint(sideX, sideY),
        QPoint(width / 2, topYOffset),
        QPoint(width - sideX, sideY)
    };
}

void BaseTableWidget::drawCard(QPainter& p, const QRect& rect, const Card* card, bool faceUp, bool selected) {
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 120));
    p.drawRoundedRect(rect.translated(4, 5), 6, 6);

    QPainterPath path;
    path.addRoundedRect(rect, 6, 6);
    p.fillPath(path, selected ? QColor(254, 240, 138) : Qt::white);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(150, 150, 150), 1));
    p.drawPath(path);

    if (!faceUp || !card) {
        const QRect inner = rect.adjusted(5, 5, -5, -5);
        p.setClipRect(inner);

        QColor shirtColor;
        switch (AppSettings::instance().getCardShirt()) {
            case CardShirtStyle::RedVelvet:   shirtColor = QColor(185, 28, 28); break;
            case CardShirtStyle::GoldRoyal:   shirtColor = QColor(180, 140, 20); break;
            case CardShirtStyle::DarkPattern: shirtColor = QColor(30, 30, 35); break;
            default:                          shirtColor = QColor(30, 58, 138); break;
        }

        p.fillRect(inner, shirtColor);
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        for (int i = inner.left() - inner.height(); i < inner.right() + inner.height(); i += 8) {
            p.drawLine(i, inner.top(), i - inner.height(), inner.bottom());
        }
        p.restore();
        return;
    }

    const bool isRed = (card->suit == Hearts || card->suit == Diamonds);
    p.setPen(isRed ? QColor(220, 38, 38) : QColor(17, 24, 39));

    static const QString suitsStr[] = { "♥", "♦", "♣", "♠" };
    static const QString ranksStr[] = { "", "", "2", "3", "4", "5", "6", "7", "8", "9", "10", getLocalizedText("В", "J"), getLocalizedText("Д", "Q"), getLocalizedText("К", "K"), getLocalizedText("Т", "A") };

    const QString suitTxt = suitsStr[card->suit];
    const QString rankTxt = ranksStr[card->rank];

    const int cornerFontSize = qMax(7, qRound(rect.height() * 0.11));
    p.setFont(QFont(p.font().family(), cornerFontSize, QFont::Bold));
    p.drawText(rect.adjusted(4, 2, -2, -2), Qt::AlignTop | Qt::AlignLeft, rankTxt + "\n" + suitTxt);

    p.save();
    p.translate(rect.center());
    p.rotate(180);
    p.setFont(QFont(p.font().family(), cornerFontSize, QFont::Bold));
    const QRectF localRect(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
    p.drawText(localRect.adjusted(4, 2, -2, -2), Qt::AlignTop | Qt::AlignLeft, rankTxt + "\n" + suitTxt);
    p.restore();

    const int centerFontSize = qMax(12, qRound(rect.height() * 0.32));
    p.setFont(QFont(p.font().family(), centerFontSize));
    p.drawText(rect, Qt::AlignCenter, suitTxt);

    p.restore();
}
