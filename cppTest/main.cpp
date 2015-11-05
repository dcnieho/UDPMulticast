#include "../UDPMultiCast.h"

#include <string>
#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
	UDPMultiCast udp;
	udp.setPort(4444);
	udp.init();
	udp.setLoopBack(TRUE);

	// send a bunch of messages
	for (int i = 0; i < 2048; i++)
	{
		udp.sendWithTimeStamp("dat,1," + std::to_string(i), ',');
	}
	std::vector<message> dataMsgs;
	udp.getData(dataMsgs);
	for (int i = 0; i < 20; i++)
	{
		udp.sendWithTimeStamp("dat,2," + std::to_string(i), ',');
	}
	udp.getData(dataMsgs);


	// send a bunch of messages
	for (int i = 0; i < 10; i++)
	{
		udp.send("cmd");
	}

	// send exit msg
	udp.send("exit");

	// clean up
	udp.deInit();
	

	return 0;
}


// function for handling errors generated by lib
void DoExitWithMsg(std::string errMsg_)
{
	std::cout << errMsg_ << std::endl;
	exit(0);
}