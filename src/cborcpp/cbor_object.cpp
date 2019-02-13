#include "cborcpp/cbor_object.h"
#include <fc/crypto/hex.hpp>
#include <string>
#include <sstream>

namespace cbor {

	std::string CborObject::str() const {
		switch (value.which()) {
		case CborObjectValue::tag<CborBoolValue>::value:
			return as_bool() ? "true" : "false";
		case CborObjectValue::tag<CborIntValue>::value:
			return std::to_string(as_int());
		case CborObjectValue::tag<CborExtraIntValue>::value:
			return std::to_string(as_extra_int());
		case CborObjectValue::tag<CborDoubleValue>::value:
			return std::to_string(as_float64());
		case CborObjectValue::tag<CborTagValue>::value:
			return std::string("tag<") + std::to_string(as_tag()) + ">";
		case CborObjectValue::tag<CborStringValue>::value:
			return as_string();
		case CborObjectValue::tag<CborBytesValue>::value: {
			const auto& bytes = as_bytes();
			const auto& hex = fc::to_hex(bytes);
			return std::string("bytes<") + hex + ">";
		}
		case CborObjectValue::tag<CborArrayValue>::value: {
			const auto& items = as_array();
			std::stringstream ss;
			ss << "[";
			for (size_t i = 0; i < items.size(); i++) {
				if (i > 0) {
					ss << ", ";
				}
				ss << items[i]->str();
			}
			ss << "]";
			return ss.str();
		} break;
		case CborObjectValue::tag<CborMapValue>::value:
		{
			const auto& items = as_map();
			std::stringstream ss;
			ss << "[";
			auto begin = items.begin();
			for (auto it = items.begin(); it != items.end();it++) {
				if (it != begin) {
					ss << ", ";
				}
				ss << it->first << ": " << it->second->str();
			}
			ss << "]";
			return ss.str();
		}
		default:
			throw cbor::CborException(std::string("not supported cbor object type ") + std::to_string(value.which()));
		}
	}

	CborObjectP CborObject::from(const CborObjectValue& value) {
		auto result = std::make_shared<CborObject>();
		result->value = value;
		switch (value.which()) {
		case CborObjectValue::tag<CborBoolValue>::value: 
			result->type = COT_BOOL;
			break;
		case CborObjectValue::tag<CborIntValue>::value:
			result->type = COT_INT;
			break;
		case CborObjectValue::tag<CborExtraIntValue>::value:
			result->type = COT_EXTRA_INT;
			break;
		case CborObjectValue::tag<CborDoubleValue>::value:
			result->type = COT_FLOAT;
			break;
		case CborObjectValue::tag<CborTagValue>::value:
			result->type = COT_TAG;
			break;
		case CborObjectValue::tag<CborStringValue>::value:
			result->type = COT_STRING;
			break;
		case CborObjectValue::tag<CborBytesValue>::value:
			result->type = COT_BYTES;
			break;
		case CborObjectValue::tag<CborArrayValue>::value:
			result->type = COT_ARRAY;
			break;
		case CborObjectValue::tag<CborMapValue>::value:
			result->type = COT_MAP;
			break;
		default:
			throw cbor::CborException(std::string("not supported cbor object type ") + std::to_string(value.which()));
		}
		return result;
	}

	CborObjectP CborObject::from_int(CborIntValue value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_INT;
		result->value = value;
		return result;
	}
	CborObjectP CborObject::from_bool(CborBoolValue value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_BOOL;
		result->value = value;
		return result;
	}
	CborObjectP CborObject::from_bytes(const CborBytesValue& value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_BYTES;
		result->value = value;
		return result;
	}
	CborObjectP CborObject::from_float64(const CborDoubleValue& value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_FLOAT;
		result->value = value;
		return result;
	}
	CborObjectP CborObject::from_string(const std::string& value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_STRING;
		result->value = value;
		return result;
	}
	CborObjectP CborObject::create_array(size_t size) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_ARRAY;
		result->value = CborArrayValue();
		result->array_or_map_size = size;
		return result;
	}
	CborObjectP CborObject::create_array(const CborArrayValue& items) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_ARRAY;
		result->value = CborArrayValue(items);
		result->array_or_map_size = items.size();
		return result;
	}
	CborObjectP CborObject::create_map(size_t size) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_MAP;
		result->value = CborMapValue();
		result->array_or_map_size = size;
		return result;
	}
	CborObjectP CborObject::create_map(const CborMapValue& items) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_MAP;
		result->value = CborMapValue(items);
		result->array_or_map_size = items.size();
		return result;
	}
	CborObjectP CborObject::from_tag(CborTagValue value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_TAG;
		result->value = value;
		return result;
	}
	CborObjectP CborObject::create_undefined() {
		auto result = std::make_shared<CborObject>();
		result->type = COT_UNDEFINED;
		return result;
	}
	CborObjectP CborObject::create_null() {
		auto result = std::make_shared<CborObject>();
		result->type = COT_NULL;
		return result;
	}
	/*CborObjectP CborObject::from_special(uint32_t value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_SPECIAL;
		result->value = value;
		return result;
	}*/
	CborObjectP CborObject::from_extra_integer(uint64_t value, bool sign) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_EXTRA_INT;
		result->value = value;
		result->is_positive_extra = sign;
		return result;
	}
	CborObjectP CborObject::from_extra_tag(uint64_t value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_EXTRA_TAG;
		result->value = value;
		return result;
	}
	/*CborObjectP CborObject::from_extra_special(uint64_t value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_EXTRA_SPECIAL;
		result->value = value;
		return result;
	}*/
}