CCFLAGS=-Wall -Wpedantic -O2 -Iinc
LDFLAGS=-lglfw
OUTNAME=outmeout
SRCDIR=src
OBJDIR=obj
SRCFILES=$(wildcard $(SRCDIR)/*.cpp)
OBJFILES=$(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCFILES))

$(OUTNAME): $(OBJFILES)
	g++ -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	g++ $(CCFLAGS) -c -o $@ $<

clean:
	rm $(OUTNAME)
	rm -r $(OBJDIR)/*
