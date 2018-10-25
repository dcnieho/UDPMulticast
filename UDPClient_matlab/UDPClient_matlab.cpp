#include "UDPMultiCast/UDPMultiCast.h"
#define DLL_EXPORT_SYM __declspec(dllexport) 
#include "mex.h"
#include "UDPMultiCast/utils.h"

#include <algorithm>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <atomic>

namespace {
    typedef UDPMultiCast class_type;
    typedef unsigned int handle_type;
    typedef std::pair<handle_type, std::shared_ptr<class_type>> indPtrPair_type;
    typedef std::map<indPtrPair_type::first_type, indPtrPair_type::second_type> instanceMap_type;
    typedef indPtrPair_type::second_type instPtr_t;

    // List actions
    enum class Action
    {
        Touch,
        New,
        Delete,

        Init,
        DeInit,
        SendWithTimeStamp,
        Send,
        CheckReceiverThreads,
        GetData,
        GetCommands,
        GetGitRefID,
        SetUseWTP,
        SetMaxClockRes,
        GetLoopBack,
        SetLoopBack,
        GetReuseSocket,
        SetReuseSocket,
        GetGroupAddress,
        SetGroupAddress,
        GetPort,
        SetPort,
        GetBufferSize,
        SetBufferSize,
        GetNumQueuedReceives,
        SetNumQueuedReceives,
        GetNumReceiverThreads,
        SetNumReceiverThreads,
        SetComputerFilter,
        GetCurrentTime,
        HasSMIIntegration,
    #ifdef HAS_SMI_INTEGRATION
        StartSMIDataSender,
        RemoveSMIDataSender,
    #endif
        HasTobiiIntegration,
    #ifdef HAS_TOBII_INTEGRATION
        ConnectToTobii,
        SetTobiiScrSize,
        SetTobiiSampleRate,
        StartTobiiDataSender,
        RemoveTobiiDataSender
    #endif
    };

    // Map string (first input argument to mexFunction) to an Action
    const std::map<std::string, Action> actionTypeMap =
    {
        { "touch",						Action::Touch },
        { "new",						Action::New },
        { "delete",						Action::Delete },

        { "startSampleBuffering",		Action::Init },
        { "deInit",		                Action::DeInit},
        { "sendWithTimeStamp",		    Action::SendWithTimeStamp},
        { "send",		                Action::Send},
        { "checkReceiverThreads",		Action::CheckReceiverThreads},
        { "getData",		            Action::GetData},
        { "getCommands",		        Action::GetCommands},
        { "getGitRefID",		        Action::GetGitRefID},
        { "setUseWTP",		            Action::SetUseWTP},
        { "setMaxClockRes",		        Action::SetMaxClockRes},
        { "getLoopBack",		        Action::GetLoopBack},
        { "setLoopBack",		        Action::SetLoopBack},
        { "getReuseSocket",		        Action::GetReuseSocket},
        { "setReuseSocket",		        Action::SetReuseSocket},
        { "getGroupAddress",		    Action::GetGroupAddress},
        { "setGroupAddress",		    Action::SetGroupAddress},
        { "getPort",		            Action::GetPort},
        { "setPort",		            Action::SetPort},
        { "getBufferSize",		        Action::GetBufferSize},
        { "setBufferSize",		        Action::SetBufferSize},
        { "getNumQueuedReceives",		Action::GetNumQueuedReceives},
        { "setNumQueuedReceives",		Action::SetNumQueuedReceives},
        { "getNumReceiverThreads",		Action::GetNumReceiverThreads},
        { "setNumReceiverThreads",		Action::SetNumReceiverThreads},
        { "setComputerFilter",		    Action::SetComputerFilter},
        { "getCurrentTime",		        Action::GetCurrentTime},
        { "hasSMIIntegration",		    Action::HasSMIIntegration},
    #ifdef HAS_SMI_INTEGRATION
        { "startSMIDataSender",		    Action::StartSMIDataSender},
        { "removeSMIDataSender",		Action::RemoveSMIDataSender},
    #endif
        { "hasTobiiIntegration",		Action::HasTobiiIntegration},
    #ifdef HAS_TOBII_INTEGRATION
        { "connectToTobii",		        Action::ConnectToTobii},
        { "setTobiiScrSize",		    Action::SetTobiiScrSize},
        { "setTobiiSampleRate",		    Action::SetTobiiSampleRate},
        { "startTobiiDataSender",		Action::StartTobiiDataSender},
        { "removeTobiiDataSender",		Action::RemoveTobiiDataSender},
    #endif
    };

    // table mapping handles to instances
    static instanceMap_type instanceTab;
    // for unique handles
    std::atomic<handle_type> handleVal = {0};

