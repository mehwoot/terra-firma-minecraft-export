#pragma once

#include "Memory.h"
#include <filesystem>
#include <optional>
#include <functional>

namespace Util {
	namespace OS {
		// bool windowIsActive(ci::app::WindowRef window);
		bool debuggerPresent();
		void breakIfDebugging();
		std::string getGraphicsCardName();
		std::string getOpenGLVersion();
		
		// Correlate OpenGL GPU name with Windows GPU info for additional details
		struct WindowsGPUInfo {
			std::string name;
			std::string driverVersion;
			double memoryGB;
			bool found;
		};
		WindowsGPUInfo findWindowsGPUInfo(const std::string& openglRendererName);
		void safeCall(const std::function<void()>& code);
		void init();
		void errorMessage(const std::string& message);
		MemoryStats getMemoryStats();
		void openURL(const char* url);
		void openFile(const std::filesystem::path& filePath);
		std::filesystem::path getWriteableDirectory(const std::string& appName);
		int getCpuCores();
		std::string getMachineName();

#ifdef _MSC_VER
		namespace filesystem = std::filesystem;

		// template<typename T>
		// void main(cinder::app::RendererRef renderer, const std::string& title, void (*callback)(cinder::app::AppBase::Settings*)) {
		// 	cinder::app::AppMsw::main<T>(renderer, title.c_str(), callback);
		// }

		// typedef EXCEPTION_POINTERS ExceptionPointers;
#else
		namespace filesystem = std::filesystem;

		template<typename T>
		void main(cinder::app::RendererRef renderer, const std::string& title, void (*callback)(cinder::app::AppBase::Settings*)) {
			int argc = 0;
			char * argv[1];
			argv[0] = ( char*)(malloc(sizeof(char)));
			argv[0][0] = '\0';
			cinder::app::AppLinux::main<T>(renderer, title.c_str(), argc, argv, callback);
		}

		typedef void ExceptionPointers;
#endif		
	}

	// std::optional<std::filesystem::path> showWindowsFolderDialog(ci::app::WindowRef window);
	// std::optional<std::filesystem::path> showWindowsFileDialog(ci::app::WindowRef window);
}