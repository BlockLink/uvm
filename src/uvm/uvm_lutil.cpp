#include <uvm/lprefix.h>

#include <string>
#include <algorithm>
#include <sstream>

#include <uvm/uvm_lutil.h>
#include <uvm/lstate.h>
#include <uvm/exceptions.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <fc/crypto/hex.hpp>

namespace uvm
{
	namespace util
	{
		typedef boost::multiprecision::int256_t sm_bigint;

		inline char file_separator()
		{
#if defined(_WIN32) or defined(WIN32)
			return '\\';
#else
			return '/';
#endif
		}

		static char *FILE_SEPRATOR_STR = nullptr;

		char *file_separator_str()
		{
			if (FILE_SEPRATOR_STR)
				return FILE_SEPRATOR_STR;
			FILE_SEPRATOR_STR = (char*)calloc(2, sizeof(char));
			FILE_SEPRATOR_STR[0] = file_separator();
			FILE_SEPRATOR_STR[1] = '\0';
			return FILE_SEPRATOR_STR;
		}

		char *copy_str(const char *str)
		{
			if (!str)
				return nullptr;
			char *copied = (char*)malloc(strlen(str) + 1);
			strcpy(copied, str);
			return copied;
		}

		bool ends_with(const std::string &str, const std::string &suffix)
		{
			return str.size() >= suffix.size() &&
				str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
		}

		bool starts_with(const std::string &str, const std::string &prefix)
		{
			return str.size() >= prefix.size() &&
				str.compare(0, prefix.size(), prefix) == 0;
		}

		bool compare_string_list(std::list<std::string> &l1, std::list<std::string> &l2)
		{
			if (l1.size() != l2.size())
				return false;
			std::list<std::string>::iterator it1 = l1.begin();
			std::list<std::string>::iterator it2 = l2.begin();
			std::list<std::string>::iterator end1 = l1.end();
			while (it1 != end1)
			{
				if (*it1 != *it2) {
					return false;
				}
				++it1;
				++it2;
			}
			return true;
		}

		void replace_all(std::string& str, const std::string& from, const std::string& to) {
			boost::replace_all(str, from, to);
		}

		static std::string need_escaped_chars = "ntabfrv";

		std::string escape_string(std::string &str)
		{
			std::string result(str);
			uvm::util::replace_all(result, "\\", "\\\\");
			uvm::util::replace_all(result, "\"", "\\\"");
			uvm::util::replace_all(result, "\n", "\\n");
			uvm::util::replace_all(result, "\t", "\\t");
			uvm::util::replace_all(result, "\a", "\\a");
			uvm::util::replace_all(result, "\b", "\\b");
			uvm::util::replace_all(result, "\f", "\\f");
			uvm::util::replace_all(result, "\r", "\\r");
			uvm::util::replace_all(result, "\v", "\\v");
			return result;
		}

		std::string unescape_string(std::string &str)
		{
			std::string result(str);
			uvm::util::replace_all(result, "\\\\", "\\");
			uvm::util::replace_all(result, "\\\"", "\"");
			uvm::util::replace_all(result, "\\n", "\n");
			uvm::util::replace_all(result, "\\t", "\t");
			uvm::util::replace_all(result, "\\a", "\a");
			uvm::util::replace_all(result, "\\b", "\b");
			uvm::util::replace_all(result, "\\f", "\f");
			uvm::util::replace_all(result, "\\r", "\r");
			uvm::util::replace_all(result, "\\v", "\v");
			return result;
		}

		std::vector<std::string> &string_split(const std::string &s, char delim, std::vector<std::string> &elems)
		{
			std::stringstream ss(s);
			std::string item;
			while (std::getline(ss, item, delim)) {
				elems.push_back(item);
			}
			return elems;
		}

		std::vector<std::string> string_split(const std::string &s, char delim) {
			std::vector<std::string> elems;
			string_split(s, delim, elems);
			return elems;
		}

