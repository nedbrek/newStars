# all tests, passing tests
atests = bombingTest buildTest colonizeTest followTest fTest loadUnload shipBuildTest shipBuildTestFail twoP
ptests = ls_tiny_sparse
tp = $(addsuffix .pass,$(ptests))

.PHONY: clean all

all: $(tp)
	@echo Regression Passed

$(tp):
	@$(MAKE) -C $(subst .pass,,$@)

