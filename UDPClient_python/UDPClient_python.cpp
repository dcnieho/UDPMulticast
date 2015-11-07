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
	// alternative for getData etc would be to use:
	// http://stackoverflow.com/questions/13675409/creating-python-collections-namedtuple-from-c-using-boostpython
	// maybe we can do a bit better when we learn from <boost/python/tuple.hpp>
	// may need this:
	// http://stackoverflow.com/questions/2135457/how-to-write-a-wrapper-over-functions-and-member-functions-that-executes-some-co
	// make a second getdata function (getDataNTL for named tuple list). Test which one is faster so we have a recommended function,
	// but keep both. preferred is simply called getData, non-preferred getDataNTL or getDataSL.

	// need to expose our message struct (NB: boost.python doesn't do plain arrays, convert to string for an easy fix)
	class_<message>("message")
		.add_property("text", static_cast<std::string(*)(const message&)>([](const message& m) {return std::string(m.text); }))	// casting to function pointer the ugly way as unary plus does not seems to work with VS2015
		.add_property("timeStamp", &message::timeStamp);

	// and a vector of messages
	class_<std::vector<message>>("messageList")
		.def(vector_indexing_suite<std::vector<message>>());

	class_<UDPMultiCast, boost::noncopyable>("UDPClient", init<>())
		.def("init", &UDPMultiCast::init)
		.def("deInit", &UDPMultiCast::deInit)
		.def("sendWithTimeStamp", &UDPMultiCast::sendWithTimeStamp, send_overloads())
		.def("send", &UDPMultiCast::send)
		.def("getData", &UDPMultiCast::getData)
		.def("getCommands", &UDPMultiCast::getCommands)
		.add_property("loopBack", &UDPMultiCast::getLoopBack, &UDPMultiCast::setLoopBack)
		.add_property("groupAddress", &UDPMultiCast::getGroupAddress, &UDPMultiCast::setGroupAddress)
		.add_property("port", &UDPMultiCast::getPort, &UDPMultiCast::setPort)
		.add_property("bufferSize", &UDPMultiCast::getBufferSize, &UDPMultiCast::setBufferSize)
		.add_property("numQueuedReceives", &UDPMultiCast::getNumQueuedReceives, &UDPMultiCast::setNumQueuedReceives)
		.add_property("numReceiverThreads", &UDPMultiCast::getNumReceiverThreads, &UDPMultiCast::setNumReceiverThreads)
		;
}

void DoExitWithMsg(std::string errMsg_)
{
	::PyErr_SetString(::PyExc_TypeError, errMsg_.c_str());
	boost::python::throw_error_already_set();
}