    // getHandle pulls the integer handle out of prhs[1]
    handle_type getHandle(int nrhs, const mxArray *prhs[])
    {
        if (nrhs < 2 || !mxIsScalar(prhs[1]))
            mexErrMsgTxt("Specify an instance with an integer handle.");
        return static_cast<handle_type>(mxGetScalar(prhs[1]));
    }

    // checkHandle gets the position in the instance table
    instanceMap_type::const_iterator checkHandle(const instanceMap_type& m, handle_type h)
    {
        auto it = m.find(h);
        if (it == m.end())
        {
            std::stringstream ss; ss << "No instance corresponding to handle " << h << " found.";
            mexErrMsgTxt(ss.str().c_str());
        }
        return it;
    }

    // forward declare
    mxArray* msgVectorToMatlab(std::vector<message> msgs_);
}


void DLL_EXPORT_SYM mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if (nrhs < 1 || !mxIsChar(prhs[0]))
        mexErrMsgTxt("First input must be an action string ('new', 'delete', or a method name).");

    // get action string
    char *actionCstr = mxArrayToString(prhs[0]);
    std::string actionStr(actionCstr);
    mxFree(actionCstr);

    // get corresponding action
    if (actionTypeMap.count(actionStr) == 0)
        mexErrMsgTxt(("Unrecognized action (not in actionTypeMap): " + actionStr).c_str());
    Action action = actionTypeMap.at(actionStr);

    // If action is not "new" or others that don't require a handle, try to locate an existing instance based on input handle
    instanceMap_type::const_iterator instIt;
    instPtr_t instance;
    if (action != Action::Touch && action != Action::New)
    {
        instIt = checkHandle(instanceTab, getHandle(nrhs, prhs));
        instance = instIt->second;
    }

    // execute action
    switch (action)
    {
    case Action::Init:
        // Check parameters
        if (nlhs < 0 || nrhs < 2)
            mexErrMsgTxt("init: Unexpected arguments.");
        // Call the method
        instance->init();
        return;
    case Action::DeInit:
        // Check parameters
        if (nlhs < 0 || nrhs < 2)
            mexErrMsgTxt("deInit: Unexpected arguments.");
        // Call the method
        instance->deInit();
        return;
    case Action::SendWithTimeStamp:
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
        int64_t timeStamp = instance->sendWithTimeStamp(str = mxArrayToString(prhs[2]),delim);
        mxFree(str);
        // return timestamp
        plhs[0] = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
        *static_cast<int64_t*>(mxGetData(plhs[0])) = timeStamp;
        return;
    }
    case Action::Send:
    {
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("send: Expected msg input.");
        if (!mxIsChar(prhs[2]))
            mexErrMsgTxt("send: Expected msg argument to be a string.");
        // Call the method
        char *str;
        instance->send(str = mxArrayToString(prhs[2]));
        mxFree(str);
        return;
    }
    case Action::CheckReceiverThreads:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("checkReceiverThreads: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(instance->checkReceiverThreads());
        return;
    case Action::GetData:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getData: Unexpected arguments.");
        // Call the method
        plhs[0] = msgVectorToMatlab(instance->getData());
        return;
    case Action::GetCommands:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getCommands: Unexpected arguments.");
        // Call the method
        plhs[0] = msgVectorToMatlab(instance->getCommands());
        return;

    case Action::GetGitRefID:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getGitRefID: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateString(instance->getGitRefID().c_str());
        return;
    case Action::SetUseWTP:
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
        instance->setUseWTP(useWTP);
        return;
    }
    case Action::SetMaxClockRes:
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
        instance->setMaxClockRes(setMaxClockRes);
        return;
    }
    case Action::GetLoopBack:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getLoopBack: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateLogicalScalar(!!instance->getLoopBack());
        return;
    case Action::SetLoopBack:
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
        instance->setLoopBack(loopback);
        return;
    }
    case Action::GetReuseSocket:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getReuseSocket: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateLogicalScalar(!!instance->getReuseSocket());
        return;
    case Action::SetReuseSocket:
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
        instance->setReuseSocket(reuseSocket);
        return;
    }
    case Action::GetGroupAddress:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getGroupAddress: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateString(instance->getGroupAddress().c_str());
        return;
    case Action::SetGroupAddress:
    {
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setGroupAddress: Expected groupAddress input.");
        if (!mxIsChar(prhs[2]))
            mexErrMsgTxt("setGroupAddress: Expected groupAddress argument to be a string.");
        // Call the method
        char *str;
        instance->setGroupAddress(str = mxArrayToString(prhs[2]));
        mxFree(str);
        return;
    }
    case Action::GetPort:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getPort: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(instance->getPort());
        return;
    case Action::SetPort:
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setPort: Expected port input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setPort: Expected argument to be a double scalar.");
        // Call the method
        instance->setPort(static_cast<unsigned short>(mxGetScalar(prhs[2])));
        return;
    case Action::GetBufferSize:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getBufferSize: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(instance->getBufferSize());
        return;
    case Action::SetBufferSize:
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setBufferSize: Expected buffer size input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setBufferSize: Expected argument to be a double scalar.");
        // Call the method
        instance->setBufferSize(static_cast<size_t>(mxGetScalar(prhs[2])));
        return;
    case Action::GetNumQueuedReceives:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getNumQueuedReceives: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(instance->getNumQueuedReceives());
        return;
    case Action::SetNumQueuedReceives:
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setNumQueuedReceives: Expected number of queued receives input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setNumQueuedReceives: Expected argument to be a double scalar.");
        // Call the method
        instance->setNumQueuedReceives(static_cast<unsigned long>(mxGetScalar(prhs[2])));
        return;
    case Action::GetNumReceiverThreads:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getNumReceiverThreads: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateDoubleScalar(instance->getNumReceiverThreads());
        return;
    case Action::SetNumReceiverThreads:
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setNumReceiverThreads: Expected number of receiver threads input.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || !mxIsScalar(prhs[2]))
            mexErrMsgTxt("setNumReceiverThreads: Expected argument to be a double scalar.");
        // Call the method
        instance->setNumReceiverThreads(static_cast<unsigned long>(mxGetScalar(prhs[2])));
        return;
    case Action::SetComputerFilter:
        // Check parameters
        if (nlhs < 0 || nrhs < 3)
            mexErrMsgTxt("setComputerFilter: Expected (possibly empty) array of computers from which you want to receive messages.");
        if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]))
            mexErrMsgTxt("setComputerFilter: Array of computers from which you want to receive messages should be double.");
        // Call the method
        instance->setComputerFilter(mxGetPr(prhs[2]), mxGetNumberOfElements(prhs[2]));
        return;
    case Action::GetCurrentTime:
        // Check parameters
        if (nlhs < 1 || nrhs < 2)
            mexErrMsgTxt("getCurrentTime: Unexpected arguments.");
        // Call the method
        plhs[0] = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
        *static_cast<int64_t*>(mxGetData(plhs[0])) = timeUtils::getTimeStamp();
        return;
    case Action::HasSMIIntegration:
        plhs[0] = mxCreateNumericMatrix(1, 1, mxLOGICAL_CLASS, mxREAL);
        *static_cast<bool*>(mxGetData(plhs[0])) = instance->hasSMIIntegration();
        return;
