#include "mainwindow.h"
#include <QDateTime>
#include <QRegularExpressionValidator>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    initDB();
    setupUI();
    updateStats();
    this->setWindowTitle("Smart Parking Pro - C++ Edition");
    this->resize(800, 600);
}

void MainWindow::initDB() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("parking_system.db");
    if (db.open()) {
        QSqlQuery q;
        // Table for current parked vehicles
        q.exec("CREATE TABLE IF NOT EXISTS bookings (id INTEGER PRIMARY KEY, slot INTEGER, plate TEXT UNIQUE, owner TEXT, phone TEXT, type TEXT, model TEXT, entry TEXT)");
        // Table for revenue history
        q.exec("CREATE TABLE IF NOT EXISTS history (id INTEGER PRIMARY KEY, amount REAL, exit_date TEXT, vehicle_type TEXT)");
    }
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Stats Section
    QGridLayout *statsLayout = new QGridLayout();
    lblStatus = new QLabel("Occupancy: 0/500");
    lblStatus->setStyleSheet("font-size: 18px; font-weight: bold; color: #38bdf8;");
    lblRevenue = new QLabel("Today's Revenue: ₹0.00");
    lblRevenue->setStyleSheet("font-size: 18px; font-weight: bold; color: #fbbf24;");

    occupancyBar = new QProgressBar();
    occupancyBar->setRange(0, TOTAL_SLOTS);

    statsLayout->addWidget(lblStatus, 0, 0);
    statsLayout->addWidget(lblRevenue, 0, 1);
    statsLayout->addWidget(occupancyBar, 1, 0, 1, 2);
    mainLayout->addLayout(statsLayout);

    // Entry Form
    QGroupBox *formGroup = new QGroupBox("Vehicle Entry Registration");
    QGridLayout *formLayout = new QGridLayout(formGroup);

    txtOwner = new QLineEdit(); txtOwner->setPlaceholderText("Owner Name");
    txtPhone = new QLineEdit();
    txtPhone->setPlaceholderText("10 Digit Phone");
    txtPhone->setMaxLength(10); // Strict 10 digit limit
    txtPhone->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9]*$"), this));

    txtPlate = new QLineEdit();
    txtPlate->setPlaceholderText("Plate (e.g. DL01AB1234)");
    // Indian Plate Regex validation
    txtPlate->setValidator(new QRegularExpressionValidator(QRegularExpression("^[A-Z]{2}[0-9]{1,2}[A-Z]{1,2}[0-9]{4}$"), this));

    txtModel = new QLineEdit(); txtModel->setPlaceholderText("Model (e.g. Swift)");
    comboType = new QComboBox();
    comboType->addItems({"Car", "SUV", "EV", "Bike"});

    QPushButton *btnPark = new QPushButton("Park Vehicle");
    btnPark->setStyleSheet("background-color: #238636; color: white; font-weight: bold; padding: 10px;");

    formLayout->addWidget(new QLabel("Owner:"), 0, 0); formLayout->addWidget(txtOwner, 0, 1);
    formLayout->addWidget(new QLabel("Phone:"), 0, 2); formLayout->addWidget(txtPhone, 0, 3);
    formLayout->addWidget(new QLabel("Plate:"), 1, 0); formLayout->addWidget(txtPlate, 1, 1);
    formLayout->addWidget(new QLabel("Type:"), 1, 2); formLayout->addWidget(comboType, 1, 3);
    formLayout->addWidget(new QLabel("Model:"), 2, 0); formLayout->addWidget(txtModel, 2, 1);
    formLayout->addWidget(btnPark, 3, 0, 1, 4);
    mainLayout->addWidget(formGroup);

    // Checkout Section
    QGroupBox *outGroup = new QGroupBox("Vehicle Exit / Billing");
    QHBoxLayout *outLayout = new QHBoxLayout(outGroup);
    txtCheckout = new QLineEdit(); txtCheckout->setPlaceholderText("Enter Plate Number");
    QPushButton *btnOut = new QPushButton("Checkout & Exit");
    btnOut->setStyleSheet("background-color: #da3633; color: white; font-weight: bold; padding: 10px;");

    outLayout->addWidget(txtCheckout);
    outLayout->addWidget(btnOut);
    mainLayout->addWidget(outGroup);

    setCentralWidget(centralWidget);

    connect(btnPark, &QPushButton::clicked, this, &MainWindow::handleParking);
    connect(btnOut, &QPushButton::clicked, this, &MainWindow::handleCheckout);
}

