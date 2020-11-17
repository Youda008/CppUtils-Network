//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  2.11.2020
// Description: classes for binary serialization into binary buffers via operators << and >>
//======================================================================================================================

#include "BufferStream.hpp"

#include <string>
using std::string;
#include <algorithm>  // find, copy
using std::find;
using std::copy;


namespace own {


//----------------------------------------------------------------------------------------------------------------------
//  string

void BufferOutputStream::writeString( const string & str )
{
	//_buffer.resize( _buffer.size() + str.size() );

	copy( str.begin(), str.end(), _curPos );
	_curPos += str.size();
}

void BufferOutputStream::writeString0( const string & str )
{
	//_buffer.resize( _buffer.size() + str.size() + 1 );

	copy( str.begin(), str.end() + 1, _curPos );
	_curPos += str.size() + 1;
}

bool BufferInputStream::readString( string & str, size_t size )
{
	if (canRead( size ))
	{
		str.resize( size, '0' );
		copy( _curPos, _curPos + size, str.begin() );
		_curPos += size;
	}
	return !_failed;
}

bool BufferInputStream::readString0( string & str )
{
	if (!_failed)
	{
		const uint8_t * strEndPos = find( _curPos, _endPos, '\0' );
		if (strEndPos >= _endPos) {
			_failed = true;
		} else {
			str.resize( strEndPos - _curPos );
			copy( _curPos, strEndPos, str.begin() );
			_curPos += str.size() + 1;
		}
	}
	return !_failed;
}


//----------------------------------------------------------------------------------------------------------------------


} // namespace own
