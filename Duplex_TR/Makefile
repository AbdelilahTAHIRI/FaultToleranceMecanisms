BIN=./bin/
IDIR=./include/
SOURCE=./
BUILD=./build/

CC=gcc
CFLAGS=-I $(IDIR)
LIST=$(BIN)/main $(BIN)/server $(BIN)/sensor $(BIN)/watchdog $(BIN)/killer $(BIN)/kick $(BIN)/injector
DEPS = $(IDIR)/main.h $(IDIR)/sensor.h  $(IDIR)/server.h $(IDIR)/watchdog.h $(IDIR)/killer.h $(IDIR)/kick.h $(IDIR)/injector.h 


all: $(LIST)


$(BIN)/%: $(SOURCE)%.c $(IDIR)/%.h
	$(CC) -o $@ $< $(CFLAGS) -lrt

.PHONY: clean

clean:
	rm -f $(BIN)/* *~ core $(INCDIR)/*~
	rm -f $(BUILD)/*.o *~ core $(INCDIR)/*~