		std::string string_trim(std::string source)
		{
			while (source.length() > 0 && (source[0] == ' ' || source[0] == '\t' || source[0] == '\n' || source[0] == '\r'))
				source.erase(0, source.find_first_of(source[0]) + 1);
			while (source.length() > 0 && (source[source.length() - 1] == ' ' || source[source.length() - 1] == '\t' || source[source.length() - 1] == '\n' || source[source.length()-1] == '\r'))
				source.erase(source.find_last_not_of(source[source.length() - 1]) + 1);
			return source;
		}

		size_t string_lines_count(const std::string &str)
		{
			return std::count(str.begin(), str.end(), '\n') + 1;
		}

		Buffer RlpEncoder::encode(unsigned char input) {
			std::string str;
			str.resize(1);
			str[0] = input;
			return encode(str);
		}

		static Buffer concat_buffers(const Buffer& a, const Buffer& b)
		{
			std::vector<unsigned char> bytes(a.size() + b.size());
			memcpy(bytes.data(), a.data(), a.size());
			memcpy(bytes.data() + a.size(), b.data(), b.size());
			return bytes;
		}

		static Buffer bytes_to_buffer(const std::vector<unsigned char>& data) {
			std::vector<unsigned char> bytes(data.begin(), data.end());
			return bytes;
		}

		static Buffer byte_to_buffer(unsigned char b) {
			std::vector<unsigned char> bytes(1);
			bytes[0] = b;
			return bytes;
		}

		static Buffer int_to_buffer(uint32_t value) {
			Buffer result;
			if (value == 0) {
				return result;
			}
			else {
				return concat_buffers(int_to_buffer(value / 256), byte_to_buffer(value % 256));
			}
		}

		Buffer RlpEncoder::encode(const std::string& input) {
			return encode(std::vector<unsigned char>(input.begin(), input.end()));
		}
		Buffer RlpEncoder::encode(const std::vector<unsigned char>& input) {
			if (input.size() == 1 && input[0] < 128) {
				return bytes_to_buffer(input);
			}
			const auto& lenth_encoded = encode_length(input.size(), 128);
			const auto& body_encoded = bytes_to_buffer(input);
			return concat_buffers(lenth_encoded, body_encoded);
		}
		Buffer RlpEncoder::encode(const std::vector<std::shared_ptr<RlpObject> >& objects) {
			Buffer result;
			for (const auto& item : objects) {
				result = concat_buffers(result, encode(item));
			}
			result = concat_buffers(encode_length(result.size(), 192), result);
			return result;
		}
		Buffer RlpEncoder::encode(std::shared_ptr<RlpObject> obj) {
			switch (obj->type) {
			case RlpObjectType::ROT_BYTES:
			{
				return encode(obj->bytes);
			} break;
			case RlpObjectType::ROT_LIST: {
				return encode(obj->objects);
			} break;
			default: {
				throw uvm::core::UvmException("unknown rlp object type");
			}
			}
		}

		Buffer RlpEncoder::encode_length(uint32_t length, int32_t offset) {
			if (length < 56)
			{
				return byte_to_buffer((unsigned char)(int32_t(length) + offset));
			}
			else if (length < UINT32_MAX) {
				const auto& length_buffer = int_to_buffer(length);
				return concat_buffers(byte_to_buffer(length_buffer.size() + offset + 55), length_buffer);
			}
			else {
				throw uvm::core::UvmException("too long bytes to rlp encode");
			}
		}

		std::shared_ptr<RlpObject> RlpDecoder::decode(const Buffer& bytes) {
			if (bytes.empty()) {
				std::shared_ptr<RlpObject> result = std::make_shared<RlpObject>();
				result->type = RlpObjectType::ROT_LIST;
				result->objects = std::vector<std::shared_ptr<RlpObject> >();
				return result;
			}
			const auto& decode_result = decode_non_empty(BufferSlice::create(std::make_shared<Buffer>(bytes), 0));
			if (!decode_result.source.empty())
				throw uvm::core::UvmException("rlp decode failed, some remaining bytes not decoded");
			return decode_result.data;
		}

		std::shared_ptr<RlpObject> RlpObject::from_char(char c) {
			auto obj = std::make_shared<RlpObject>();
			obj->type = RlpObjectType::ROT_BYTES;
			obj->bytes.resize(1);
			obj->bytes[0] = c;
			return obj;
		}

