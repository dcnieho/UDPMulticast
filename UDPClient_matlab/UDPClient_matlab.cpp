#include "../UDPMultiCast.h"
#define DLL_EXPORT_SYM __declspec(dllexport) 
#include "mex.h"
#include "class_handle.hpp"
#include "../str2int.h"

#include <cwchar>

mxArray* msgVectorToMatlab(std::vector<message> msgs_);

void DLL_EXPORT_SYM mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
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
    case str2int("checkReceiverThreads"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("checkReceiverThreads: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->checkReceiverThreads());
        return;
    case str2int("getData"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getData: Unexpected arguments.");
        // Call the method
        plhs[0] = msgVectorToMatlab(UDPinstance->getData());
        return;
    case str2int("getCommands"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getCommands: Unexpected arguments.");
        // Call the method
        plhs[0] = msgVectorToMatlab(UDPinstance->getCommands());
        return;
    case str2int("getLoopBack"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getLoopBack: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateLogicalScalar(!!UDPinstance->getLoopBack());
        return;
    case str2int("setUseWTP"):
    {
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setUseWTP: Expected loopback input.");
        if (!(mxIsDouble(prhs[2]) && !mxIsComplex(prhs[2]) && mxIsScalar(prhs[2])) && !mxIsLogicalScalar(prhs[2]))
            mexErrMsgTxt("setUseWTP: Expected argument to be a logical scalar.");
        bool useWTP;
        if (mxIsDouble(prhs[2]))
            useWTP = !!mxGetScalar(prhs[2]);
        else
            useWTP = mxIsLogicalScalarTrue(prhs[2]);
        // Call the method
        UDPinstance->setUseWTP(useWTP);
        return;
    }
    case str2int("setMaxClockRes"):
    {
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setMaxClockRes: Expected loopback input.");
        if (!(mxIsDouble(prhs[2]) && !mxIsComplex(prhs[2]) && mxIsScalar(prhs[2])) && !mxIsLogicalScalar(prhs[2]))
            mexErrMsgTxt("setMaxClockRes: Expected argument to be a logical scalar.");
        bool setMaxClockRes;
        if (mxIsDouble(prhs[2]))
            setMaxClockRes = !!mxGetScalar(prhs[2]);
        else
            setMaxClockRes = mxIsLogicalScalarTrue(prhs[2]);
        // Call the method
        UDPinstance->setMaxClockRes(setMaxClockRes);
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
    case str2int("getGroupAddress"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getGroupAddress: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateString(UDPinstance->getGroupAddress().c_str());
        return;
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
    case str2int("getPort"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getPort: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->getPort());
        return;
    case str2int("setPort"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setPort: Expected port input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setPort: Expected argument to be a double scalar.");
        // Call the method
        UDPinstance->setPort(static_cast<unsigned short>(mxGetScalar(prhs[2])));
        return;
    case str2int("getBufferSize"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getBufferSize: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->getBufferSize());
        return;
    case str2int("setBufferSize"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setBufferSize: Expected buffer size input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setBufferSize: Expected argument to be a double scalar.");
        // Call the method
        UDPinstance->setBufferSize(static_cast<size_t>(mxGetScalar(prhs[2])));
        return;
    case str2int("getNumQueuedReceives"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getNumQueuedReceives: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->getNumQueuedReceives());
        return;
    case str2int("setNumQueuedReceives"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setNumQueuedReceives: Expected number of queued receives input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setNumQueuedReceives: Expected argument to be a double scalar.");
        // Call the method
        UDPinstance->setNumQueuedReceives(static_cast<unsigned long>(mxGetScalar(prhs[2])));
        return;
    case str2int("getNumReceiverThreads"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getNumReceiverThreads: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->getNumReceiverThreads());
        return;
    case str2int("setNumReceiverThreads"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setNumReceiverThreads: Expected number of receiver threads input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setNumReceiverThreads: Expected argument to be a double scalar.");
        // Call the method
        UDPinstance->setNumReceiverThreads(static_cast<unsigned long>(mxGetScalar(prhs[2])));
        return;
    default:
        // Got here, so command not recognized
        mexErrMsgTxt("Command not recognized.");
    }
}

mxArray* msgVectorToMatlab(std::vector<message> msgs_)
{
    const char* fieldNames[] = { "text", "timeStamp", "ip" };
    mxArray* out = mxCreateStructMatrix(1, msgs_.size(), sizeof(fieldNames) / sizeof(*fieldNames), fieldNames);
    size_t i = 0;
    for (auto &msg : msgs_)
    {
        mxSetFieldByNumber(out, i, 0, mxCreateString(msg.text));
        mxArray *temp;
        mxSetFieldByNumber(out, i, 1, temp = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL));
        *static_cast<long long*>(mxGetData(temp)) = msg.timeStamp;
        mxSetFieldByNumber(out, i, 2, mxCreateString(msg.ip));
        i++;
    }
    return out;
}


// function for handling errors generated by lib
void DoExitWithMsg(std::string errMsg_)
{
    mexErrMsgTxt(errMsg_.c_str());
}