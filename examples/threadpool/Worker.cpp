#include "Worker.h"

#include "../../Profiler.h"

#include <QThread>
#include <unistd.h>

QAtomicInt Worker::m_sequence;

Worker::Worker(int timeout) {
	m_id = m_sequence.fetchAndAddOrdered(1);
	m_timeout = timeout;
}

void Worker::run() {
	PROFILE("example", "worker");
	qDebug("Worker %d started (thread: 0x%08llx)", m_id, QThread::currentThread());
	sleep(m_timeout);
	qDebug("Worker %d finished (thread: 0x%08llx)", m_id, QThread::currentThread());
}
