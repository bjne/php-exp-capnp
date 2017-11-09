NAME         = capnp
PHP_DIR     ?= /usr
EXT_DIR     ?= $(shell ${PHP_DIR}/bin/php-config --extension-dir)
INCLUDES    += $(shell ${PHP_DIR}/bin/php-config --includes)

EXTENSION   ?= ${NAME}.so

CC           = g++
LD           = g++

CFLAGS      ?= -Wall -c -O2 -std=c++11 -fPIC -o
LDFLAGS     ?= -shared
LIBS        ?= -lphpcpp -lcapnpc -lcapnp -lkj

RM           = rm -f
CP           = cp -f
MKDIR        = mkdir -p

SOURCES      = $(wildcard *.c++)
OBJECTS      = $(SOURCES:%.c++=%.o)

all: ${OBJECTS} ${EXTENSION}

${EXTENSION}: ${OBJECTS}
	${LD} ${LDFLAGS} -o $@ ${OBJECTS} ${LIBS}

${OBJECTS}:
	${CC} ${INCLUDES} ${CFLAGS} $@ ${@:%.o=%.c++}

install:
	${CP} ${EXTENSION} ${EXT_DIR}

clean:
	${RM} ${EXTENSION} ${OBJECTS}

