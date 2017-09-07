# invoke SourceDir generated makefile for main.pem3
main.pem3: .libraries,main.pem3
.libraries,main.pem3: package/cfg/main_pem3.xdl
	$(MAKE) -f C:\Users\wu\WORKSP~3\STMREG~1/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\wu\WORKSP~3\STMREG~1/src/makefile.libs clean

