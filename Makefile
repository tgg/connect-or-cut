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

32__CFLAGS        := -m32
32__LDFLAGS       := -m32
SunOS__LDFLAGS    := -lsocket -lnsl
SunOS_CPPFLAGS    := -DMISSING_STRNDUP
SunOS_CFLAGS      := -mt -w
SunOS_LIBFLAGS    := -G -h $(LNK)
GCC_CFLAGS        := -Wall -pthread
GCC_LIBFLAGS      := -pthread -shared -Wl,-soname,$(LNK)
Linux_LIBFLAGS    := -ldl $(GCC_LIBFLAGS)
FreeBSD_LIBFLAGS  := $(GCC_LIBFLAGS)
NetBSD_LIBFLAGS   := $(GCC_LIBFLAGS)
OpenBSD_LIBFLAGS  := $(GCC_LIBFLAGS)
DragonFly_LIBFLAGS:= $(GCC_LIBFLAGS)
Linux_CFLAGS      := $(GCC_CFLAGS)
NetBSD_CFLAGS     := $(GCC_CFLAGS)
FreeBSD_CFLAGS    := $(GCC_CFLAGS)
OpenBSD_CFLAGS    := $(GCC_CFLAGS)
DragonFly_CFLAGS  := $(GCC_CFLAGS)
Darwin_LIBFLAGS   := -dynamiclib -flat_namespace -ldl -Wl,-dylib_install_name,$(LNK)
Darwin_CFLAGS     := -fno-common

CFLAGS  += -fPIC ${${os}_CFLAGS} ${${bits}_CFLAGS}
CPPFLAGS+= ${OPTION_STEALTH_${stealth}} ${${os}_CPPFLAGS}
LDFLAGS += ${${os}__LDFLAGS} ${${bits}__LDFLAGS}

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
