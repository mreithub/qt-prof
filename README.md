# qt-prof - Qt invocation tracker

This project 



## Usage

Simply copy the `Profiler.cpp` and `Profiler.h` files into your project and add the following to your `.pro` file:

```qmake
DEFINES += USE_PROFILER
```

This gives you access to the following profiling macros:

- `PROFILE(scope, name)` track this function (and store the results under the given name and scope (both are strings))
- `PROFILE_FUNC(scope)` same as above, but uses the current function/method name as `name` parameter
- `PROFILE_SNAPSHOT(name)` create a snapshot (allows you to track invocations in different stages of your application)
- `CHECK_THREAD()` convenience macro for making sure a method is called in the right thread (only works in QObject subclasses - and doesn't profile invocations)

If you don't specify the `USE_PROFILER` preprocessor macro, above macros will be replaced by no-op macros (allowing you to disable profiling in production)



## Examples:

### Making sure code runs in the right QThread:

The profiler data is grouped by thread, so it's relatively easy to spot threading mistakes using `PROFILE()`

Have a look at the [examples/threadpool](examples/threadpool/) directory for an example

### Tracking SQL statements:

```cpp
// Use this as a wrapper for all your SQL queries to track their speed and invocation count
// (this could help identify bottlenecks (n+1 issues, slow queries, etc.)
bool DaoClass::executeQuery(QSqlQuery &query, const QString &sql, const QVariantList &params) {
	PROFILE("database", sql);
	bool rc = false;

	if (query.prepare(sql)) {
		foreach (QVariant param, params) {
			query.addBindValue(param);
		}

#if 0 // print all queries
		// qDebug("SQL: %s", _(sql));
		foreach (QVariant par, params) {
			qDebug(" - param(%s): %s", par.typeName(), _(par.toString()));
		}
#endif

		if (query.exec()) {
			rc = true;
		}
		else qWarning("Error executing database query '%s': %s", _(sql), _(query.lastError()));
	}
	else qWarning("Error preparing database query '%s': %s", _(sql), _query.lastError()));

	return rc;
}

```
