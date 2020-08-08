#pragma once

#include <QWidget>
#include "ui_tradewidget.h"

class Workshop;

class TradeWidget : public QWidget
{
	Q_OBJECT

public:
	TradeWidget( Workshop* ws, QWidget *parent = Q_NULLPTR );
	~TradeWidget();

	void updateContent();

private:
	Ui::TradeWidget ui;

	Workshop* m_workshop;

	unsigned int m_traderID = 0;

	unsigned int m_upperValue = 0;
	unsigned int m_lowerValue = 0;

	void checkText( QLineEdit* le );
	void transfer( QListWidget* fromWidget, QListWidget* toWidget, int amount, bool checkItem = false );
	
	void calcUpper();
	void calcLower();

private slots:
	void onButtonTraderLeft();
	void onButtonTraderRight();
	void onButtonGnomesLeft();
	void onButtonGnomesRight();

	void onAmount1Changed();
	void onAmount2Changed();

	void checkTrader();

	void onButtonTrade();
};
