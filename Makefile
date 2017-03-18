SRC := connect-or-cut.c
OBJ := $(SRC:.c=.o)
ABI := 1
VER := $(ABI).0.2
LIB := libconnect-or-cut.so
TGT := $(LIB).$(VER)
TST := tcpcontest
LNK := $(LIB).$(ABI)

DESTDIR ?= /usr/local
DESTBIN ?= $(DESTDIR)/bin
DESTLIB ?= $(DESTDIR)/lib

OPTION_STEALTH_1 := -DCOC_STEALTH

32_CFLAGS        := -m32
32_LDFLAGS       := -m32
SunOS_LDFLAGS    := -lsocket -lnsl
SunOS_LIBFLAGS   := -h $(LNK)
Linux_LIBFLAGS   := -shared -ldl -Wl,-soname,$(LNK)
FreeBSD_LIBFLAGS := $(Linux_LIBFLAGS)
NetBSD_LIBFLAGS  := $(Linux_LIBFLAGS)
OpenBSD_LIBFLAGS := $(Linux_LIBFLAGS)
Darwin_LIBFLAGS  := -dynamiclib -flat_namespace -ldl -Wl,-dylib_install_name,$(LNK)
Darwin_CFLAGS    := -fno-common

CC      += -pthread
CFLAGS  += -Wall -fPIC ${${os}_CFLAGS} ${${bits}_CFLAGS}
CPPFLAGS+= ${OPTION_STEALTH_${stealth}} ${${os}_CPPFLAGS}
LDFLAGS += ${${os}_LDFLAGS} ${${bits}_LDFLAGS}

.PHONY: all
all: $(TGT) $(TST)

.PHONY: clean
clean:
	rm -f $(OBJ) $(TGT) $(LNK) $(TST)

$(TGT): $(OBJ)
	$(CC) -o $(TGT) $(OBJ) $(LDFLAGS) ${${os}_LIBFLAGS}
	rm -f $(LNK)
	ln -s $(TGT) $(LNK)

$(TST): $(TST).o
	$(CC) -o $(TST) $(TST).o $(LDFLAGS)

.PHONY: install
install: $(TGT)
	mkdir -p $(DESTBIN)
	install -m755 coc $(DESTBIN)
	mkdir -p $(DESTLIB)
	install -m755 $(TGT) $(DESTLIB)
	(cd $(DESTLIB) && rm -f $(LNK) && ln -s $(TGT) $(LNK))

.PHONY: test
test: $(TGT) $(TST)
	./testsuite