		std::shared_ptr<RlpObject> RlpObject::from_int32(int32_t value) {
			return RlpObject::pack_to_bytes<int32_t>(value);
		}

		std::shared_ptr<RlpObject> RlpObject::from_int64(int64_t value) {
			return RlpObject::pack_to_bytes<int64_t>(value);
		}
		std::shared_ptr<RlpObject> RlpObject::from_string(const std::string& str) {
			Buffer buf(str.begin(), str.end());
			return from_bytes(buf);
		}
		std::shared_ptr<RlpObject> RlpObject::from_bytes(const Buffer& buf) {
			auto obj = std::make_shared<RlpObject>();
			obj->type = RlpObjectType::ROT_BYTES;
			obj->bytes = buf;
			return obj;
		}

		std::shared_ptr<RlpObject> RlpObject::from_list(std::vector<std::shared_ptr<RlpObject> > objects) {
			auto obj = std::make_shared<RlpObject>();
			obj->type = RlpObjectType::ROT_LIST;
			obj->objects = objects;
			return obj;
		}

		static void uvm_assert(bool cond, const std::string& error) {
			if (!cond) {
				throw uvm::core::UvmException(error);
			}
		}

		int32_t RlpObject::to_int32() const {
			uvm_assert(type == RlpObjectType::ROT_BYTES, "invalid rlp object type");
			return little_endian_unpack_int<int32_t>(bytes);
		}
		int64_t RlpObject::to_int64() const {
			uvm_assert(type == RlpObjectType::ROT_BYTES, "invalid rlp object type");
			return little_endian_unpack_int<int64_t>(bytes);
		}
		char RlpObject::to_char() const {
			uvm_assert(type == RlpObjectType::ROT_BYTES, "invalid rlp object type");
			uvm_assert(bytes.size() == 1, "invalid rlp char");
			return (char) bytes.at(0);
		}
		std::string RlpObject::to_string() const {
			uvm_assert(type == RlpObjectType::ROT_BYTES, "invalid rlp object type");
			std::vector<char> data(bytes.size());
			memcpy(data.data(), bytes.data(), bytes.size());
			return std::string(data.begin(), data.end());
		}
		Buffer RlpObject::to_buffer() const {
			uvm_assert(type == RlpObjectType::ROT_BYTES, "invalid rlp object type");
			Buffer data(bytes.size());
			memcpy(data.data(), bytes.data(), bytes.size());
			return data;
		}

		size_t BufferSlice::size() const {
			if (!source)
				return 0;
			auto source_size = source->size();
			return (source_size >= offset) ? (source_size - offset) : 0;
		}

		unsigned char BufferSlice::at(size_t pos) const {
			if (!source) {
				Buffer zero_buf;
				return zero_buf.at(0);
			}
			auto real_pos = pos + offset;
			return source->at(real_pos);
		}

		bool BufferSlice::empty() const {
			return !source || offset >= source->size();
		}

		BufferSlice BufferSlice::slice(size_t from) const {
			BufferSlice result;
			result.source = this->source;
			result.offset = this->offset + from;
			return result;
		}

		std::shared_ptr<Buffer> BufferSlice::clone_slice(size_t from, size_t length) const {
			auto result = std::make_shared<Buffer>();
			auto sliced = slice(from);
			auto sliced_size = sliced.size();
			for (size_t i = 0; i < std::min(sliced_size, length); i++) {
				result->push_back(sliced.at(i));
			}
			return result;
		}

		std::shared_ptr<Buffer> BufferSlice::to_buffer() const {
			auto result = std::make_shared<Buffer>();
			auto this_size = this->size();
			for (size_t i = 0; i < this_size; i++) {
				result->push_back(this->at(i));
			}
			return result;
		}

		BufferSlice BufferSlice::create(std::shared_ptr<Buffer> source, size_t offset) {
			BufferSlice slice;
			slice.source = source;
			slice.offset = offset;
			return slice;
		}

