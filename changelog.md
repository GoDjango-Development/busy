# Changelog

## [Unreleased]

### Added

* Handle `SIGTERM`, `SIGHUP`, and `SIGQUIT` in addition to `SIGINT`
* `-Wall -Wextra` warnings in release and debug builds
* Exit failure when a worker process exits unexpectedly

### Changed

* Started processes are stopped and reaped before exiting on setup failures
* Object directories are created by the build; `make prepare` is no longer required
* Worker creation errors now stop the workload instead of continuing with fewer workers

### Fixed

* Workers surviving `SIGTERM` or terminal shutdown
* Workers inheriting an ignored shutdown signal
* Interrupted `wait()` ending the reap loop early
* Shutdown signals arriving before the worker process group exists
* Header dependencies missing from object file rules
* `./busy` paths in `readme.md`; the binary is `release/busy`
* Error message typos

### Removed

*

## [1.2.0] - 2026-08-27

### Added

* Global group id for child processess
* Function to creat the group leader

### Changed

* Refactored run_busy()

### Fixed

* Return value in crt_bgchilds()
* Exit child on unsuccessful set group id

### Removed

* PIDs array table


