SETTINGS = {
	:source_path => '/glibc-2.11.1/',
	:stop_on_fail => true,
	:rebuild_failed => true,
	:retry_failed => true,
	:copy_targets => [],
	:loader_base_url => 'http://localhost:5002/',
	:htdocs_dir => '/htdocs/',	# set to nil or false to disable copying.
}
