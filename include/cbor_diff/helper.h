#pragma once

#include <fc/io/json.hpp>
#include <fc/string.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

namespace cbor_diff
{
	namespace utils
	{
		bool string_ends_with(std::string str, std::string end);

		// �ҵ�һ���ַ���strȡ����׺ext��ʣ����ַ���
		std::string string_without_ext(std::string str, std::string ext);
	}
}