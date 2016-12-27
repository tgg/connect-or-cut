SRC := connect-or-cut.c
OBJ := $(SRC:.c=.o)
ABI := 1
VER := $(ABI).0.2
LIB := libconnect-or-cut.so
TGT := $(LIB).$(VER)
LNK := $(LIB).$(ABI)

DESTDIR ?= /usr/local
DESTBIN ?= $(DESTDIR)/bin
DESTLIB ?= $(DESTDIR)/lib

OPTION_STEALTH_1 := -DCOC_STEALTH

32_CFLAGS        := -m32
32_LDFLAGS       := -m32
SunOS_LDFLAGS    := -lsocket -lnsl -h $(LNK)
Linux_LDFLAGS    := -ldl -Wl,-soname,$(LNK)
Darwin_LDFLAGS   := -ldl -Wl,-dylib_install_name,$(LNK)

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
	$(CC) -shared -o $(TGT) $(OBJ) $(LDFLAGS)
	rm -f $(LNK)
	ln -s $(TGT) $(LNK)

.PHONY: install
install: $(TGT)
	mkdir -p $(DESTBIN)
	install -m755 coc $(DESTBIN)
	mkdir -p $(DESTLIB)
	install -m755 $(TGT) $(DESTLIB)
	(cd $(DESTLIB) && rm -f $(LNK) && ln -s $(TGT) $(LNK))
