#include "Profiler.h"

#ifdef USE_PROFILER

#include <QDateTime>
#include <QIODevice>
#include <time.h>


Profiler::Snapshot *Profiler::m_data = new Profiler::Snapshot("initial");

Profiler::SnapshotPrivate::~SnapshotPrivate() {
	/// @todo cleanp data
}

Profiler::Snapshot::Snapshot(const QString &name, const Snapshot *parent): d(new SnapshotPrivate()) {
	d->m_name = name;
	d->m_parent = parent;
	if (parent != NULL) d->m_index = parent->getIndex()+1;
	else d->m_index = 0;

	d->m_ts = QDateTime::currentDateTime();
}

Profiler::Snapshot::Snapshot(): d(new SnapshotPrivate()) {
}

Profiler::Snapshot Profiler::Snapshot::clone() const {
	QReadLocker locker(&d->m_lock); Q_UNUSED(locker);
	Snapshot rc(getName(), NULL);
	rc.d->m_index = getIndex();
	rc.d->m_ts = d->m_ts;

	foreach (QThread *thread, d->m_data.keys()) {
		rc.d->m_data.insert(thread, d->m_data.value(thread)->copy());
	}

	return rc;
}

Profiler::Snapshot Profiler::Snapshot::compareTo(const Snapshot &other) const {
	Snapshot rc = this->clone();

	if (!other.isNull()) {
		QMap<QThread*, Data*> otherData = other.getAllData();
		foreach (QThread *thread, rc.d->m_data.keys()) {
			if (otherData.contains(thread)) {
				Data *otherValue = otherData.value(thread);
				Data *thisValue = rc.d->m_data.value(thread);
				thisValue->_recSubstract(otherValue);
			}
		}
	}

	return rc;
}

Profiler::Snapshot* Profiler::Snapshot::createSnapshot(const QString &name) const {
	Snapshot *rc = new Snapshot(name, this);
	rc->d->m_data = getAllData();

	return rc;
}

QMap<QThread *, Profiler::Data *> Profiler::Snapshot::getAllData() const {
	QMap<QThread*, Data*> rc;
	QReadLocker locker(&d->m_lock);
	Q_UNUSED (locker);

	// make a deep copy
	foreach (QThread* thread, d->m_data.keys()) {
		Data *data = d->m_data.value(thread);

		if (!rc.contains(thread)) rc.insert(thread, data->copy());
	}

	return rc;
}

Profiler::Data* Profiler::Snapshot::getLocalData() const {
	QThread *currentThread = QThread::currentThread();
	d->m_lock.lockForRead();
	if (!d->m_data.contains(currentThread)) {
		d->m_lock.unlock();
		d->m_lock.lockForWrite();
		const_cast<QMap<QThread*,Data*>& >(d->m_data).insert(currentThread, new Data());
		d->m_lock.unlock();
		d->m_lock.lockForRead();
	}
	Data *rc = d->m_data.value(currentThread);
	d->m_lock.unlock();

	return rc;
}

const Profiler::Snapshot* Profiler::Snapshot::getParent(int index) const {
	const Snapshot *cp = this;
	while (cp != NULL) {
		if (cp->getIndex() == index) return cp;
		cp = cp->d->m_parent;
	}

	return NULL;
}

Profiler::Data::Data(Data *parent) {
	m_parent = parent;
	m_calls = 0;
	m_usec = 0;
}

Profiler::Data::~Data() {
	foreach (Data *child, m_children.values()) {
		delete child;
	}
}

void Profiler::Data::_recSubstract(Data *other) {
	m_calls -= other->getCalls();
	m_usec -= other->m_usec;
	foreach (QString childName, m_children.keys()) {
		Data *child = m_children.value(childName);
		Data *otherChild = other->getChild(childName);
		child->_recSubstract(otherChild);
	}
}

void Profiler::Data::addInvocation(qint64 usec) {
	m_usec += usec;
	m_calls++;

	if (m_parent != NULL) m_parent->addInvocation(usec);
}

