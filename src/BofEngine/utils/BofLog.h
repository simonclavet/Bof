#pragma once

#include <memory>

#pragma warning(push)
#pragma warning(disable: 4459) // declaration of 'integral' hides global declaration
#pragma warning(disable: 4005) // 'APIENTRY': macro redefinition

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#pragma warning(pop)

namespace Bof
{
	class Log
	{
	public:
		static void Init()
		{
			std::vector<spdlog::sink_ptr> sinks;
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
			sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("BofEngine.log", true));

			spdlog::set_default_logger(std::make_shared<spdlog::logger>("Bof", begin(sinks), end(sinks)));
			spdlog::default_logger()->sinks()[0]->set_pattern("[%H:%M:%S] [%^%L%$] %v");
			spdlog::default_logger()->sinks()[1]->set_pattern("[%H:%M:%S] [%^%L%$] %v");

			//spdlog::default_logger()->sinks()[0]->set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");

		}

		//static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }
	private:
		//static std::shared_ptr<spdlog::logger> s_Logger;
	};

}
#define BOF_TRACE(...) spdlog::trace(__VA_ARGS__)
#define BOF_INFO(...) spdlog::info(__VA_ARGS__)
#define BOF_WARN(...) spdlog::warn(__VA_ARGS__)
#define BOF_ERROR(...) spdlog::error(__VA_ARGS__)


