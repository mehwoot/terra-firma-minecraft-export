
#include "OS.h"
#include "mc-export-plugin-module.h"

#include <iomanip>
#include <thread>

#ifdef _MSC_VER

#include <tchar.h>
#include <wchar.h>
#include <stdlib.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <Psapi.h>
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
// #include <sentry.h>
// #include "Util/Sentry.h"

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

void Util::OS::init() {
    timeBeginPeriod(1);
}

// bool Util::OS::windowIsActive(ci::app::WindowRef window) {
// 	return (HWND)window->getNative() == GetFocus();
// }

bool Util::OS::debuggerPresent() {
	static bool value = IsDebuggerPresent();
    return value;
}

void Util::OS::breakIfDebugging() {
	if (debuggerPresent()) {
		__debugbreak();
	}
}

// int handleException(unsigned int code, struct _EXCEPTION_POINTERS* ep) {
    
//     // Try to extract C++ exception information from SEH parameters
//     const char* exceptionType = "Unknown";
//     const char* exceptionMessage = "Unknown";
// 	bool isCppException = false;
    
//     // Check if this is a C++ exception
//     if (code == 0xE06D7363) { // Microsoft C++ Exception code
//         if (ep && ep->ExceptionRecord && ep->ExceptionRecord->NumberParameters >= 3) {
//             try {
//                 // The third parameter often contains a pointer to the exception object
//                 void* exceptionObject = reinterpret_cast<void*>(ep->ExceptionRecord->ExceptionInformation[1]);
                
//                 if (exceptionObject) {
//                     // Try to cast to std::exception and extract information
//                     // This is highly implementation-dependent and may not work reliably
//                     std::exception* stdException = static_cast<std::exception*>(exceptionObject);
//                     exceptionMessage = stdException->what();
//                     exceptionType = typeid(*stdException).name();

//                     // The point of the alternate path is to get the exception type and message and combine
//                     // it with the stack trace.  If we can't get the type and message, i.e. anything up to
//                     // this point failed, using the normal path will still work, it'll just send the stack trace.
// 					isCppException = true;
//                 }
//             } catch (...) {}
//         }
//     }

// 	if (isCppException) {
// 		Util::Sentry::get().addBreadcrumb("error_type", exceptionType, "error");
// 		Util::Sentry::get().addBreadcrumb("error_message", exceptionMessage, "error");

// 		sentry_value_t event = sentry_value_new_event();
// 		sentry_value_t exception = sentry_value_new_exception(exceptionType, exceptionMessage);
// 		sentry_event_add_exception(event, exception);
// 		sentry_value_set_by_key(event, "level", sentry_value_new_string("fatal"));
// 		sentry_value_t mechanism = sentry_value_new_object();
// 		sentry_value_set_by_key(mechanism, "type", sentry_value_new_string("generic"));
// 		sentry_value_set_by_key(mechanism, "handled", sentry_value_new_bool(0));
// 		sentry_value_set_by_key(exception, "mechanism", mechanism);

// 		// Add the original stack trace from SEH context
// 		sentry_ucontext_t sentry_context;
// 		sentry_context.exception_ptrs = *ep;

// 		// We need to use the lower-level API to combine the exception info with SEH stack trace
// 		// Unfortunately, sentry_handle_exception doesn't let us customize the exception details
// 		// So let's create a custom event and add stack trace manually
// 		void* stack[128];
// 		size_t frame_count = sentry_unwind_stack_from_ucontext(&sentry_context, stack, 128);
// 		if (frame_count > 0) {
// 			sentry_value_set_stacktrace(exception, stack, frame_count);
// 		}

// 		sentry_capture_event(event);
// 	} else {
// 		sentry_ucontext_t sentry_context;
// 		sentry_context.exception_ptrs = *ep;
// 		Util::Sentry::get().sendError(&sentry_context);
// 	}

//     return EXCEPTION_EXECUTE_HANDLER;
// }

// void cplusplusCatch(const std::function<void()>& code) {
//     if (Util::OS::debuggerPresent()) {
//         return code();
//     } else {
//         try {
//             return code();
//         } catch (std::exception& e) {
//             std::cout << e.what() << std::endl;
//             Util::Sentry::get().addBreadcrumb("error", e.what(), "error");
            
//             throw;
//         }
//     }
// }

