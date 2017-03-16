#include "UDPMultiCast/UDPMultiCast.h"
#define DLL_EXPORT_SYM __declspec(dllexport) 
#include "mex.h"
#include "class_handle.hpp"
#include "UDPMultiCast/strHash.h"
#include "UDPMultiCast/utils.h"

#include <cwchar>
#include <algorithm>

mxArray* msgVectorToMatlab(std::vector<message> msgs_);

void DLL_EXPORT_SYM mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Get the command string
    char cmd[64] = {0};
    if (nrhs < 1 || mxGetString(prhs[0], cmd, sizeof(cmd)))
        mexErrMsgTxt("First input should be a command string less than 64 characters long.");
    size_t nChar = std::min(strlen(cmd),size_t(64));

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
    switch (rt::crc32(cmd,nChar))
    {
    case ct::crc32("init"):
        // Check parameters
        if (nlhs < 0 || nrhs < 2)
            mexErrMsgTxt("init: Unexpected arguments.");
        // Call the method
        UDPinstance->init();
        return;
    case ct::crc32("deInit"):
        // Check parameters
        if (nlhs < 0 || nrhs < 2)
            mexErrMsgTxt("deInit: Unexpected arguments.");
        // Call the method
        UDPinstance->deInit();
        return;
    case ct::crc32("sendWithTimeStamp"):
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
        uint64_t timeStamp = UDPinstance->sendWithTimeStamp(str = mxArrayToString(prhs[2]),delim);
        mxFree(str);
        // return timestamp (as signed int to keep calculating with it easy)
        plhs[0] = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
        *static_cast<int64_t*>(mxGetData(plhs[0])) = timeStamp;
        return;
    }
    case ct::crc32("send"):
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
    case ct::crc32("checkReceiverThreads"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("checkReceiverThreads: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->checkReceiverThreads());
        return;
    case ct::crc32("getData"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getData: Unexpected arguments.");
        // Call the method
        plhs[0] = msgVectorToMatlab(UDPinstance->getData());
        return;
    case ct::crc32("getCommands"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getCommands: Unexpected arguments.");
        // Call the method
        plhs[0] = msgVectorToMatlab(UDPinstance->getCommands());
        return;

    case ct::crc32("getGitRefID"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getGitRefID: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateString(UDPinstance->getGitRefID().c_str());
        return;
    case ct::crc32("setUseWTP"):
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
    case ct::crc32("setMaxClockRes"):
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
    case ct::crc32("getLoopBack"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getLoopBack: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateLogicalScalar(!!UDPinstance->getLoopBack());
        return;
    case ct::crc32("setLoopBack"):
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
    case ct::crc32("getReuseSocket"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getReuseSocket: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateLogicalScalar(!!UDPinstance->getReuseSocket());
        return;
    case ct::crc32("setReuseSocket"):
    {
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setReuseSocket: Expected loopback input.");
        if (!(mxIsDouble(prhs[2]) && !mxIsComplex(prhs[2]) && mxIsScalar(prhs[2])) && !mxIsLogicalScalar(prhs[2]))
            mexErrMsgTxt("setLoopBack: Expected argument to be a logical scalar.");
        BOOL reuseSocket;
        if (mxIsDouble(prhs[2]))
            reuseSocket = !!mxGetScalar(prhs[2]);
        else
            reuseSocket = mxIsLogicalScalarTrue(prhs[2]);
        // Call the method
        UDPinstance->setReuseSocket(reuseSocket);
        return;
    }
    case ct::crc32("getGroupAddress"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getGroupAddress: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateString(UDPinstance->getGroupAddress().c_str());
        return;
    case ct::crc32("setGroupAddress"):
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
    case ct::crc32("getPort"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getPort: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->getPort());
        return;
    case ct::crc32("setPort"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setPort: Expected port input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setPort: Expected argument to be a double scalar.");
        // Call the method
        UDPinstance->setPort(static_cast<unsigned short>(mxGetScalar(prhs[2])));
        return;
    case ct::crc32("getBufferSize"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getBufferSize: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->getBufferSize());
        return;
    case ct::crc32("setBufferSize"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setBufferSize: Expected buffer size input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setBufferSize: Expected argument to be a double scalar.");
        // Call the method
        UDPinstance->setBufferSize(static_cast<size_t>(mxGetScalar(prhs[2])));
        return;
    case ct::crc32("getNumQueuedReceives"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getNumQueuedReceives: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->getNumQueuedReceives());
        return;
    case ct::crc32("setNumQueuedReceives"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setNumQueuedReceives: Expected number of queued receives input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setNumQueuedReceives: Expected argument to be a double scalar.");
        // Call the method
        UDPinstance->setNumQueuedReceives(static_cast<unsigned long>(mxGetScalar(prhs[2])));
        return;
    case ct::crc32("getNumReceiverThreads"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getNumReceiverThreads: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(UDPinstance->getNumReceiverThreads());
        return;
    case ct::crc32("setNumReceiverThreads"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setNumReceiverThreads: Expected number of receiver threads input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setNumReceiverThreads: Expected argument to be a double scalar.");
        // Call the method
        UDPinstance->setNumReceiverThreads(static_cast<unsigned long>(mxGetScalar(prhs[2])));
        return;
    case ct::crc32("setComputerFilter"):
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setComputerFilter: Expected (possibly empty) array of computers from which you want to receive messages.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]))
            mexErrMsgTxt("setComputerFilter: Array of computers from which you want to receive messages should be double.");
        // Call the method
        UDPinstance->setComputerFilter(mxGetPr(prhs[2]), mxGetNumberOfElements(prhs[2]));
        return;
    case ct::crc32("getCurrentTime"):
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getCurrentTime: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
        *static_cast<int64_t>(mxGetData(plhs[0])) = timeUtils::getTimeStamp();
        return;
    case ct::crc32("hasSMIIntegration"):
        plhs[0] = mxCreateNumericMatrix(1, 1, mxLOGICAL_CLASS, mxREAL);
        *static_cast<bool*>(mxGetData(plhs[0])) = UDPinstance->hasSMIIntegration();
        return;
#ifdef HAS_SMI_INTEGRATION
    case ct::crc32("startSMIDataSender"):
        UDPinstance->startSMIDataSender();
        return;
    case ct::crc32("removeSMIDataSender"):
        UDPinstance->removeSMIDataSender();
        return;
#endif // HAS_SMI_INTEGRATION
    case ct::crc32("hasTobiiIntegration"):
        plhs[0] = mxCreateNumericMatrix(1, 1, mxLOGICAL_CLASS, mxREAL);
        *static_cast<bool*>(mxGetData(plhs[0])) = UDPinstance->hasTobiiIntegration();
        return;
#ifdef HAS_TOBII_INTEGRATION
    case ct::crc32("connectToTobii"):
    {
        if (nrhs < 2 || !mxIsChar(prhs[2]))
            mexErrMsgTxt("connectToTobii: Third argument must be a string.");

        char* address = mxArrayToString(prhs[2]);
        UDPinstance->connectToTobii(address);
        mxFree(address);
        return;
    }
    case ct::crc32("setTobiiSampleRate"):
    {
        if (nrhs < 2 || !mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]))
            mexErrMsgTxt("setTobiiSampleRate: Third argument must be a double");

        UDPinstance->setTobiiSampleRate(static_cast<float>(*mxGetPr(prhs[2])));
        return;
    }
    case ct::crc32("startTobiiDataSender"):
        UDPinstance->startTobiiDataSender();
        return;
    case ct::crc32("removeTobiiDataSender"):
        UDPinstance->removeTobiiDataSender();
        return;
#endif // HAS_SMI_INTEGRATION
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
        mxSetFieldByNumber(out, i, 0, mxCreateString(msg.text.get()));
        mxArray *temp;
        mxSetFieldByNumber(out, i, 1, temp = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL));
        *static_cast<int64_t*>(mxGetData(temp)) = msg.timeStamp;
#ifdef IP_ADDR_AS_STR
        mxSetFieldByNumber(out, i, 2, mxCreateString(msg.ip));
#else
        mxSetFieldByNumber(out, i, 2, temp = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL));
        *static_cast<double*>(mxGetData(temp)) = double(msg.ip);
#endif
        i++;
    }
    return out;
}


// function for handling errors generated by lib
void DoExitWithMsg(std::string errMsg_)
{
    mexErrMsgTxt(errMsg_.c_str());
}
