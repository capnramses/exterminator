# uncomment for debug build
G = -g
F = -std=c99 -O0 -Wall -pedantic -fmessage-length=78 -m64

# uncomment for profiling build
#PG = -pg 
#F = -O0 -Wall -pedantic -fmessage-length=78 -m64

# uncomment for release build
#F = -Ofast -fmessage-length=78 -m64

BIN = exterminator
CC = clang
OBJ = obj/main.o obj/utils.o obj/wins.o obj/ipc.o obj/parse.o
# UNC is "use ncurses"
D = -DUNC
DEPS = -MMD -MP -MF"${@:%.o=%.d}" -MT"${@:%.o=%.d}"
INCLUDES = ${wildcard include/*.h}
L = lib/lin64/
#STA =
DYN = -lncurses
I = -I include/

.PHONY : all
all : ${OBJ} ${INCLUDES}
	${CC} ${F} ${D} -o ${BIN} ${I} ${OBJ} ${STA} ${DYN} ${PG}

obj/%.o: src/%.c ${INCLUDES}
	${CC} ${I} ${G} ${F} ${D} -c ${DEPS} -o "$@" "$<" ${PG}
