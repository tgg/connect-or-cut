SRC := connect-or-cut.c
OBJ := $(SRC:.c=.o)
TGT := libconnect-or-cut.so

OPTION_STEALTH_1 := -DCOC_STEALTH

FreeBSD_CPPFLAGS := -DHAVE_STRNDUP
NetBSD_CPPFLAGS  := -DHAVE_STRNDUP
OpenBSD_CPPFLAGS := -DHAVE_STRNDUP
Linux_CPPFLAGS   := -DMISSING_GETPROGNAME -DHAVE_STRNDUP

Linux_LDFLAGS    := -ldl
Darwin_LDFLAGS   := -ldl

CC      += -pthread
CFLAGS  += -Wall -fPIC ${${os}_CFLAGS}
CPPFLAGS+= ${OPTION_STEALTH_${stealth}} ${${os}_CPPFLAGS}
LDFLAGS += ${${os}_LDFLAGS}

.PHONY: all
all: $(TGT)

.PHONY: clean
clean:
	rm -f $(OBJ) $(TGT)

$(TGT): $(OBJ)
	$(CC) -shared -o $(TGT) $(OBJ) $(LDFLAGS)
