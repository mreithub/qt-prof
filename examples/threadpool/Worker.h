#ifndef WORKER_H
#define WORKER_H

#include <QAtomicInt>
#include <QRunnable>

class Worker: public QRunnable {
public:
	explicit Worker(int timeout);

	virtual void run();

private:
	int m_id;
	int m_timeout;
	static QAtomicInt m_sequence;
};

#endif // WORKER_H
