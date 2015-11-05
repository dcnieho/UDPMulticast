#include "../UDPMultiCast.h"
#include "mex.h"
#include "class_handle.hpp"
#include "../utils.h"

#include <cwchar>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	// Get the command string
	char cmd[64];
	if (nrhs < 1 || mxGetString(prhs[0], cmd, sizeof(cmd)))
		mexErrMsgTxt("First input should be a command string less than 64 characters long.");

	// New
	if (!strcmp("new", cmd)) {
		// Check parameters
		if (nlhs != 1)
			mexErrMsgTxt("New: One output expected.");
		// Return a handle to a new C++ instance
		plhs[0] = convertPtr2Mat<UDPMultiCast>(new UDPMultiCast);
		return;
	}

	// Check there is a second input, which should be the class instance handle
	if (nrhs < 2)
		mexErrMsgTxt("Second input should be a class instance handle.");

	// Delete
	if (!strcmp("delete", cmd)) {
		// Destroy the C++ object
		destroyObject<UDPMultiCast>(prhs[1]);
		// Warn if other commands were ignored
		if (nlhs != 0 || nrhs != 2)
			mexWarnMsgTxt("Delete: Unexpected arguments ignored.");
		return;
	}

	// Get the class instance pointer from the second input
	UDPMultiCast *UDPinstance = convertMat2Ptr<UDPMultiCast>(prhs[1]);

	// Call the various class methods
	switch (str2int(cmd))
	{
	case str2int("init"):
		// Check parameters
		if (nlhs < 0 || nrhs < 2)
			mexErrMsgTxt("init: Unexpected arguments.");
		// Call the method
		UDPinstance->init();
		return;
	case str2int("deInit"):
		// Check parameters
		if (nlhs < 0 || nrhs < 2)
			mexErrMsgTxt("deInit: Unexpected arguments.");
		// Call the method
		UDPinstance->deInit();
		return;
	case str2int("sendWithTimeStamp"):
	{
		// Check parameters
		if (nlhs < 0 || nrhs < 3)
			mexErrMsgTxt("sendWithTimeStamp: Expected msg input.");
		if (!mxIsChar(prhs[2]))
			mexErrMsgTxt("sendWithTimeStamp: Expected msg argument to be a string.");
		char delim = ',';	// default
		if (nrhs > 3)
		{
			// custom delimiter
			if (!mxIsChar(prhs[3]) || !mxIsScalar(prhs[3]))
				mexErrMsgTxt("sendWithTimeStamp: Expected delimiter argument to be a scalar string.");
			mxChar delimMx = *mxGetChars(prhs[3]);
			int delimPromo = wctob(delimMx);
			if (delim==EOF)
				mexErrMsgTxt("sendWithTimeStamp: Delimiter cannot be a unicode or multibyte character.");
			delim = delimPromo;
		}
		// Call the method
		char *str;
		UDPinstance->sendWithTimeStamp(str = mxArrayToString(prhs[2]),delim);
		mxFree(str);
		return;
	}
	case str2int("send"):
	{
		// Check parameters
		if (nlhs < 0 || nrhs < 3)
			mexErrMsgTxt("send: Expected msg input.");
		if (!mxIsChar(prhs[2]))
			mexErrMsgTxt("send: Expected msg argument to be a string.");
		// Call the method
		char *str;
		UDPinstance->send(str = mxArrayToString(prhs[2]));
		mxFree(str);
		return;
	}
	case str2int("getLoopBack"):
	{
		// Check parameters
		if (nlhs < 1 || nrhs < 2)
			mexErrMsgTxt("Train: Unexpected arguments.");
		// Call the method
		auto loopback = !!UDPinstance->getLoopBack();
		plhs[0] = mxCreateLogicalScalar(loopback);
		return;
	}
	case str2int("setLoopBack"):
	{
		// Check parameters
		if (nlhs < 0 || nrhs < 3)
			mexErrMsgTxt("setLoopBack: Expected loopback input.");
		if (!(mxIsDouble(prhs[2]) && !mxIsComplex(prhs[2]) && mxIsScalar(prhs[2])) && !mxIsLogicalScalar(prhs[2]))
			mexErrMsgTxt("setLoopBack: Expected argument to be a logical scalar.");
		BOOL loopback;
		if (mxIsDouble(prhs[2]))
			loopback = !!mxGetScalar(prhs[2]);
		else
			loopback = mxIsLogicalScalarTrue(prhs[2]);
		// Call the method
		UDPinstance->setLoopBack(loopback);
		return;
	}
	case str2int("setGroupAddress"):
	{
		// Check parameters
		if (nlhs < 0 || nrhs < 3)
			mexErrMsgTxt("setGroupAddress: Expected groupAddress input.");
		if (!mxIsChar(prhs[2]))
			mexErrMsgTxt("setGroupAddress: Expected groupAddress argument to be a string.");
		// Call the method
		char *str;
		UDPinstance->setGroupAddress(str = mxArrayToString(prhs[2]));
		mxFree(str);
		return;
	}
	case str2int("setPort"):
		// Check parameters
		if (nlhs < 0 || nrhs < 3)
			mexErrMsgTxt("setPort: Expected port input.");
		if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
			mexErrMsgTxt("setPort: Expected argument to be a double scalar.");
		// Call the method
		UDPinstance->setPort(static_cast<unsigned short>(mxGetScalar(prhs[2])));
		return;
	case str2int("getData"):
	{
		// Check parameters
		if (nlhs < 1 || nrhs < 2)
			mexErrMsgTxt("getData: Unexpected arguments.");
		// Call the method
		std::vector<message> dataMsgs = UDPinstance->getData();
		// set output
		const char* fieldNames[] = { "msg", "timeStamp" };
		plhs[0] = mxCreateStructMatrix(1, dataMsgs.size(), sizeof(fieldNames) / sizeof(*fieldNames), fieldNames);
		size_t i = 0;
		for (auto &msg : dataMsgs)
		{
			mxSetFieldByNumber(plhs[0], i, 0, mxCreateString(msg.text));
			mxArray *temp;
			mxSetFieldByNumber(plhs[0], i, 1, temp=mxCreateNumericMatrix(1,1, mxINT64_CLASS, mxREAL));
			*static_cast<long long*>(mxGetData(temp)) = msg.timeStamp;
			i++;
		}
		return;
	}
	case str2int("getCommands"):
	{
		// Check parameters
		if (nlhs < 1 || nrhs < 2)
			mexErrMsgTxt("getCommands: Unexpected arguments.");
		// Call the method
		std::vector<message> cmdMsgs = UDPinstance->getCommands();
		// set output
		const char* fieldNames[] = { "msg", "timeStamp" };
		plhs[0] = mxCreateStructMatrix(1, cmdMsgs.size(), sizeof(fieldNames) / sizeof(*fieldNames), fieldNames);
		size_t i = 0;
		for (auto &msg : cmdMsgs)
		{
			mxSetFieldByNumber(plhs[0], i, 0, mxCreateString(msg.text));
			i++;
		}
		return;
	}
	default:
		// Got here, so command not recognized
		mexErrMsgTxt("Command not recognized.");
	}
}


// function for handling errors generated by lib
void DoExitWithMsg(std::string errMsg_)
{
	mexErrMsgTxt(errMsg_.c_str());
}