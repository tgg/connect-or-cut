SRC := connect-or-cut.c
OBJ := $(SRC:.c=.o)
TGT := libconnect-or-cut.so

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
	rm -f $(OBJ) $(TGT)

$(TGT): $(OBJ)
	$(CC) -shared -o $(TGT) $(OBJ) $(LDFLAGS)