#ifdef HAS_SMI_INTEGRATION
    case Action::StartSMIDataSender:
        instance->startSMIDataSender();
        return;
    case Action::RemoveSMIDataSender:
        instance->removeSMIDataSender();
        return;
#endif
    case Action::HasTobiiIntegration:
        plhs[0] = mxCreateNumericMatrix(1, 1, mxLOGICAL_CLASS, mxREAL);
        *static_cast<bool*>(mxGetData(plhs[0])) = instance->hasTobiiIntegration();
        return;
#ifdef HAS_TOBII_INTEGRATION
    case Action::ConnectToTobii:
    {
        if (nrhs < 2 || !mxIsChar(prhs[2]))
            mexErrMsgTxt("connectToTobii: Third argument must be a string.");

        char* address = mxArrayToString(prhs[2]);
        instance->connectToTobii(address);
        mxFree(address);
        return;
    }
    case Action::SetTobiiScrSize:
    {
        if (nrhs < 2 || !mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || mxGetNumberOfElements(prhs[2]) != 2)
            mexErrMsgTxt("setTobiiScrSize: Third argument must be a double array with two elements ([x,y] screen size).");

        instance->setTobiiScrSize({*mxGetPr(prhs[2]),*(mxGetPr(prhs[2]) + 1)});
        return;
    }
    case Action::SetTobiiSampleRate:
    {
        if (nrhs < 2 || !mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]))
            mexErrMsgTxt("setTobiiSampleRate: Third argument must be a double");

        instance->setTobiiSampleRate(static_cast<float>(*mxGetPr(prhs[2])));
        return;
    }
    case Action::StartTobiiDataSender:
        instance->startTobiiDataSender();
        return;
    case Action::RemoveTobiiDataSender:
        instance->removeTobiiDataSender();
        return;
#endif
    default:
        mexErrMsgTxt(("Unhandled action: " + actionStr).c_str());
        break;
    }
}

// helpers
namespace
{
    mxArray* msgVectorToMatlab(std::vector<message> msgs_)
    {
        const char* fieldNames[] = {"text", "timeStamp", "ip"};
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
}


// function for handling errors generated by lib
void DoExitWithMsg(std::string errMsg_)
{
    mexErrMsgTxt(errMsg_.c_str());
}
