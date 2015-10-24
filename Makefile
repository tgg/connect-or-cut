SRC := connect-or-cut.c
OBJ := $(SRC:.c=.o)
TGT := libconnect-or-cut.so

OPTION_STEALTH_1 := -DCOC_STEALTH

CC      += -pthread
CFLAGS  += -Wall -fPIC
CPPFLAGS+= ${OPTION_STEALTH_${stealth}}

.PHONY: all
all: $(TGT)

.PHONY: clean
clean:
	rm -f $(OBJ) $(TGT)

$(TGT): $(OBJ)
	$(CC) -shared -o $(TGT) $(OBJ) -ldl