// void cSEHCatch(const std::function<void()>& code) {
//     if (Util::OS::debuggerPresent()) {
//         //return cplusplusCatch(code);
//         return code();
//     } else {
//         __try {
// 			/* The problem with cplusplusCatch is that the default sentry handling for C++ exceptions doesn't include the actual exception type
// 				or the message from e.what() in the exception.  However, if we catch and then rethrow the exception, then the stack trace
// 				becomes the stack trace from the rethrow location, not the original location.
// 				To solve this, we use Structured exception handling for everything, detect when it was actually a C++ exception, and do some
// 				nasty technically non-portable business pulling the original C++ exception and associated information out from the SEH exception */
//             // return cplusplusCatch(code);
//             return code();
//         } __except (handleException(GetExceptionCode(), GetExceptionInformation())) {
//             exit(1);
//         }
//     }
// }

// void Util::OS::safeCall(const std::function<void()>& code) {
//     //code();
//     cSEHCatch(code);
// }

std::string convert(wchar_t* wideString) {
    std::ostringstream ret;
    std::unique_ptr<char[]> converted = std::make_unique<char[]>(MB_CUR_MAX + 1);
    size_t size = MB_CUR_MAX;
    int length;

    while (*wideString) {
        length = wctomb(converted.get(), *wideString);
        if (length < 1) break;
        converted[length] = '\0';
        ret << converted;
        ++wideString;
    }

    return ret.str();
}

// std::string Util::OS::getOpenGLVersion() {
//     return std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
// }

std::string Util::OS::getGraphicsCardName() {
    static std::string result = "";
	if (result != "") return result;

    HRESULT hres;
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return "";

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) {
        CoUninitialize();
        return "";
    }
    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return "";
    }
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"root\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return "";
    }
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "";
}
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "";
    }

    std::ostringstream ret;

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) break;
        
        VARIANT vtProp;
        // Get graphics card name
        hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
            ret << convert(vtProp.bstrVal);
        }
        VariantClear(&vtProp);
        
        // Get video memory (AdapterRAM is in bytes)
        hr = pclsObj->Get(L"AdapterRAM", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I4) {
            DWORD memoryBytes = vtProp.ulVal;
            double memoryGB = memoryBytes / (1024.0 * 1024.0 * 1024.0);
            ret << " (" << std::fixed << std::setprecision(1) << memoryGB << " GB)";
        }
        VariantClear(&vtProp);
        
        ret << "; ";
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    if (pclsObj != NULL) pclsObj->Release();
    CoUninitialize();

    result = ret.str();
    return result;
}

Util::OS::WindowsGPUInfo Util::OS::findWindowsGPUInfo(const std::string& openglRendererName) {
    WindowsGPUInfo result = { "", "", 0.0, false };
    
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return result;

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) {
        CoUninitialize();
        return result;
    }
    
    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return result;
    }
    
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"root\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return result;
    }
    
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return result;
    }
    
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return result;
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) break;
        
        VARIANT vtProp;
        
        // Get graphics card name
        hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
            std::string gpuName = convert(vtProp.bstrVal);
            
            // Check if this GPU name matches or is contained in the OpenGL renderer string
            if (openglRendererName.find(gpuName) != std::string::npos || 
                gpuName.find(openglRendererName) != std::string::npos) {
                
                result.name = gpuName;
                result.found = true;
                
                VariantClear(&vtProp);
                
                // Get driver version
                hr = pclsObj->Get(L"DriverVersion", 0, &vtProp, 0, 0);
                if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
                    result.driverVersion = convert(vtProp.bstrVal);
                }
                VariantClear(&vtProp);
                
                // Get video memory
                hr = pclsObj->Get(L"AdapterRAM", 0, &vtProp, 0, 0);
                if (SUCCEEDED(hr) && vtProp.vt == VT_I4) {
                    DWORD memoryBytes = vtProp.ulVal;
                    result.memoryGB = memoryBytes / (1024.0 * 1024.0 * 1024.0);
                }
                VariantClear(&vtProp);
                
                break; // Found our GPU
            }
        }
        VariantClear(&vtProp);
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    if (pclsObj != NULL) pclsObj->Release();
    CoUninitialize();

    return result;
}

void Util::OS::errorMessage(const std::string& message) {
	MessageBoxExA(NULL, message.c_str(), "Error", MB_SYSTEMMODAL | MB_ICONERROR, 0);
	//MessageBoxExA(GetDesktopWindow(), message.c_str(), "Error", MB_OK | MB_ICONERROR | MB_TASKMODAL, 0);
}

// Util::MemoryStats Util::OS::getMemoryStats() {
//     PROCESS_MEMORY_COUNTERS processMemoryInfo;
//     MEMORYSTATUSEX globalMemoryInfo;
//     globalMemoryInfo.dwLength = sizeof(MEMORYSTATUSEX);
//     const double gb = (1024.0 * 1024.0 * 1024.0);

