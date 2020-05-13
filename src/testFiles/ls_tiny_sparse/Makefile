.PHONY: all

TCL:=tclsh
#TCL:=tclsh85

DIFF := diff -w

all:
	@cp -f 0/tiny_sparse_master.xml .
	@../../newStars.exe -r tiny_sparse_master.xml
	@$(TCL) orders_P1_2400.tcl
	@$(TCL) orders_P2_2400.tcl
	@../../newStars.exe -g tiny_sparse_master.xml
	$(DIFF) tiny_sparse_master.xml 1
	$(DIFF) tiny_sparse_P1_Y2401.xml 1
	$(DIFF) tiny_sparse_P2_Y2401.xml 1
	@$(TCL) orders_P1_2401.tcl
	@../../newStars.exe -g tiny_sparse_master.xml
	$(DIFF) tiny_sparse_master.xml 2
	$(DIFF) tiny_sparse_P1_Y2402.xml 2
	$(DIFF) tiny_sparse_P2_Y2402.xml 2
	@$(TCL) orders_P1_2402.tcl
	@../../newStars.exe -g tiny_sparse_master.xml
	$(DIFF) tiny_sparse_master.xml 3
	$(DIFF) tiny_sparse_P1_Y2403.xml 3
	$(DIFF) tiny_sparse_P2_Y2403.xml 3
	@$(TCL) orders_P1_2403.tcl
	@../../newStars.exe -g tiny_sparse_master.xml
	$(DIFF) tiny_sparse_master.xml 4
	$(DIFF) tiny_sparse_P1_Y2404.xml 4
	$(DIFF) tiny_sparse_P2_Y2404.xml 4
	@$(TCL) orders_P1_2404.tcl
	@../../newStars.exe -g tiny_sparse_master.xml
	$(DIFF) tiny_sparse_master.xml 5
	$(DIFF) tiny_sparse_P1_Y2405.xml 5
	$(DIFF) tiny_sparse_P2_Y2405.xml 5
	@$(TCL) orders_P1_2405.tcl
	@../../newStars.exe -g tiny_sparse_master.xml
	$(DIFF) tiny_sparse_master.xml 6
	$(DIFF) tiny_sparse_P1_Y2406.xml 6
	$(DIFF) tiny_sparse_P2_Y2406.xml 6

