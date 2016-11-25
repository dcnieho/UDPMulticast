#include "../UDPMultiCast.h"
#include "../utils.h"

#include <iostream>
#include <string>

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
using namespace boost::python;

// TODO: add pure python functions as per
// http://www.boost.org/doc/libs/1_62_0/libs/python/doc/html/tutorial/tutorial/techniques.html
// e.g., for debug, getting single commands, etc

struct theMsgConverter {
	theMsgConverter() {
		auto collections = import("collections");
		auto namedtuple = collections.attr("namedtuple");
		list fields;
		fields.append("text");
		fields.append("timestamp");
		fields.append("ip");
		msgTuple = namedtuple("message", fields);
	}

	api::object msgTuple;

	list getMessages(const std::vector<message>& msgs_) {
		list result;
		for (auto& msg : msgs_)
#ifdef IP_ADDR_AS_STR
			// boost.python doesn't do plain arrays, convert to string for an easy fix
			result.append(msgTuple(msg.text.get(), msg.timeStamp, std::string(msg.ip)));
#else
			result.append(msgTuple(msg.text.get(), msg.timeStamp, msg.ip));
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

// tell python that UDPMultiCast::sendWithTimeStamp last argument is optional
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(send_overloads, UDPMultiCast::sendWithTimeStamp, 1, 2);
// start module scope
BOOST_PYTHON_MODULE(UDPClient_python)
{
	class_<UDPMultiCast, boost::noncopyable>("UDPClient", init<>())
        .def("init", &UDPMultiCast::init)
        .def("deInit", &UDPMultiCast::deInit)
        .def("sendWithTimeStamp", &UDPMultiCast::sendWithTimeStamp, send_overloads())
        .def("send", &UDPMultiCast::send)
        .def("checkReceiverThreads",&UDPMultiCast::checkReceiverThreads)

        // get the data and command messages received since the last call to this function
        .def("getData", getData)
        .def("getCommands", getCommands)

		// getters and setters
		.add_property("loopBack", &UDPMultiCast::getLoopBack, &UDPMultiCast::setLoopBack)
		.add_property("reuseSocket", &UDPMultiCast::getReuseSocket, &UDPMultiCast::setReuseSocket)
        .def("setUseWTP", &UDPMultiCast::setUseWTP)
        .def("setMaxClockRes", &UDPMultiCast::setMaxClockRes)
        .add_property("groupAddress", &UDPMultiCast::getGroupAddress, &UDPMultiCast::setGroupAddress)
        .add_property("port", &UDPMultiCast::getPort, &UDPMultiCast::setPort)
        .add_property("bufferSize", &UDPMultiCast::getBufferSize, &UDPMultiCast::setBufferSize)
        .add_property("numQueuedReceives", &UDPMultiCast::getNumQueuedReceives, &UDPMultiCast::setNumQueuedReceives)
		.add_property("numReceiverThreads", &UDPMultiCast::getNumReceiverThreads, &UDPMultiCast::setNumReceiverThreads)
		.def("setComputerFilter", &UDPMultiCast::setComputerFilter)
#ifdef HAS_SMI_INTEGRATION
		.def("hasSMIIntegration", &UDPMultiCast::hasSMIIntegration)
		.def("startSMIDataSender", &UDPMultiCast::startSMIDataSender)
		.def("removeSMIDataSender", &UDPMultiCast::removeSMIDataSender)
#else // HAS_SMI_INTEGRATION
		.def("hasSMIIntegration", &UDPMultiCast::hasSMIIntegration)
#endif // HAS_SMI_INTEGRATION
        ;

	// free functions
	def("getCurrentTime", &timeUtils::getTimeStamp);
}

void DoExitWithMsg(std::string errMsg_)
{
    ::PyErr_SetString(::PyExc_TypeError, errMsg_.c_str());
    boost::python::throw_error_already_set();
}