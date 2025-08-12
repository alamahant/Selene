#ifndef LOGVIEWERDIALOG_H
#define LOGVIEWERDIALOG_H

#include <QDialog>

#include <QString>

class QTextEdit;
class QLineEdit;

class LogViewerDialog : public QDialog {
    Q_OBJECT
public:
    explicit LogViewerDialog(const QString& logPath, QWidget* parent = nullptr);

private slots:
    void onSearchClicked();
    void onTruncateClicked();

private:
    void loadLog();

    QString m_logPath;
    QString m_logContents;
    QTextEdit* m_logEdit;
    QLineEdit* m_searchEdit;
};

#endif // LOGVIEWERDIALOG_H
