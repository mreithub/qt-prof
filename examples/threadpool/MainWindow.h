#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThreadPool>

namespace Ui {
	class MainWindow;
}

class MainWindow: public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void startWorker();
	void saveProfile();

private:
	Ui::MainWindow *ui;

	QThreadPool *m_pool;
};

#endif // MAINWINDOW_H
