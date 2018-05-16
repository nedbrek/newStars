# all tests, passing tests
atests = bombingTest buildTest colonizeTest followTest fTest loadUnload shipBuildTest shipBuildTestFail
ptests = bombingTest buildTest colonizeTest fTest loadUnload shipBuildTest shipBuildTestFail
tp = $(addsuffix .pass,$(ptests))

.PHONY: clean all

all: $(tp)
	@echo Regression Passed
	@rm -f *.xml

clean:
	@rm -f *.xml

$(tp):
	@$(MAKE) -C $(subst .pass,,$@)

