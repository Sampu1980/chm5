#
# Infinera 2020
#

CXX = g++
CXXFLAGS += -std=c++11 -I./ -I/home/infinera/h/ -I/home/infinera/grpc/include
LDFLAGS += -lgrpc++ -lprotobuf -lpthread -ldl -lgpr -lcrypto -lssl \
		   -lgrpc++_reflection

CC_SRC = $(wildcard *.cc)
CPP_SRC = $(wildcard *.cpp)
CC_OBJ = $(patsubst %cc, %o, $(CC_SRC))
CPP_OBJ = $(patsubst %cpp, %o, $(CPP_SRC))
LIB_PATH = -L /home/infinera/lib -L /home/infinera/grpc/libs/opt
LIB = -lclient
TARGET = ct

$(TARGET) : $(CC_OBJ) $(CPP_OBJ)
	@echo "Creating test executable: $(TARGET)"
	$(CXX) $^ $(LDFLAGS) -o $@ $(LIB_PATH) $(LIB)

clean:
	rm -f *.o $(TARGET)
