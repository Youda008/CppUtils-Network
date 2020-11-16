//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  1.11.2020
// Description: universal way of getting information about system errors
//======================================================================================================================

#ifndef CPPUTILS_SYSTEM_ERROR_INFO_INCLUDED
#define CPPUTILS_SYSTEM_ERROR_INFO_INCLUDED


#include <cstdint>
#include <string>


#ifdef _WIN32
	using system_error_t = uint32_t;  // should be DWORD but let's not include the whole windows.h just because of this
#else
	using system_error_t = int;
#endif


namespace own {


/** OS-independent way to retrieve the error code of the last system call */
system_error_t getLastError();

/** OS-independent way to convert an error code to a readable error message */
std::string getErrorString( system_error_t errorCode );


} // namespace own


#endif // CPPUTILS_SYSTEM_ERROR_INFO_INCLUDED