//     bool success = GetProcessMemoryInfo(GetCurrentProcess(), &processMemoryInfo, sizeof(processMemoryInfo));
//     if (!success) {
//         auto error = GetLastError();
//         soft_assert(false, "error");
//     }
    
//     success = GlobalMemoryStatusEx(&globalMemoryInfo);
//     if (!success) {
//         auto error = GetLastError();
//         soft_assert(false, "error");
//     }

//     if (success) {
//         return {
//             globalMemoryInfo.ullTotalPhys / gb,
//             0.0,
//             processMemoryInfo.WorkingSetSize / gb
//         };
//     } else {
//         return {
//             0.0, 0.0, 0.0
//         };
//     }
// }

void Util::OS::openURL(const char* url) {
	ShellExecuteA(0, 0, url, 0, 0, SW_SHOW);
}

void Util::OS::openFile(const std::filesystem::path& filePath) {
	std::string pathStr = filePath.string();
	ShellExecuteA(0, "open", pathStr.c_str(), 0, 0, SW_SHOW);
}

// std::optional<std::filesystem::path> showWindowsDialog(ci::app::WindowRef window, int flags) {
// 	std::optional<std::filesystem::path> result;
// 	HWND hwndOwner = window->getNative() ? static_cast<HWND>(window->getNative()) : nullptr;
// 	if (hwndOwner && (window->isFullScreen() || window->isBorderless())) {
// 		ShowWindow(hwndOwner, SW_MINIMIZE);
// 	}
// 	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
// 	bool comInitialized = SUCCEEDED(hr);

// 	IFileDialog* pFileDialog = nullptr;
// 	hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));
// 	if (SUCCEEDED(hr)) {
// 		DWORD options;
// 		pFileDialog->GetOptions(&options);
// 		pFileDialog->SetOptions(options | flags);

// 		hr = pFileDialog->Show(hwndOwner);
// 		if (SUCCEEDED(hr)) {
// 			IShellItem* pItem = nullptr;
// 			hr = pFileDialog->GetResult(&pItem);
// 			if (SUCCEEDED(hr)) {
// 				PWSTR pszFilePath = nullptr;
// 				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
// 				if (SUCCEEDED(hr) && pszFilePath) {
// 					result = std::filesystem::path(pszFilePath);
// 					CoTaskMemFree(pszFilePath);
// 				}
// 				pItem->Release();
// 			}
// 		}
// 		pFileDialog->Release();
// 	}
// 	if (comInitialized) {
// 		CoUninitialize();
// 	}
// 	if (hwndOwner) {
// 		ShowWindow(hwndOwner, SW_RESTORE);
// 	}
// 	return result;
// }

// std::optional<std::filesystem::path> Util::showWindowsFolderDialog(ci::app::WindowRef window) {
// 	return showWindowsDialog(window, FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
// }

// std::optional<std::filesystem::path> Util::showWindowsFileDialog(ci::app::WindowRef window) {
// 	return showWindowsDialog(window, FOS_FORCEFILESYSTEM);
// }

std::filesystem::path Util::OS::getWriteableDirectory(const std::string& appName) {
	PWSTR localAppDataPath = nullptr;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppDataPath);
	
	if (SUCCEEDED(hr) && localAppDataPath) {
		std::filesystem::path appDataPath(localAppDataPath);
		CoTaskMemFree(localAppDataPath);
		
		std::filesystem::path writeableDir = appDataPath / appName;
		
		// Create the directory if it doesn't exist
		std::error_code ec;
		std::filesystem::create_directories(writeableDir, ec);
		
		return writeableDir;
	}
	
    reportFatalErrorC("could not get writeable directory");

	// Fallback to current directory if we can't get AppData
	return std::filesystem::current_path();
}

int Util::OS::getCpuCores() {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return static_cast<int>(sysInfo.dwNumberOfProcessors);
}

std::string Util::OS::getMachineName() {
	char buffer[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(buffer);
	if (GetComputerNameA(buffer, &size)) {
		return std::string(buffer, size);
	}
	return "";
}

#else

bool Util::OS::windowIsActive(ci::app::WindowRef window) {
	return true;
	// fatal_assert(false, "OS function not implemented");
}

bool Util::OS::debuggerPresent() {
	return false;
}

int Util::OS::getCpuCores() {
	return static_cast<int>(std::thread::hardware_concurrency());
}

void Util::OS::openFile(const std::filesystem::path& filePath) {
	std::string pathStr = filePath.string();
	std::string command = "xdg-open \"" + pathStr + "\"";
	system(command.c_str());
}

#endif
