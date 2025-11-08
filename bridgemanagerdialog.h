#ifndef BRIDGEMANAGERDIALOG_H
#define BRIDGEMANAGERDIALOG_H

#include <QDialog>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include"constants.h"
#include<QCheckBox>
#include<QSettings>

class BridgeManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BridgeManagerDialog(QWidget *parent = nullptr);

private slots:
    void addBridges();
    void clearBridges();
    void loadCurrentBridges();
    QString getTorrcPath();

private:
    void setupUI();
    void setupConnections();

    QTextEdit *bridgeTextEdit;
    QPushButton *addButton;
    QPushButton *clearButton;
    QPushButton *closeButton;
    QLabel *statusLabel;
    QString torrcPath = getTorrcDirPath();
private slots:
    void deleteAllBridges();
    bool removeBridgesFromTorrc();

private:
    QPushButton *deleteButton;
private slots:
    void toggleBridges(bool enabled);

private:
    QCheckBox *enableBridgesCheckbox;
    QSettings settings;
};

#endif // BRIDGEMANAGERDIALOG_H
