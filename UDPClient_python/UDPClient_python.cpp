#include "../UDPMultiCast.h"

#include <iostream>
#include <string>

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
using namespace boost::python;


BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(send_overloads, UDPMultiCast::sendWithTimeStamp, 1, 2);
BOOST_PYTHON_MODULE(UDPClient_python)
{
	// need to expose our message struct (NB: boost.python doesn't do plain arrays, convert to string for an easy fix)
	class_<message>("message")
		.add_property("msg", static_cast<std::string(*)(const message&)>([](const message& m) {return std::string(m.text); }))
		.add_property("timeStamp", &message::timeStamp);

	// and a vector of messages
	class_<std::vector<message>>("messageList")
		.def(vector_indexing_suite<std::vector<message>>());

	class_<UDPMultiCast, boost::noncopyable>("UDPClient", init<>())
		.def("init", &UDPMultiCast::init)
		.def("deInit", &UDPMultiCast::deInit)
		.def("sendWithTimeStamp", &UDPMultiCast::sendWithTimeStamp, send_overloads())
		.def("send", &UDPMultiCast::send)
		.add_property("loopBack", &UDPMultiCast::getLoopBack, &UDPMultiCast::setLoopBack)
		.add_property("groupAddress", &UDPMultiCast::getGroupAddress, &UDPMultiCast::setGroupAddress)
		.add_property("port", &UDPMultiCast::getPort, &UDPMultiCast::setPort)
		.def("getData", &UDPMultiCast::getData)
		.def("getCommands", &UDPMultiCast::getCommands)
		;
}

void DoExitWithMsg(std::string errMsg_)
{
	::PyErr_SetString(::PyExc_TypeError, errMsg_.c_str());
	boost::python::throw_error_already_set();
}