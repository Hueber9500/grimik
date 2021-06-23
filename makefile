LIBS=-lpthread -lsqlite3

servermake: main.cpp 
	/usr/bin/g++ -g main.cpp $(LIBS) -o main