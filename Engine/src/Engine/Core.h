#pragma once

#ifdef ENGINE_EXPORT_WIN
#ifdef ENGINE_BUILD_DLL
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif
#else
#error OOP currently supports only windows 10 or higher
#endif