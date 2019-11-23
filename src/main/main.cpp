#include "hexabitz/ProxyModule.h"
#include "helper/helper.h"

#include "hal/Serial.h"
#include "hexabitz/BOS.h"
#include "hexabitz/Service.h"
#include "hexabitz/BOSMessage.h"
#include "hexabitz/BOSMessageBuilder.h"

#include "Config.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <float.h>
#include <errno.h>


void exampleTerminal(HardwareSerial& serial)
{
	while (1) {
		std::string str;
		std::cin >> str;
		serial.println(str.c_str());

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		while (serial.available())
			std::cout << char(serial.read());
	}
}

void testBinaryMessage(int times = -1)
{
	hstd::setCLIRespDefault(true);
	hstd::setTraceDefault(true);

	while (times) {
		hstd::Message m = hstd::make_message(hstd::Addr_t(0,1), hstd::Addr_t(0,1), CODE_hi);

		// std::cout << "Sending: " << m << std::endl;
		Service::getInstance()->send(m);

		if (!Service::getInstance()->receive(m))
			std::cout << "Received: " << m << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));
		times--;
	}
}

int main(int argc, char *argv[])
{
	std::string port = "/dev/ttyUSB0";

	std::cout << "Program Started (";
	std::cout << "Major: " << VERSION_MAJOR << " ";
	std::cout << "Minor: " << VERSION_MINOR << ")" << std::endl;

	// for (auto& s: BOS::getPartNumberList())
	// 	std::cout << "Part number: " << s  << " | Num of Ports: " << BOS::getNumOfPorts(BOS::toPartNumberEnum(s)) << std::endl;

	if (argc > 1)
		port = std::string(argv[1]);

	// BinaryBuffer buffer;
	// buffer.append(uint16_t(700));
	// buffer.append(uint8_t(5));
	// buffer.append(uint16_t(20));
	// buffer.append(uint32_t(50));

	// std::cout << "8: " << unsigned(buffer.popui8()) << std::endl;
	// std::cout << "8: " << unsigned(buffer.popui8()) << std::endl;
	// std::cout << "8: " << unsigned(buffer.popui8()) << std::endl;
	// std::cout << "16: " << unsigned(buffer.popui16()) << std::endl;
	// std::cout << "32: " << unsigned(buffer.popui32()) << std::endl;

	// return 0;

	std::cout << "Connecting to port " << port << std::endl;
	Service::getInstance()->init(port);
	std::shared_ptr<ProxyModule> master = std::make_shared<ProxyModule>(BOS::HF1R0);
	master->setUID(1);
	Service::getInstance()->setOwn(master);

	// Testing Communication Link
	testBinaryMessage(1);

	std::cout << "---------------- Start EXPLORE ----------------" << std::endl;
	// Service::getInstance()->ping(1, 0);
	// Service::getInstance()->osDelay(5000);
	int status = Service::getInstance()->Explore();
	std::cout << "Status: " << strerror(-status) << std::endl;
	std::cout << "---------------- Stop  EXPLORE ----------------" << std::endl;

	// testBinaryMessage();

	H01R0 module;
	while (true) {
		int red = 0, green = 0, blue = 0, intensity = 100;
		std::cin >> red >> green >> blue >> intensity;
		module.setRGB(red, green, blue, intensity);
	}

	std::cout << "Closing Program" << std::endl;
	return 0;
}