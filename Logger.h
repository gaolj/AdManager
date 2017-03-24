#pragma once
//#define BOOST_USE_WINAPI_VERSION BOOST_WINAPI_VERSION_WIN7
//#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>				// BOOST_LOG_SEV
#include <boost/log/sources/severity_logger.hpp>			// severity_logger
#include <boost/log/sources/severity_channel_logger.hpp>	// severity_channel_logger
#include <boost/log/sources/global_logger_storage.hpp>

//#include <boost/log/attributes.hpp>						
#include <boost/log/attributes/named_scope.hpp>				// BOOST_LOG_NAMED_SCOPE
#include <boost/log/attributes/timer.hpp>					// attrs::timer
#include <boost/log/attributes/constant.hpp>				// attrs::constant
#include <boost/log/attributes/scoped_attribute.hpp>		// BOOST_LOG_SCOPED_THREAD_ATTR

#include <boost/log/utility/manipulators/add_value.hpp>

//#include <boost/log/support/exception.hpp>					// logging::current_scope

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

enum SeverityLevel
{
	trace,
	debug,
	notify,
	info,
	warn,
	error,
	fatal
};

#define LOG_TRACE(logger)	BOOST_LOG_SEV(logger, trace)
#define LOG_DEBUG(logger)	BOOST_LOG_SEV(logger, debug)
#define LOG_NTFY(logger)	BOOST_LOG_SEV(logger, notify)
#define LOG_INFO(logger)	BOOST_LOG_SEV(logger, info)
#define LOG_WARN(logger)	BOOST_LOG_SEV(logger, warn)
#define LOG_ERROR(logger)	BOOST_LOG_SEV(logger, error)
#define LOG_FATAL(logger)	BOOST_LOG_SEV(logger, fatal)

void initLogger(SeverityLevel lvl);
BOOST_LOG_GLOBAL_LOGGER(tracer_logger, src::severity_logger_mt<SeverityLevel>)
BOOST_LOG_GLOBAL_LOGGER(player_logger, src::severity_channel_logger_mt<SeverityLevel>)

class FuncTracer
{
public:
	std::string _msg;
	FuncTracer(const std::string& msg) : _msg(msg)
	{
		LOG_TRACE(tracer_logger::get()) << "BGN	" << _msg;
	}

	~FuncTracer(void)
	{
		LOG_TRACE(tracer_logger::get()) << "END	" << _msg;
	}
};

#ifdef _DEBUG
#define FUNC_TRACER(msg)	FuncTracer tracer(msg)
#else
// #define FUNC_TRACER(msg)	// 性能原因
#define FUNC_TRACER(msg)	FuncTracer tracer(msg)
#endif

