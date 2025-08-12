#include "emojipickerwidget.h"

#include <QPushButton>
#include <QPixmap>
#include <QIcon>
#include <QSize>

EmojiPickerWidget::EmojiPickerWidget(const QStringList& emojiResourcePaths, QWidget* parent)
    : QWidget(parent), m_emojiResourcePaths(emojiResourcePaths)
{
    m_gridLayout = new QGridLayout(this);
    m_gridLayout->setSpacing(4);
    m_gridLayout->setContentsMargins(4, 4, 4, 4);

    int columns = 8; // Adjust as needed
    int row = 0, col = 0;

    // Use the loaded emoji font
    QFont emojiFont("Noto Color Emoji");
    emojiFont.setPointSize(30); // Adjust size as needed

    for (const QString& emojiChar : m_emojiResourcePaths) {
        QPushButton* btn = new QPushButton(this);
        btn->setText(emojiChar); // Set emoji as button text
        btn->setFont(emojiFont);
        btn->setFixedSize(36, 36);
        btn->setFlat(true);
        btn->setStyleSheet("border: none; background: transparent;");

        connect(btn, &QPushButton::clicked, this, [this, emojiChar]() {
            emit emojiSelected(emojiChar);
        });

        m_gridLayout->addWidget(btn, row, col);

        ++col;
        if (col >= columns) {
            col = 0;
            ++row;
        }
    }

    setLayout(m_gridLayout);
}

