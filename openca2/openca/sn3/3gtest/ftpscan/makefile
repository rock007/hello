CC = g++ -g
CFLAGS = -Wall -Wno-deprecated -D__Linux__

MYSQLINCLUDEPATH=/usr/local/mysql/include
MYSQLLIBPATH=/usr/local/mysql/lib


INC_PATH = -I$(MYSQLINCLUDEPATH) -I../../../include

LIB_PATH = -L$(MYSQLLIBPATH) -L./
LIBS = ../../../libs/logfile.o ../../../libs/childprocess.a ../../../../bin/spiderlicense.a -lpthread -lboost_thread -lmysqlclient -ln_common -lz 

SRC = $(wildcard *.cpp)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INC_PATH) -c $?

CLEINTOBJ = ftpscan.o ftpscan_proc.o ../appserver/appserver_db.o \
			../appserver/client_msg.o 

TARGET = 3GTEST_FTPSCAN

all: $(TARGET)
	mv -f $(TARGET) ../../../../bin/sn3_service_17
	@echo

$(TARGET): $(CLEINTOBJ)
	$(CC) -o $(@) $(CFLAGS) $(CLEINTOBJ) $(LIB_PATH) $(LIBS)
	
clean:
	rm -rf *.o $(TARGET)
		
cleano:
	rm -rf *.o

rebuild: clean all

update:
	cp -f ../../../../bin/sn3_service_17      /home/spider/bin/.
	@echo
