include Makefile.inc

DIRS	= protocols sim
EXE	= sim_trace
OBJS	=
LIBDIR	= lib
OBJLIBS	= $(LIBDIR)/libprotocols.a $(LIBDIR)/libsim.a
LIBS	= -L$(LIBDIR)/ -lprotocols -lsim

all : $(EXE)

$(EXE) : $(OBJLIBS)
	g++ -o $(EXE) $(OBJS) $(LIBS)

$(LIBDIR)/libprotocols.a : $(LIBDIR)
	cd protocols; $(MAKE) $(MFLAGS)

$(LIBDIR)/libsim.a : $(LIBDIR)
	cd sim; $(MAKE) $(MFLAGS)

.PHONY.: clean
clean :
	$(ECHO) cleaning up in .
	-$(RM) -rf $(EXE) $(OBJS) $(OBJLIBS) $(LIBDIR)
	-for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

$(LIBDIR) :
	-mkdir -p $(LIBDIR)
