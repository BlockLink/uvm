#pragma once

#include <vector>

namespace uvm
{
	namespace lua
	{
		namespace lib
		{

			class UvmByteStream
			{
			private:
				size_t _pos;
				std::vector<char> _buff;
			public:
				inline UvmByteStream() : _pos(0) {}
				inline virtual ~UvmByteStream() {}

				inline bool eof() const
				{
					return _pos >= _buff.size();
				}

				inline char current() const
				{
					return eof() ? -1 : _buff[_pos];
				}

				inline bool next()
				{
					if (eof())
						return false;
					++_pos;
					return true;
				}

				inline size_t pos() const
				{
					return _pos;
				}

				inline void reset_pos()
				{
					_pos = 0;
				}

				inline size_t size() const
				{
					return _buff.size();
				}

				inline std::vector<char>::const_iterator begin() const
				{
					return _buff.begin();
				}

				inline std::vector<char>::const_iterator end() const
				{
					return _buff.end();
				}

				inline void push(char c)
				{
					_buff.push_back(c);
				}

				inline void push_some(std::vector<char> const& data)
				{
					_buff.resize(_buff.size() + data.size());
					memcpy(_buff.data() + _buff.size(), data.data(), sizeof(char) * data.size());
				}

				inline bool equals(UvmByteStream* other)
				{
					if (!other)
						return false;
					if (this == other)
						return true;
					if (this->size() != other->size())
						return false;
					return this->_buff == other->_buff;
				}

			};
		}
	}
}
