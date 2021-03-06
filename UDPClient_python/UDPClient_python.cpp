#include "UDPMultiCast/UDPMultiCast.h"
#include "UDPMultiCast/utils.h"

#include <iostream>
#include <string>

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
using namespace boost::python;


struct theMsgConverter {
    void init() {
        auto collections = import("collections");
        auto namedtuple = collections.attr("namedtuple");
        list fields;
        fields.append("text");
        fields.append("timestamp");
        fields.append("ip");
        msgTuple = namedtuple("message", fields);
    }

    bool inited = false;
    api::object msgTuple;

    list getMessages(const std::vector<message>& msgs_) {
        if (!inited)
        {
            init();
            inited = true;
        }
        list result;
        for (auto& msg : msgs_)
            // boost.python doesn't do plain arrays, convert to string for an easy fix
#ifdef IP_ADDR_AS_STR
            result.append(msgTuple(std::string(msg.text.get()), msg.timeStamp, std::string(msg.ip)));
#else
            result.append(msgTuple(std::string(msg.text.get()), msg.timeStamp, msg.ip));
#endif
        return result;
    }
};
theMsgConverter convertMsgs;


list getData(UDPMultiCast& udp_) {
    return convertMsgs.getMessages(udp_.getData());
}
list getCommands(UDPMultiCast& udp_) {
    return convertMsgs.getMessages(udp_.getCommands());
}

void setComputerFilter(UDPMultiCast& udp_,list computerFilterList_)
{
    auto n = len(computerFilterList_);
    if (!n)
        udp_.setComputerFilter(nullptr, 0);
    else
    {
        auto arr = new double[n];
        for (boost::python::ssize_t i=0; i<n; i++)
        {
            extract<double> x(computerFilterList_[i]);	// check if contains double or something convertable to double
            if (x.check())
                arr[i] = x();
            else
                ErrorMsgExit("setComputerFilter: list contains python object that is not convertable to double");
        }
        udp_.setComputerFilter(arr, n);
        delete[] arr;
    }
}


#ifdef HAS_TOBII_INTEGRATION
void setTobiiScrSize(UDPMultiCast& udp_,list scrSize_)
{
    auto n = len(scrSize_);
    if (n!=2)
        ErrorMsgExit("setTobiiScrSize: argument must be a double array with two elements ([x,y] screen size)");
    else
    {
        std::vector<double> arr;
        for (boost::python::ssize_t i=0; i<n; i++)
        {
            extract<double> x(scrSize_[i]);	// check if contains double or something convertable to double
            if (x.check())
                arr.push_back(x());
            else
                ErrorMsgExit("setTobiiScrSize: argument must be a double array with two elements ([x,y] screen size). List contains python object that is not convertable to double");
        }
        udp_.setTobiiScrSize(arr);
    }
}
#endif

// tell boost.python about functions with optional arguments
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(sendWithTimeStamp_overloads, UDPMultiCast::sendWithTimeStamp, 1, 2);
// start module scope
BOOST_PYTHON_MODULE(UDPClient_python)
{
    class_<UDPMultiCast, boost::noncopyable>("UDPClient", init<>())
        .def("init", &UDPMultiCast::init)
        .def("deInit", &UDPMultiCast::deInit)
        .def("sendWithTimeStamp", &UDPMultiCast::sendWithTimeStamp, sendWithTimeStamp_overloads())
        .def("send", &UDPMultiCast::send)
        .def("checkReceiverThreads",&UDPMultiCast::checkReceiverThreads)

        // get the data and command messages received since the last call to this function
        .def("getData", getData)
        .def("getCommands", getCommands)

        // getters and setters
        .def("getGitRefID", &UDPMultiCast::getGitRefID)
        .def("setUseWTP", &UDPMultiCast::setUseWTP)
        .def("setMaxClockRes", &UDPMultiCast::setMaxClockRes)
        .add_property("loopBack", &UDPMultiCast::getLoopBack, &UDPMultiCast::setLoopBack)
        .add_property("reuseSocket", &UDPMultiCast::getReuseSocket, &UDPMultiCast::setReuseSocket)
        .add_property("groupAddress", &UDPMultiCast::getGroupAddress, &UDPMultiCast::setGroupAddress)
        .add_property("port", &UDPMultiCast::getPort, &UDPMultiCast::setPort)
        .add_property("bufferSize", &UDPMultiCast::getBufferSize, &UDPMultiCast::setBufferSize)
        .add_property("numQueuedReceives", &UDPMultiCast::getNumQueuedReceives, &UDPMultiCast::setNumQueuedReceives)
        .add_property("numReceiverThreads", &UDPMultiCast::getNumReceiverThreads, &UDPMultiCast::setNumReceiverThreads)
        .def("setComputerFilter", setComputerFilter)
        .def("hasSMIIntegration", &UDPMultiCast::hasSMIIntegration)
#ifdef HAS_SMI_INTEGRATION
        .def("startSMIDataSender", &UDPMultiCast::startSMIDataSender)
        .def("removeSMIDataSender", &UDPMultiCast::removeSMIDataSender)
#endif
        .def("hasTobiiIntegration", &UDPMultiCast::hasTobiiIntegration)
#ifdef HAS_TOBII_INTEGRATION
        .def("connectToTobii", &UDPMultiCast::connectToTobii)
        .def("setTobiiScrSize", setTobiiScrSize)
        .def("setTobiiSampleRate", &UDPMultiCast::setTobiiSampleRate)
        .def("startTobiiDataSender", &UDPMultiCast::startTobiiDataSender)
        .def("removeTobiiDataSender", &UDPMultiCast::removeTobiiDataSender)
#endif
        ;

    // free functions
    def("getCurrentTime", &timeUtils::getTimeStamp);
}

void DoExitWithMsg(std::string errMsg_)
{
    ::PyErr_SetString(::PyExc_TypeError, errMsg_.c_str());
    boost::python::throw_error_already_set();
}