void MainWindow::handleParking() {
    QString plate = txtPlate->text().toUpper();
    if(txtPhone->text().length() < 10) {
        QMessageBox::warning(this, "Error", "Phone must be 10 digits."); return;
    }

    QSqlQuery q;
    q.prepare("INSERT INTO bookings (slot, plate, owner, phone, type, model, entry) "
              "VALUES ((SELECT IFNULL(MAX(slot), 0) + 1 FROM bookings), ?, ?, ?, ?, ?, ?)");
    q.addBindValue(plate);
    q.addBindValue(txtOwner->text());
    q.addBindValue(txtPhone->text());
    q.addBindValue(comboType->currentText());
    q.addBindValue(txtModel->text());
    q.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    if(q.exec()) {
        QMessageBox::information(this, "Success", "Vehicle Parked!");
        txtPlate->clear(); txtOwner->clear(); txtPhone->clear(); txtModel->clear();
        updateStats();
    } else {
        QMessageBox::critical(this, "Error", "Vehicle already exists!");
    }
}

void MainWindow::handleCheckout() {
    QString val = txtCheckout->text().toUpper();
    QSqlQuery q;
    q.prepare("SELECT entry, type, slot FROM bookings WHERE plate = ? OR slot = ?");
    q.addBindValue(val); q.addBindValue(val);

    if(q.exec() && q.next()) {
        QDateTime entry = QDateTime::fromString(q.value(0).toString(), "yyyy-MM-dd HH:mm:ss");
        QDateTime current = QDateTime::currentDateTime();

        qint64 totalSeconds = entry.secsTo(current);
        double totalFee = 0.0;
        double minutes = (double)totalSeconds / 60.0;

        // Grace Period: 2 Minutes (₹0 charge if exit within 2 mins)
        if (minutes > 2.0) {
            QString vType = q.value(1).toString();
            double ratePerHour = 20.0; //
            if(vType == "SUV") ratePerHour = 30.0;
            else if(vType == "EV") ratePerHour = 25.0;
            else if(vType == "Bike") ratePerHour = 10.0;

            totalFee = minutes * (ratePerHour / 60.0);
        }

        QSqlQuery h;
        h.prepare("INSERT INTO history (amount, exit_date, vehicle_type) VALUES (?, ?, ?)");
        h.addBindValue(totalFee);
        h.addBindValue(current.toString("yyyy-MM-dd"));
        h.addBindValue(q.value(1).toString());
        h.exec();

        QSqlQuery d;
        d.prepare("DELETE FROM bookings WHERE plate = ? OR slot = ?");
        d.addBindValue(val); d.addBindValue(val);
        d.exec();

        QString duration = (minutes < 60) ? QString("%1 Mins").arg(minutes, 0, 'f', 1)
                                          : QString("%1 Hrs").arg(minutes/60.0, 0, 'f', 1);

        QMessageBox::information(this, "Receipt", QString("Plate: %1\nDuration: %2\nTotal Fee: ₹%3")
                                                      .arg(val).arg(duration).arg(totalFee, 0, 'f', 2));
        txtCheckout->clear();
        updateStats();
    } else {
        QMessageBox::warning(this, "Error", "Vehicle not found!");
    }
}

void MainWindow::updateStats() {
    QSqlQuery q;
    if(q.exec("SELECT COUNT(*) FROM bookings")) {
        q.next();
        int count = q.value(0).toInt();
        lblStatus->setText(QString("Occupancy: %1/%2").arg(count).arg(TOTAL_SLOTS));
        occupancyBar->setValue(count);
    }

    QSqlQuery r;
    r.prepare("SELECT SUM(amount) FROM history WHERE exit_date = ?");
    r.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    if(r.exec() && r.next()) {
        lblRevenue->setText(QString("Today's Revenue: ₹%1").arg(r.value(0).toDouble(), 0, 'f', 2));
    }
}

MainWindow::~MainWindow() { db.close();
}