		std::string buffer_to_hex(const Buffer& buffer) {
			std::vector<char> chars(buffer.size());
			memcpy(chars.data(), buffer.data(), buffer.size());
			try {
				return fc::to_hex(chars);
			}
			catch (const std::exception& e) {
				throw uvm::core::UvmException(e.what());
			}
		}

		int64_t hex_to_int(const std::string& hex_str) {
			try {
				Buffer bytes(hex_str.size() / 2);
				auto decoded_bytes_count = fc::from_hex(hex_str, (char*)bytes.data(), bytes.size());
				return little_endian_unpack_int<int64_t>(bytes);
			}
			catch (const std::exception& e) {
				throw uvm::core::UvmException(e.what());
			}
		}

		DecodeResult RlpDecoder::decode_non_empty(const BufferSlice& bytes) {
			std::shared_ptr<RlpObject> result = std::make_shared<RlpObject>();
			result->type = RlpObjectType::ROT_BYTES;
			DecodeResult decode_result;
			auto first_byte = bytes.at(0);
			if (first_byte <= 0x7f) {
				// a single byte whose value is in the [0x00, 0x7f] range, that byte is its own RLP encoding.
				result->bytes = byte_to_buffer(first_byte);
				decode_result.data = result;
				decode_result.source = bytes.slice(1);
				return decode_result;
			}
			else if (first_byte <= 0xb7) {
				// string is 0-55 bytes long. A single byte with value 0x80 plus the length of the string followed by the string
				// The range of the first byte is [0x80, 0xb7]
				auto length = first_byte - 0x7f;
				// set 0x80 null to 0
				if (first_byte == 0x80) {
					result->bytes = Buffer();
				}
				else {
					result->bytes = *bytes.clone_slice(1, length - 1);
				}
				if (length == 2 && result->bytes.at(0) < 0x80) {
					throw uvm::core::UvmException("invalid rlp encoding: byte must be less 0x80");
				}
				decode_result.data = result;
				decode_result.source = bytes.slice(length);
				return decode_result;
			}
			else if (first_byte <= 0xbf) {
				auto llength = first_byte - 0xb6; // list length
				auto length = hex_to_int(buffer_to_hex(*bytes.clone_slice(1, llength - 1)));
				result->bytes = *bytes.clone_slice(llength, length);
				if (result->bytes.size() < length) {
					throw uvm::core::UvmException("invalid RLP");
				}
				decode_result.data = result;
				decode_result.source = bytes.slice(length + llength);
				return decode_result;
			}
			else if (first_byte <= 0xf7) {
				// a list between  0-55 bytes long
				auto length = first_byte - 0xbf;
				auto innerRemainder = BufferSlice::create(bytes.clone_slice(1, length - 1));
				std::vector<std::shared_ptr<RlpObject>> decoded;
				while (!innerRemainder.empty()) {
					auto d = decode_non_empty(innerRemainder);
					decoded.push_back(d.data);
					innerRemainder = d.source;
				}
				result->type = RlpObjectType::ROT_LIST;
				result->objects = decoded;
				decode_result.data = result;
				decode_result.source = bytes.slice(length);
				return decode_result;
			}
			else {
				// a list  over 55 bytes long
				auto llength = first_byte - 0xf6;
				auto length = hex_to_int(buffer_to_hex(*bytes.clone_slice(1, llength - 1)));
				auto totalLength = llength + length;
				if (totalLength > bytes.size()) {
					throw uvm::core::UvmException("invalid rlp: total length is larger than the data");
				}

				auto innerRemainder = BufferSlice::create(bytes.clone_slice(llength, totalLength - llength));
				if (innerRemainder.empty()) {
					throw uvm::core::UvmException("invalid rlp, List has a invalid length");
				}
				std::vector<std::shared_ptr<RlpObject>> decoded;
				while (!innerRemainder.empty()) {
					auto d = decode_non_empty(innerRemainder);
					decoded.push_back(d.data);
					innerRemainder = d.source;
				}
				result->type = RlpObjectType::ROT_LIST;
				result->objects = decoded;
				decode_result.data = result;
				decode_result.source = bytes.slice(totalLength);
				return decode_result;
			}
			return decode_result;
		}

	} // end namespace uvm::util
} // end namespace uvm
