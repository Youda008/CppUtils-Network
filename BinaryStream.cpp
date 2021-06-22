//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: classes for binary serialization into binary buffers via operators << and >>
//======================================================================================================================

#include "BinaryStream.hpp"

#include <string>
using std::string;
#include <algorithm>  // find, copy
using std::find;
using std::copy;
#include <cstring>  // memcpy


namespace own {


//----------------------------------------------------------------------------------------------------------------------
//  strings and arrays

void BinaryOutputStream::writeBytes( const_byte_span buffer )
{
	memcpy( _curPos, buffer.data(), buffer.size() );
	_curPos += buffer.size();
}

void BinaryOutputStream::writeString0( const string & str )
{
	memcpy( _curPos, str.data(), str.size() + 1 );
	_curPos += str.size() + 1;
}

bool BinaryInputStream::readBytes( byte_span buffer )
{
	if (!canRead( buffer.size() )) {
		return false;
	}

	memcpy( buffer.data(), _curPos, buffer.size() );
	_curPos += buffer.size();
	return true;
}

bool BinaryInputStream::readString( string & str, size_t size )
{
	if (!canRead( size )) {
		return false;
	}

	str.resize( size );
	memcpy( const_cast< char * >( str.data() ), _curPos, size );
	_curPos += size;
	return true;
}

bool BinaryInputStream::readString0( string & str )
{
	if (!_failed)
	{
		const uint8_t * strEndPos = find( _curPos, _endPos, '\0' );
		if (strEndPos >= _endPos) {
			_failed = true;
		} else {
			const size_t size = size_t( strEndPos - _curPos );
			str.resize( size );
			memcpy( const_cast< char * >( str.data() ), _curPos, size );
			_curPos += str.size() + 1;
		}
	}
	return !_failed;
}

void BinaryOutputStream::writeZeros( size_t numZeroBytes )
{
	memset( _curPos, 0, numZeroBytes );
	_curPos += numZeroBytes;
}

bool BinaryInputStream::readRemaining( std::vector< uint8_t > & buffer )
{
	const size_t size = size_t( _endPos - _curPos );
	buffer.resize( size );
	memcpy( &*buffer.begin(), _curPos, size );
	_curPos += size;
	return true;
}

bool BinaryInputStream::readRemaining( std::string & str )
{
	const size_t size = size_t( _endPos - _curPos );
	str.resize( size, '0' );
	memcpy( const_cast< char * >( str.data() ), _curPos, size );
	_curPos += size;
	return true;
}

bool BinaryInputStream::skip( size_t numBytes )
{
	if (!canRead( numBytes )) {
		return false;
	}

	_curPos += numBytes;
	return true;
}


//----------------------------------------------------------------------------------------------------------------------


} // namespace own
