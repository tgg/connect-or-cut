SRC := connect-or-cut.c
OBJ := $(SRC:.c=.o)
TGT := libconnect-or-cut.so

CC      += -pthread
CFLAGS  += -Wall -fPIC

ifeq ($(stealth),1)
CFLAGS  += -DCOC_STEALTH
endif

.PHONY: all
all: $(TGT)

.PHONY: clean
clean:
	rm -f $(OBJ) $(TGT)

$(TGT): $(OBJ)
	$(CC) -shared -o $(TGT) $(OBJ) -ldl
