# invoke SourceDir generated makefile for empty.pem3
empty.pem3: .libraries,empty.pem3
.libraries,empty.pem3: package/cfg/empty_pem3.xdl
	$(MAKE) -f C:\Users\wu\WORKSP~3\POLLIN~1/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\wu\WORKSP~3\POLLIN~1/src/makefile.libs clean

