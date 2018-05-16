.PHONY: all clean

all: gui
	@echo Generating Colonize Test Files
	@../../newStars.exe -r 0/colonizeTest_master.xml
	diff colonizeTest_master.xml 0/
	diff colonizeTest_P1_Y2402.xml 0/
	@echo Generating Next Turn
	@../../newStars.exe -g colonizeTest_master.xml
	diff colonizeTest_master.xml 1/
	diff colonizeTest_P1_Y2403.xml 1/
	@echo Colonize Test Passed
	@rm -f *.xml

gui:
	@echo Generating Colonize Test From Definition
	@../../newStars.exe -n GameDef/colonizeTest.xml > colonizeTest_master.xml
	@../../newStars.exe -r colonizeTest_master.xml
	diff colonizeTest_master.xml N1/
	diff colonizeTest_P1_Y2400.xml N1/
	@echo "Applying player orders for 2400"
	@tclsh orders2400.tcl
	@echo Generating Next Turn
	@../../newStars.exe -g colonizeTest_master.xml
	diff colonizeTest_master.xml N2/
	diff colonizeTest_P1_Y2401.xml N2/
	@echo Generating Next Turn
	@../../newStars.exe -g colonizeTest_master.xml
	diff colonizeTest_master.xml N3/
	diff colonizeTest_P1_Y2402.xml N3/
	@echo "Applying player orders for 2402"
	@tclsh orders2402.tcl
	@echo Generating Next Turn
	@../../newStars.exe -g colonizeTest_master.xml
	diff colonizeTest_master.xml N4/
	diff colonizeTest_P1_Y2403.xml N4/

clean:
	@rm -f *.xml

