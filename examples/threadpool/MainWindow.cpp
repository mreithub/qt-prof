#include "MainWindow.h"
#include "Worker.h"
#include "ui_MainWindow.h"
#include "../../Profiler.h"

#include <QDebug>
#include <QFile>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);

	m_pool = new QThreadPool();

	connect(ui->btnWorker, SIGNAL(clicked()), this, SLOT(startWorker()));
	connect(ui->btnSave, SIGNAL(clicked()), this, SLOT(saveProfile()));
}

void MainWindow::saveProfile() {
	QFile file("/tmp/profile.html");

	if (!file.open(QFile::WriteOnly)) {
		qWarning() << "Error opening /tmp/profile.html for writing: " << file.errorString();
		return;
	}

	Profiler::writeAsHtml(&file);

	file.close();
}

void MainWindow::startWorker() {
	Worker *worker = new Worker(qrand() % 10);
	m_pool->start(worker);
}

MainWindow::~MainWindow() {
	delete ui;
}
