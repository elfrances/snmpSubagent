
BIN_DIR = .
DEP_DIR = .
OBJ_DIR = .

CFLAGS = -I. -ggdb -Wall -Werror -O0
LDFLAGS = -ggdb 

SOURCES = $(wildcard *.c)
OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS := $(patsubst %.c,$(DEP_DIR)/%.d,$(SOURCES))

# Rule to autogenerate dependencies files
$(DEP_DIR)/%.d: %.c
	@set -e; $(RM) $@; \
         $(CC) -MM $(CPPFLAGS) $< > $@.temp; \
         sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)\/\1.o $@ : ,g' < $@.temp > $@; \
         $(RM) $@.temp

# Rule to generate object files
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

all: snmpSubagent

snmpSubagent: $(OBJECTS) Makefile
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/$@ $(OBJECTS) -lnetsnmp -lnetsnmpagent

clean:
	$(RM) $(OBJECTS) $(DEP_DIR)/*.d $(BIN_DIR)/snmpSubagent

include $(DEPS)

