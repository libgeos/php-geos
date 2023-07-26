
check:
	REPORT_EXIT_STATUS=1 NO_INTERACTION=1 make test TESTS="-q --show-diff"
