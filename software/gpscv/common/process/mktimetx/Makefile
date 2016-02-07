PROGRAM = mktimetx
CXX = g++
INCLUDE = -I/usr/local/include 
LDFLAGS= 
LIBS= -lconfigurator
CXXFLAGS= -Wall -DDEBUG -g
CFGFLAGS= 
OBJECTS = Antenna.o Counter.o GPS.o HexBin.o Main.o Receiver.o RIN2CGGTTS.o MakeTimeTransferFile.o ReceiverMeasurement.o \
	CGGTTS.o RINEX.o \
	Javad.o NVS.o TrimbleResolution.o \
	Troposphere.o Utility.o

all: $(PROGRAM)

Antenna.o: Antenna.cpp Antenna.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c Antenna.cpp
	
Counter.o: Counter.cpp Counter.h CounterMeasurement.h Debug.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c Counter.cpp

CGGTTS.o : CGGTTS.cpp CGGTTS.h  Antenna.h Counter.h Debug.h GPS.h MakeTimeTransferFile.h MeasurementPair.h Receiver.h  ReceiverMeasurement.h Utility.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c CGGTTS.cpp
	
GPS.o: GPS.cpp  Antenna.h Debug.h GPS.h MakeTimeTransferFile.h Receiver.h ReceiverMeasurement.h SVMeasurement.h Troposphere.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c GPS.cpp

HexBin.o: HexBin.cpp HexBin.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c HexBin.cpp

Javad.o: Javad.cpp Antenna.h Debug.h GPS.h HexBin.h Javad.h Receiver.h ReceiverMeasurement.h SVMeasurement.h 
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c Javad.cpp

NVS.o: NVS.cpp Antenna.h Debug.h GPS.h HexBin.h NVS.h Receiver.h ReceiverMeasurement.h SVMeasurement.h 
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c NVS.cpp
	
Main.o: Main.cpp Debug.h MakeTimeTransferFile.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c Main.cpp

Receiver.o: Receiver.cpp Antenna.h Debug.h Receiver.h ReceiverMeasurement.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c Receiver.cpp

ReceiverMeasurement.o: ReceiverMeasurement.cpp ReceiverMeasurement.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c ReceiverMeasurement.cpp

RIN2CGGTTS.o: RIN2CGGTTS.cpp RIN2CGGTTS.h Utility.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c RIN2CGGTTS.cpp

MakeTimeTransferFile.o: MakeTimeTransferFile.cpp  Antenna.h CGGTTS.h Counter.h CounterMeasurement.h Debug.h  \
	Javad.h MakeTimeTransferFile.h  MeasurementPair.h   NVS.h Receiver.h ReceiverMeasurement.h \
	RINEX.h SVMeasurement.h  TrimbleResolution.h Utility.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c MakeTimeTransferFile.cpp

RINEX.o: RINEX.cpp  Antenna.h Counter.h Debug.h  MakeTimeTransferFile.h MeasurementPair.h Receiver.h ReceiverMeasurement.h RINEX.h 
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c RINEX.cpp

TrimbleResolution.o: TrimbleResolution.cpp Antenna.h Debug.h GPS.h HexBin.h Receiver.h ReceiverMeasurement.h SVMeasurement.h TrimbleResolution.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c TrimbleResolution.cpp

Troposphere.o: Troposphere.cpp Troposphere.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c Troposphere.cpp
	
Utility.o: Utility.cpp Utility.h
	$(CXX) $(CXXFLAGS) $(CFGFLAGS) $(INCLUDE)  -c Utility.cpp
	
$(PROGRAM): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $(PROGRAM) $(OBJECTS) $(LIBS)

clean:
	rm -f *.o $(PROGRAM)
 
	