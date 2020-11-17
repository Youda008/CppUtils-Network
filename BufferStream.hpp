//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  2.11.2020
// Description: classes for binary serialization into binary buffers via operators << and >>
//======================================================================================================================

#ifndef CPPUTILS_BUFFER_STREAM_INCLUDED
#define CPPUTILS_BUFFER_STREAM_INCLUDED


#include "Essential.hpp"

#include "LangUtils.hpp"

#include <vector>
#include <array>
#include <string>
#include <algorithm>  // copy


namespace own {


//======================================================================================================================
/** binary buffer input stream allowing serialization via operator << */

class BufferOutputStream
{

 public:

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object
	  * and for allocating the storage big enough for all write operations to fit in. */
	BufferOutputStream( uint8_t * buffer, size_t size ) : _curPos( buffer ), _endPos( buffer + size ) {}

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object
	  * and for allocating the storage big enough for all write operations to fit in. */
	template< size_t size >
	BufferOutputStream( uint8_t (& buffer) [size] ) : _curPos( std::begin(buffer) ), _endPos( std::end(buffer) ) {}

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object
	  * and for allocating the storage big enough for all write operations to fit in. */
	BufferOutputStream( std::vector< uint8_t > & buffer ) : _curPos( &*buffer.begin() ), _endPos( &*buffer.end() ) {}

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object
	  * and for allocating the storage big enough for all write operations to fit in. */
	template< size_t size >
	BufferOutputStream( std::array< uint8_t, size > & buffer ) : _curPos( &*buffer.begin() ), _endPos( &*buffer.end() ) {}

	~BufferOutputStream() {}

	void reset( uint8_t * buffer, size_t size )         { _curPos = buffer; _endPos = buffer + size; }
	template< size_t size >
	void reset( uint8_t (& buffer) [size] )             { _curPos = std::begin(buffer); _endPos = std::end(buffer); }
	void reset( std::vector< uint8_t > & buffer )       { _curPos = &*buffer.begin(); _endPos = &*buffer.end(); }
	template< size_t size >
	void reset( std::array< uint8_t, size > & buffer )  { _curPos = &*buffer.begin(); _endPos = &*buffer.end(); }

	//-- atomic elements -----------------------------------------------------------------------------------------------

	BufferOutputStream & operator<<( uint8_t b )
	{
		*_curPos = b;
		_curPos++;
		return *this;
	}

	BufferOutputStream & operator<<( char c )
	{
		*_curPos = uint8_t(c);
		_curPos++;
		return *this;
	}

	//-- integers ------------------------------------------------------------------------------------------------------

	/** Converts an arbitrary integral number from native format to big endian and writes it into the buffer. */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	void writeIntBE( IntType native )
	{
		//_buffer.resize( _buffer.size() + sizeof(IntType) );

		uint8_t * const bigEndianBegin = _curPos;
		uint8_t * const bigEndianEnd = bigEndianBegin + sizeof(IntType);

		uint8_t * pos = bigEndianEnd;
		while (pos > bigEndianBegin) { // can't use traditional for-loop approach, because decrementing past begin
			--pos;                     // is formally undefined and causes assertion in some standard lib implementations
			*pos = uint8_t(native & 0xFF);
			native = IntType(native >> 8);
		}

		_curPos += sizeof(IntType);
	}

	/** Converts an arbitrary integral number from native format to little endian and writes it into the buffer. */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	void writeIntLE( IntType native )
	{
		//_buffer.resize( _buffer.size() + sizeof(IntType) );

		uint8_t * const littleEndianBegin = _curPos;
		uint8_t * const littleEndianEnd = littleEndianBegin + sizeof(IntType);

		uint8_t * pos = littleEndianBegin;
		while (pos < littleEndianEnd) {
			*pos = uint8_t(native & 0xFF);
			native = IntType(native >> 8);
			++pos;
		}

		_curPos += sizeof(IntType);
	}

	/** Converts an integer representation of an enum value from native format to big endian and writes it into the buffer. */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	void writeEnumBE( EnumType native )
	{
		writeIntBE( enumToInt( native ) );
	}

	/** Converts an integer representation of an enum value from native format to little endian and writes it into the buffer. */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	void writeEnumLE( EnumType native )
	{
		writeIntLE( enumToInt( native ) );
	}

	//-- strings and arrays --------------------------------------------------------------------------------------------