Profiler::Data* Profiler::Data::copy(Data *parent) const {
	Data *rc = new Data(parent);
	rc->m_calls = m_calls;
	rc->m_usec = m_usec;

	foreach (QString name, m_children.keys()) {
		rc->m_children.insert(name, m_children.value(name)->copy(rc));
	}

	return rc;
}

Profiler::Data* Profiler::Data::getChild(const QString &name) {
	if (!m_children.contains(name)) {
		m_children.insert(name, new Data(this));
	}
	return m_children.value(name);
}

Profiler::Profiler(const QString &category, const QString &name, const char *filename, int line) {
	m_category = category;
	m_name = name;
	m_filename = filename;
	m_line = line;
	m_startUsec = _getCurrentClockUsec();
}

Profiler::~Profiler() {
	qint64 endUsec = _getCurrentClockUsec();
	qint64 duration = endUsec - m_startUsec;

	Data *data = m_data->getLocalData();
	data->getChild(m_category)->getChild(m_name)->addInvocation(duration);
}

qint64 Profiler::_getCurrentClockUsec() {
	timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_sec*1000000+(ts.tv_nsec/1000);
}

void Profiler::checkThread(const QObject *obj) {
	if (obj->thread() != QThread::currentThread()) {
		qWarning("%s has been called from another thread!", obj->metaObject()->className());
	}
}

void Profiler::createSnapshot(const QString &name) {
	m_data = m_data->createSnapshot(name);
}


void Profiler::_recWriteAsHtml(Profiler::Data *data, QIODevice *out, const QString &prefix, const QString &name) {
	QLocale cLocale = QLocale::c();
	QString formattedCalls = cLocale.toString(data->getCalls());
	QString formattedTime = cLocale.toString(data->getUsec());
	QString formattedAvg = cLocale.toString(data->getAvgUsec());

	out->write("<tr>\n");
	out->write(QString("<td>%1%2</td>\n").arg(prefix).arg(name).toLocal8Bit());
	out->write(QString("<td style=\"text-align: right\">%1</td>\n").arg(formattedCalls).toLocal8Bit());
	out->write(QString("<td style=\"text-align: right\">%1</td>\n").arg(formattedTime).toLocal8Bit());
	out->write(QString("<td style=\"text-align: right\">%1</td></tr>\n").arg(formattedAvg).toLocal8Bit());

	foreach (QString key, data->getChildNames()) {
		_recWriteAsHtml(data->getChild(key), out, prefix+"&nbsp;&nbsp;", key);
	}
}


void Profiler::writeAsHtml(QIODevice *out, int cpIndex) {
	Profiler::Snapshot cp;
	if (cpIndex >= 0) {
		cp = Profiler::getSnapshot(cpIndex);
	} else {
		cp = Profiler::getSnapshot();
	}

	QMap<QThread*, Profiler::Data*> data = cp.getAllData();

	out->write("<html><body>");
	out->write(QString("<h2> Snapshot %1: %2").arg(cp.getIndex()).arg(cp.getName()).toLocal8Bit());

	foreach (QThread *thread, data.keys()) {
		Profiler::Data *threadData = data.value(thread);
		out->write(QString("<h1>Thread %1 (0x%2)</h1>\n").arg(thread->objectName()).arg((quint64) thread, 8, 16, QChar('0')).toLocal8Bit());

		out->write("<table style=\"width:100%; font-family: monospace\"><tr><th>name</th><th>calls</th><th>total time</th><th>avg time</th></tr>\n");
		if (!threadData->isEmpty()) {
			_recWriteAsHtml(threadData, out, "", "/");
		}
		else {
			out->write("<tr><td colspan=\"4\"><em>-- none -- </em></td></tr>\n");
		}
		out->write("</table>\n");
	}

	out->write("</body></html>");

	// cleanup:
	foreach (Profiler::Data *item, data.values()) {
		delete item;
	}
}


Profiler::Snapshot Profiler::getSnapshot() {
	return m_data->clone();
}

Profiler::Snapshot Profiler::getSnapshot(int index) {
	const Snapshot *cp = m_data->getParent(index);
	if (cp != NULL) return cp->clone();
	else return Snapshot();
}

#endif // #ifdef USE_PROFILER
