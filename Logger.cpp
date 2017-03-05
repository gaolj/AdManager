#include "Logger.h"
#include <fstream>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>

#include <boost/log/support/date_time.hpp>
#include <boost/log/support/exception.hpp>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/from_stream.hpp>			// init_from_stream
#include <boost/log/utility/setup/filter_parser.hpp>		// register_simple_filter_factory

BOOST_LOG_ATTRIBUTE_KEYWORD(_severity, "Severity", SeverityLevel)
BOOST_LOG_ATTRIBUTE_KEYWORD(_timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(_uptime, "Uptime", attrs::timer::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(_scope, "Scope", attrs::named_scope::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(_remoteAddress, "RemoteAddress", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(_errorCode, "ErrorCode", int32_t)

logging::formatting_ostream& operator <<
(
	logging::formatting_ostream& strm,
	logging::to_log_manip< SeverityLevel, tag::_severity > const& manip
)
{
	static const char* strings[] =
	{
		"TRACE",
		"DEBUG",
		"NTFY ",
		"INFO ",
		"WARN ",
		"ERROR",
		"FATAL"
	};

	SeverityLevel level = manip.get();
	if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
		strm << strings[level];
	else
		strm << static_cast< int >(level);

	return strm;
}

void initLogger()
{
#ifdef _DEBUG
	auto mode = std::ios::trunc;
#else
	auto mode = std::ios::app;
#endif
	//logging::register_simple_formatter_factory<SeverityLevel, char>("Severity");
	//logging::register_simple_filter_factory<SeverityLevel, char>("Severity");
	//std::ifstream settings("log.ini");
	//if (!settings.is_open())
	//	std::cout << "Could not open log.ini file" << std::endl;
	//else
	//	logging::init_from_stream(settings);

	logging::add_file_log(
		keywords::file_name = "log/advert1_%3N.log",
		keywords::open_mode = mode,
		keywords::auto_flush = true,
		keywords::rotation_size = 10 * 1024 * 1024,
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		//keywords::filter = _severity == trace,
		keywords::format = expr::stream
		<< expr::format_date_time(_timestamp, "%Y-%m-%d %H:%M:%S.%f")
		<< "	" << expr::attr< attrs::current_thread_id::value_type >("ThreadID")
		<< "	" << _severity
		<< "	" << expr::message
		<< "			" << expr::format_named_scope(_scope, keywords::format = "%c", keywords::iteration = expr::reverse, keywords::depth = 3));
	logging::add_file_log(
		keywords::file_name = "log/advert2_%3N.log",
		keywords::open_mode = mode,
		keywords::auto_flush = true,
		keywords::rotation_size = 10 * 1024 * 1024,
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		keywords::filter = _severity.or_default(debug) >= debug,
		keywords::format = expr::stream
		<< expr::format_date_time(_timestamp, "%Y-%m-%d %H:%M:%S.%f")
		<< "	" << expr::attr< attrs::current_thread_id::value_type >("ThreadID")
		<< "	" << _severity
		<< expr::if_(expr::has_attr(_remoteAddress))
			[
				expr::stream << "	" << _remoteAddress
			]
		<< expr::if_(expr::has_attr(_uptime))
			[
				expr::stream << "	" << _uptime
			]
		<< "	" << expr::message
		<< expr::if_(expr::has_attr(_errorCode))
			[
				expr::stream << "	(" << _errorCode << ")"
			]
		<< expr::if_(_severity >= notify)
			[
				expr::stream << "			" << expr::format_named_scope(_scope, keywords::format = "[%c@%F:%l]", keywords::iteration = expr::reverse, keywords::depth = 3)
			]);

	logging::add_common_attributes();
	logging::core::get()->add_global_attribute("Scope", attrs::named_scope());
	logging::core::get()->set_filter(_severity >= debug);
}