	/** Writes a string WITHOUT its null terminator to the buffer. */
	void writeString( const std::string & str );

	/** Writes a string WITH its null terminator to the buffer. */
	void writeString0( const std::string & str );

	/** Writes a string and its null terminator to the buffer. */
	BufferOutputStream & operator<<( const std::string & str )
	{
		writeString0( str );
		return *this;
	}

	// TODO: write arbitrary number of characters or bytes

	//-- error handling ------------------------------------------------------------------------------------------------

	bool overflew() const { return _overflew; }

 private:

	uint8_t * _curPos;  ///< current position in the buffer
	uint8_t * _endPos;  ///< ending position in the buffer
	bool _overflew = false;  ///< the end was reached while attemting to write into buffer

};


//======================================================================================================================
/** binary buffer input stream allowing deserialization via operator >> */

class BufferInputStream
{

 public:

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object. */
	BufferInputStream( const uint8_t * buffer, size_t size ) : _curPos( buffer ), _endPos( buffer + size ) {}

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object. */
	template< size_t size >
	BufferInputStream( const uint8_t (& buffer) [size] ) : _curPos( std::begin(buffer) ), _endPos( std::end(buffer) ) {}

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object. */
	BufferInputStream( const std::vector< uint8_t > & buffer ) : _curPos( &*buffer.begin() ), _endPos( &*buffer.end() ) {}

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object. */
	template< size_t size >
	BufferInputStream( const std::array< uint8_t, size > & buffer ) : _curPos( &*buffer.begin() ), _endPos( &*buffer.end() ) {}

	~BufferInputStream() {}

	void reset( const uint8_t * buffer, size_t size )        { _curPos = buffer; _endPos = buffer + size; }
	template< size_t size >
	void reset( const uint8_t (& buffer) [size] )            { _curPos = std::begin(buffer); _endPos = std::end(buffer); }
	void reset( const std::vector< uint8_t > & buffer )      { _curPos = &*buffer.begin(); _endPos = &*buffer.end(); }
	template< size_t size >
	void reset( const std::array< uint8_t, size > & buffer ) { _curPos = &*buffer.begin(); _endPos = &*buffer.end(); }

	//-- atomic elements -----------------------------------------------------------------------------------------------

	uint8_t get()
	{
		if (canRead( 1 )) {
			return *(_curPos++);
		} else {
			return 0;
		}
	}

	char getChar()
	{
		return char( get() );
	}

	BufferInputStream & operator>>( uint8_t & b )
	{
		b = get();
		return *this;
	}

	BufferInputStream & operator>>( char & b )
	{
		b = getChar();
		return *this;
	}

	//-- integers ------------------------------------------------------------------------------------------------------

	/** Reads an arbitrary integral number from the buffer and converts it from big endian to native format.
	  * (return value variant) */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	IntType readIntBE()
	{
		IntType native = 0;

		if (canRead( sizeof(IntType) ))
		{
			const uint8_t * const bigEndianBegin = _curPos;
			const uint8_t * const bigEndianEnd = bigEndianBegin + sizeof(IntType);

			const uint8_t * pos = bigEndianBegin;
			while (pos < bigEndianEnd) {
				native = IntType(native << 8);
				native = IntType(native | *pos);
				++pos;
			}

			_curPos += sizeof(IntType);
		}

		return native;
	}

	/** Reads an arbitrary integral number from the buffer and converts it from big endian to native format.
	  * (output parameter variant) */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	bool readIntBE( IntType & native )
	{
		native = readIntBE< IntType >();
		return !_failed;
	}

	/** Reads an arbitrary integral number from the buffer and converts it from little endian to native format.
	  * (return value variant) */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	IntType readIntLE()
	{
		IntType native = 0;

		if (canRead( sizeof(IntType) ))
		{
			const uint8_t * const littleEndianBegin = _curPos;
			const uint8_t * const littleEndianEnd = littleEndianBegin + sizeof(IntType);

			const uint8_t * pos = littleEndianEnd;
			while (pos > littleEndianBegin) { // can't use traditional for-loop approach, because decrementing past begin
				--pos;                        // is formally undefined and causes assertion in some standard lib implementations
				native = IntType(native << 8);
				native = IntType(native | *pos);
			}

			_curPos += sizeof(IntType);
		}

		return native;
	}

	/** Reads an arbitrary integral number from the buffer and converts it from little endian to native format.
	  * (output parameter variant) */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	bool readIntLE( IntType & native )
	{
		native = readIntLE< IntType >();
		return !_failed;
	}

