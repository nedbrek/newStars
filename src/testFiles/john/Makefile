.PHONY: all

all:
	@echo Generating John sync test files
	@../../newStars.exe -n test.def
	diff test_master.xml in/
	diff test_P1_Y2400.xml in/
	diff test_P2_Y2400.xml in/
	@echo John sync test passed
	@rm -f *.xml

