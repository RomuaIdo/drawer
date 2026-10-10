#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class SmdStrip;

// Ohm's law: fill any two fields to compute the others.
class OhmWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OhmWidget(QWidget *parent = nullptr);

private:
    void recalculate();

    QLineEdit *m_volts;
    QLineEdit *m_amps;
    QLineEdit *m_ohms;
    QLineEdit *m_watts;
    QLabel *m_result;
};

class DividerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DividerWidget(QWidget *parent = nullptr);

private:
    void recalculate();

    QLineEdit *m_vin;
    QLineEdit *m_r1;
    QLineEdit *m_r2;
    QLabel *m_result;
};

class LedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LedWidget(QWidget *parent = nullptr);

private:
    void recalculate();

    QLineEdit *m_supply;
    QLineEdit *m_forward;
    QLineEdit *m_current;
    QLabel *m_result;
};

class SmdWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SmdWidget(QWidget *parent = nullptr);

private:
    void recalculate();

    SmdStrip *m_strip;
    QLineEdit *m_marking;
    QLabel *m_result;
};