	/** Reads an integer representation of an enum value from the buffer and converts it from big endian to native format.
	  * (return value variant) */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	EnumType readEnumBE()
	{
		return EnumType( readIntBE< typename std::underlying_type< EnumType >::type >() );
	}

	/** Reads an integer representation of an enum value from the buffer and converts it from big endian to native format.
	  * (output parameter variant) */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	bool readEnumBE( EnumType & native )
	{
		native = readEnumBE< EnumType >();
		return !_failed;
	}

	/** Reads an integer representation of an enum value from the buffer and converts it from little endian to native format.
	  * (return value variant) */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	EnumType readEnumLE()
	{
		return EnumType( readIntLE< typename std::underlying_type< EnumType >::type >() );
	}

	/** Reads an integer representation of an enum value from the buffer and converts it from little endian to native format.
	  * (output parameter variant) */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	bool readEnumLE( EnumType & native )
	{
		native = readEnumLE< EnumType >();
		return !_failed;
	}

	//-- strings and arrays --------------------------------------------------------------------------------------------

	/** Reads a string of specified size from the buffer.
	  * (output parameter variant) */
	bool readString( std::string & str, size_t size );

	/** Reads a string of specified size from the buffer.
	  * (return value variant) */
	std::string readString( size_t size )
	{
		std::string str;
		readString( str, size );
		return str;
	}

	/** Reads a string from the buffer until a null terminator is found.
	  * (output parameter variant) */
	bool readString0( std::string & str );

	/** Reads a string from the buffer until a null terminator is found.
	  * (return value variant) */
	std::string readString0()
	{
		std::string str;
		readString0( str );
		return str;
	}

	/** Reads a string from the buffer until a null terminator is found. */
	BufferInputStream & operator>>( std::string & str )
	{
		readString0( str );
		return *this;
	}

	// TODO: read arbitrary number of characters or bytes

	//-- error handling ------------------------------------------------------------------------------------------------

	void setFailed() { _failed = true; }
	void resetFailed() { _failed = false; }
	bool hasFailed() const { return _failed; }

 private:

	bool canRead( size_t size )
	{
		// the _failed flag can be true already from the previous call, in that case it will stay failed
		_failed |= _curPos + size > _endPos;
		return !_failed;
	}

 private:

	const uint8_t * _curPos;
	const uint8_t * _endPos;
	bool _failed = false;

};


//======================================================================================================================


} // namespace own


//======================================================================================================================
//  this allows you to use operator << and >> for integers and enums without losing the choice between little/big endian

#define MAKE_LITTLE_ENDIAN_DEFAULT \
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 > \
	static own::BufferOutputStream & operator<<( own::BufferOutputStream & stream, IntType val ) \
	{\
		stream.writeIntLE( val ); \
		return stream; \
	}\
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 > \
	static own::BufferOutputStream & operator<<( own::BufferOutputStream & stream, EnumType val ) \
	{\
		stream.writeEnumLE( val ); \
		return stream; \
	}\
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 > \
	static own::BufferInputStream & operator>>( own::BufferInputStream & stream, IntType & val ) \
	{\
		stream.readIntLE( val ); \
		return stream; \
	}\
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 > \
	static own::BufferInputStream & operator>>( own::BufferInputStream & stream, EnumType & val ) \
	{\
		stream.readEnumLE( val ); \
		return stream; \
	}

#define MAKE_BIG_ENDIAN_DEFAULT \
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 > \
	static own::BufferOutputStream & operator<<( own::BufferOutputStream & stream, IntType val ) \
	{\
		stream.writeIntBE( val ); \
		return stream; \
	}\
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 > \
	static own::BufferOutputStream & operator<<( own::BufferOutputStream & stream, EnumType val ) \
	{\
		stream.writeEnumBE( val ); \
		return stream; \
	}\
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 > \
	static own::BufferInputStream & operator>>( own::BufferInputStream & stream, IntType & val ) \
	{\
		stream.readIntBE( val ); \
		return stream; \
	}\
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 > \
	static own::BufferInputStream & operator>>( own::BufferInputStream & stream, EnumType & val ) \
	{\
		stream.readEnumBE( val ); \
		return stream; \
	}


//======================================================================================================================


#endif // CPPUTILS_BUFFER_STREAM_INCLUDED
