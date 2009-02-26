#ifndef __LCD_MONITOR_H_
#define __LCD_MONITOR_H_

#include <signal.h>
#include <time.h>

#include <sstream>
#include <string>
#include <vector>

#include "CFA635.h"

class Menu;
class Dialog;

class LCDMonitor:public CFA635
{
	public:
	
		LCDMonitor(int,char **);
		~LCDMonitor();
	
		void run();
		void showStatus();
		void execMenu();
		bool execDialog(Dialog *);
		
		// callbacks for whatever - public so menu can access
		
		void showAlarms();
		
		void networkDisable();
		void networkConfigDHCP();
		void networkConfigApply();
		
		void networkConfigStaticIPv4Address();
		void networkConfigStaticIPv4NetMask();
		void networkConfigStaticIPv4Gateway();
		void networkConfigStaticIPv4NameServer();
		
		void networkConfigStaticIPv6Address();
		void networkConfigStaticIPv6NetMask();
		void networkConfigStaticIPv6Gateway();
		void networkConfigStaticIPv6NameServer();
		
		void restartGPS();
		void restartNtpd();
		void reboot();
		void poweroff();
		
	private:
	
		enum LEDState {Off,RedOn,GreenOn};
		
		static void signalHandler(int);
		void startTimer(long secs=10);
		void stopTimer();
		struct sigaction sa;
		static bool timeout;
		
		void init();
		void configure();
		void log(std::string );
		void touchLock();

		void showHelp();
		void showVersion();
		
		void makeMenu();
	
		void getResponse();
		void clearDisplay();
		void showCursor(bool);
		void updateLine(int,std::string);
		void updateStatusLED(int,LEDState);
		void statusLEDsOff();
		void updateCursor(int,int);
		
		bool checkAlarms();
		bool checkGPS(int *,std::string &,bool *);
		
		bool checkFile(const char *);
		time_t lastLazyCheck;
		
		void parseNetworkConfig();
		void parseConfigEntry(std::string &,std::string &,char );

		std::string user;

		std::string poweroffCommand;
		std::string rebootCommand;
		std::string ntpdRestartCommand;
		std::string gpsRxRestartCommand;
		std::string gpsLoggerLockFile;

		std::string bootProtocol;
		std::string ipv4addr,ipv4nm,ipv4gw,ipv4ns;
		
		std::string status[4];
		LEDState    statusLED[4];
		std::vector<std::string> alarms;
		
		bool showPRNs;

		Menu *menu;
		
		std::string logFile;
		std::string lockFile;
		std::string alarmPath;

		std::string NTPuser,GPSCVuser;
		
		std::string DNSconf,networkConf,eth0Conf;
		
		std::string receiverName;
		std::string rbStatusFile,GPSStatusFile; 
};

#endif
