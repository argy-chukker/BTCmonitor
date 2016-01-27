HEADERS_PATH = ./

CFLAGS = -std=c++14 -O3

SRC_FILES = main.cpp apiCallers.cpp financeMetrics.cpp
SRC_PATH = ./src
SOURCES = $(SRC_FILES:%.cpp=$(SRC_PATH)/%.cpp)

OMPFLAG = -fopenmp

CC = g++

LIBS = curl ncurses

OUT_PATH = ./bin

all:
	mkdir -p ${OUT_PATH}
	${CC} ${CFLAGS} -I ${HEADERS_PATH} ${SOURCES} $(LIBS:%=-l %) -o ${OUT_PATH}/BTCMonitor.so ${OMPFLAG}
