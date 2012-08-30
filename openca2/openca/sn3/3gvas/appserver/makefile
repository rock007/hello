CC = g++ -g
CFLAGS = -Wall -Wno-deprecated -D__Linux__

MYSQLINCLUDEPATH=/usr/local/mysql/include
MYSQLLIBPATH=/usr/local/mysql/lib


INC_PATH = -I$(MYSQLINCLUDEPATH) -I../../../gk/include -I../../../include

LIB_PATH = -L$(MYSQLLIBPATH) -L./
LIBS = ../../../gk/lib/sock.o ../../../libs/logfile.o ../../../libs/childprocess.a ../../../../bin/spiderlicense.a -lpthread -lboost_thread -lmysqlclient -ln_common -lz 

SRC = $(wildcard *.cpp)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INC_PATH) -c $?

CLEINTOBJ = appserver.o appserver_proc.o appserver_shortmsg.o appserver_sock.o \
			appserver_bussop.o appserver_db.o client_msg.o

TARGET = 3GTEST_APPSERVER

all: $(TARGET)
	mv -f $(TARGET) ../../../../bin/sn3_service_16
	@echo

$(TARGET): $(CLEINTOBJ)
	$(CC) -o $(@) $(CFLAGS) $(CLEINTOBJ) $(LIB_PATH) $(LIBS)
	
clean:
	rm -rf *.o $(TARGET)
		
cleano:
	rm -rf *.o

rebuild: clean all

update:
	cp -f ../../../../bin/sn3_service_16      /home/spider/bin/.
	@echo
