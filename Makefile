SRC := connect-or-cut.c
OBJ := $(SRC:.c=.o)
ABI := 1
VER := $(ABI).0.1
LIB := libconnect-or-cut.so
TGT := $(LIB).$(VER)
LNK := $(LIB).$(ABI)

DESTDIR ?= /usr/local
DESTLIB ?= $(DESTDIR)/lib

OPTION_STEALTH_1 := -DCOC_STEALTH

32_CFLAGS        := -m32
32_LDFLAGS       := -m32
Linux_LDFLAGS    := -ldl
Darwin_LDFLAGS   := -ldl

CC      += -pthread
CFLAGS  += -Wall -fPIC ${${os}_CFLAGS} ${${bits}_CFLAGS}
CPPFLAGS+= ${OPTION_STEALTH_${stealth}} ${${os}_CPPFLAGS}
LDFLAGS += ${${os}_LDFLAGS} ${${bits}_LDFLAGS}

.PHONY: all
all: $(TGT)

.PHONY: clean
clean:
	rm -f $(OBJ) $(TGT) $(LNK)

$(TGT): $(OBJ)
	$(CC) -shared -Wl,-soname,$(LNK) -o $(TGT) $(OBJ) $(LDFLAGS)
	rm -f $(LNK)
	ln -s $(TGT) $(LNK)

.PHONY: install
install: $(TGT)
	mkdir -p $(DESTDIR)/bin
	install -m755 coc $(DESTDIR)/bin
	mkdir -p $(DESTLIB)
	install -m755 $(TGT) $(DESTLIB)
	(cd $(DESTLIB) && rm -f $(LNK) && ln -s $(TGT) $(LNK))
