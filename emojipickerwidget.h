#ifndef EMOJIPICKERWIDGET_H
#define EMOJIPICKERWIDGET_H

#include <QWidget>

#include <QGridLayout>
#include <QStringList>

class EmojiPickerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EmojiPickerWidget(const QStringList& emojiResourcePaths, QWidget* parent = nullptr);

signals:
    void emojiSelected(const QString& emojiResourcePath);

private:
    QGridLayout* m_gridLayout;
    QStringList m_emojiResourcePaths;
};

#endif // EMOJIPICKERWIDGET_H
