#ifndef PROFILER_H
#define PROFILER_H

/*
#ifndef QT_NO_DEBUG
#	define USE_PROFILER 1
#endif
*/

#ifndef USE_PROFILER
#	define PROFILE(category, name) {}
#	define PROFILE_FUNC(category) {}
#	define PROFILE_SNAPSHOT(name) {}
#	define CHECK_THREAD {}
#else
#	define PROFILE(category, name) Profiler __profiler(category, name, __FILE__, __LINE__);
#	define PROFILE_FUNC(category) PROFILE(category, __PRETTY_FUNCTION__);
#	define PROFILE_SNAPSHOT(name) Profiler::createSnapshot(name)
#	define CHECK_THREAD Profiler::checkThread(this);

#include <QDateTime>
#include <QMap>
#include <QReadWriteLock>
#include <QString>
#include <QTextStream>
#include <QThread>

/**
 * \brief Simple class to measure a method's execution time
 *
 * The constructor of this class requires providing a (unique) name
 * for which the times will be summed up
 * \nonreentrant
 */
class Profiler {
public:
	class Snapshot;
	/**
	 * @brief Recursive, thread local Profiler data
	 *
	 * This class is NOT thead safe, but as there's different Profiler data for each thread, that won't be a problem
	 */
	class Data {
		friend class Snapshot;
	public:
		Data(Data *parent = NULL);
		~Data();

		void addInvocation(qint64 usec);

		qint64 getAvgUsec() const { return m_calls > 0 ? m_usec / m_calls : 0; }
		int getCalls() const { return m_calls; }
		qint64 getUsec() const { return m_usec; }

		Data* getChild(const QString &name);
		QList<QString> getChildNames() const { return m_children.keys(); }

		/// create and return a deep copy of this Data object
		Data* copy(Data *parent = NULL) const;

		bool isEmpty() const { return m_children.isEmpty(); }
	private:
		void _recSubstract(Data *other);

		/// total time spent in this thread
		qint64 m_usec;
		/// number of calls to this thread
		int m_calls;

		Data *m_parent;

		QMap<QString, Data*> m_children;
	};

	class SnapshotPrivate;

	/**
	 * @brief Profiler data at a certain point in time
	 */
	class Snapshot {
		friend class Profiler;
	private:
		Snapshot(const QString &name, const Snapshot *parent = NULL);

		/**
		 * @brief Create and return a new Snapshot with the given name (the data will be copied)
		 * @param name New snapshot's name
		 * @return new Snapshot instance (its parent will be set to this)
		 */
		Snapshot* createSnapshot(const QString &name) const;

		Data* getLocalData() const;

		const Snapshot* getParent() const { return d->m_parent; }
		const Snapshot* getParent(int index) const;

	public:
		Snapshot();

		/**
		 * @brief Creates and returns a clone (= deep copy) of this Snapshot instance (and only this Snapshot instance - so parent will be set to NULL)
		 * @return Snapshot clone
		 */
		Snapshot clone() const;

		/**
		 * @brief Returns the differences between two snapshots (to get relative instead of absolute data)
		 * @param other Snapshot to compare to
		 * @return Snapshot instance containing the relative differences between the two instances
		 */
		Snapshot compareTo(const Snapshot &other) const;

		/**
		 * @brief Returns this Snapshot instance's data
		 * @return Profiler data
		 */
		QMap<QThread*,Data*> getAllData() const;

		/**
		 * @brief Returns a new Snapshot instance with measurements between the current snapshot and the given one (i.e. the profiler data between those two snapshots)
		 * @param other Snapshot to compare data with
		 * @return Difference in calls (and measured time) between the two Snapshots
		 */
		Snapshot getAllDataSince(const Snapshot &other) const;

		int getIndex() const { return d->m_index; }
		QString getName() const { return d->m_name; }

		bool isNull() const { return d.isNull() || getName().isNull(); }
	private:
		QSharedPointer<SnapshotPrivate> d;
	};

	/**
	 * @brief shared Snapshot data
	 */
	class SnapshotPrivate {
	private:
		friend class Snapshot;

		const Snapshot *m_parent;
		int m_index;
		QString m_name;
		QDateTime m_ts;
		QMap<QThread*, Data*> m_data;

		mutable QReadWriteLock m_lock;

	public:
		~SnapshotPrivate();
	};

private:
	static Snapshot *m_data;

	QString m_category, m_name;
	const char *m_filename;
	int m_line;
	qint64 m_startUsec;

	static inline qint64 _getCurrentClockUsec();
	static void _recWriteAsHtml(Profiler::Data *data, QIODevice *out, const QString &prefix, const QString &name);
public:
	Profiler(const QString &category, const QString &name, const char *filename, int line);
	~Profiler();

	/**
	 * @brief Get a specific snapshot (creates a deep copy of the data)
	 * @param index snapshot index
	 * @return
	 */
	static Snapshot getSnapshot(int index);

	/**
	 * @brief Return a deep copy of the current profiler state
	 * @return
	 */
	static Snapshot getSnapshot();

	static QMap<QThread *, Data *> getCurrentStatistics();
	static void createSnapshot(const QString &name);
	static void writeAsHtml(QIODevice *output, int cpIndex = -1);


	/// Issue a qWarning if the current thread doesn't match obj's thread()
	static void checkThread(const QObject *obj);
};

#endif // USE_PROFILER

#endif // PROFILER_H
