#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QProgressBar>
#include <QGroupBox>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleParking();
    void handleCheckout();
    void updateStats();

private:
    QLineEdit *txtPlate, *txtPhone, *txtOwner, *txtModel, *txtCheckout;
    QComboBox *comboType;
    QLabel *lblStatus, *lblRevenue;
    QProgressBar *occupancyBar;

    QSqlDatabase db;
    const int TOTAL_SLOTS = 500; // Limit set to 500
    void setupUI();
    void initDB();
};
#endif