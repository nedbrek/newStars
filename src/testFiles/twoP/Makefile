.PHONY: all clean

all: gui
	@echo Applying Player 1 orders for 2408
	@tclsh85 orders_P1_2408.tcl
	@../../newStars.exe -g twoPtest_master.xml
	@echo Applying Player 1 orders for 2409
	@tclsh85 orders_P1_2409.tcl
	@../../newStars.exe -g twoPtest_master.xml
	@echo Applying Player 1 orders for 2410
	@tclsh85 orders_P1_2410.tcl
	@../../newStars.exe -g twoPtest_master.xml
	@echo Applying Player 1 orders for 2411
	@tclsh85 orders_P1_2411.tcl
	@../../newStars.exe -g twoPtest_master.xml
	@echo Applying Player 1 orders for 2412
	@tclsh85 orders_P1_2412.tcl
	@../../newStars.exe -g twoPtest_master.xml
	@../../newStars.exe -g twoPtest_master.xml
	@../../newStars.exe -g twoPtest_master.xml
	@../../newStars.exe -g twoPtest_master.xml
	@echo Two Player Test passed
#	@rm -f *.xml

gui:
	@echo Generating Two Player Test From Definition
	@../../newStars.exe -n GameDef/twoPtest.xml
	diff twoPtest_master.xml 0/
	diff twoPtest_P1_Y2400.xml 0/
	diff twoPtest_P2_Y2400.xml 0/
	@echo Applying Player 1 orders
	@tclsh85 orders_P1_2400.tcl
	@echo Applying Player 2 orders
	@tclsh85 orders_P2_2400.tcl
	@echo Generating next turn
	@../../newStars.exe -g twoPtest_master.xml
	diff twoPtest_master.xml 1/
	diff twoPtest_P1_Y2401.xml 1/
	diff twoPtest_P2_Y2401.xml 1/
	@echo Applying Player 1 orders
	@tclsh85 orders_P1_2401.tcl
	@echo Applying Player 2 orders
	@tclsh85 orders_P2_2401.tcl
	@echo Generating next turn
	@../../newStars.exe -g twoPtest_master.xml
	diff twoPtest_master.xml 2/
	diff twoPtest_P1_Y2402.xml 2/
	diff twoPtest_P2_Y2402.xml 2/
	@echo Applying Player 1 orders
	@tclsh85 orders_P1_2402.tcl
	@echo Applying Player 2 orders
	@tclsh85 orders_P2_2402.tcl
	@echo Generating next turn
	@../../newStars.exe -g twoPtest_master.xml
	diff twoPtest_master.xml 3/
	diff twoPtest_P1_Y2403.xml 3/
	diff twoPtest_P2_Y2403.xml 3/
	@../../newStars.exe -g twoPtest_master.xml
	@../../newStars.exe -g twoPtest_master.xml
	@../../newStars.exe -g twoPtest_master.xml
	@../../newStars.exe -g twoPtest_master.xml
	@../../newStars.exe -g twoPtest_master.xml

clean:
	@rm -f *.xml

