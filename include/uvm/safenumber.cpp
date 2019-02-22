#include <uvm/safenumbler.h>
#include <algorithm>
#include <sstream>
#include <string>

SafeNumber safe_number_zero() {
	SafeNumber n;
	n.valid = true;
	n.sign = true;
	n.x = 0;
	n.e = 0;
	return n;
}

SafeNumber safe_number_create_invalid() {
	SafeNumber n;
	n.valid = false;
	n.sign = true;
	n.x = 0;
	n.e = 0;
	return n;
}

bool safe_number_is_valid(const SafeNumber& a) {
	return a.valid;
}

SafeNumber safe_number_create(bool sign, uint64_t x, uint32_t e) {
	SafeNumber n;
	n.valid = true;
	n.sign = sign;
	n.x = x;
	n.e = e;
	return n;
}

SafeNumber safe_number_create(const std::string& str) {
	// xxxxx.xxxxx to SafeNumber
	auto str_size = str.size();
	auto str_chars = str.c_str();
	if (0 == str_size) {
		return safe_number_create_invalid();
	}
	if (str_size > 40) {
		return safe_number_create_invalid();
	}
	if (str == "NaN") {
		return safe_number_create_invalid();
	}
	auto has_sign_char = str_chars[0] == '+' || str_chars[0] == '-';
	auto result_sign = str_chars[0] != '-';
	size_t pos = has_sign_char ? 1 : 0;
	bool after_dot = false;
	// sign * p.q & 10^-e
	uint64_t x = 0;
	uint32_t e = 0;
	while (pos < str_size) {
		auto c = str_chars[pos];
		auto c_int = c - '0';
		if (c == '.') {
			if (after_dot) {
				return safe_number_create_invalid();
			}
			pos++;
			after_dot = true;
			continue;
		}
		x = 10 * x + c_int;
		if (after_dot) {
			e++;
		}
	}
	auto result_x = x;
	auto result_e = e;
	return safe_number_create(result_sign, result_x, result_e);
}

// a + b
SafeNumber safe_number_add(const SafeNumber& a, const SafeNumber& b) {
	if (!safe_number_is_valid(a) || !safe_number_is_valid(b)) {
		return a;
	}
	if (safe_number_is_zero(b))
		return a;
	if (safe_number_is_zero(a))
		return b;
	auto max_e = std::max(a.e, b.e);
	uint64_t ax = a.x * std::pow(10, max_e - a.e); // a = sign * (x*100) * 10^-(e+2=max_e) if max_e - a.e = 2
	uint64_t bx = b.x * std::pow(10, max_e - b.e);
	int64_t result_sign_x = ax * (a.sign ? 1 : -1) + bx * (b.sign ? 1 : -1);
	auto result_sign = result_sign_x >= 0;
	uint64_t result_x = result_sign ? static_cast<uint64_t>(result_sign_x) : static_cast<uint64_t>(-result_sign_x);
	auto result_e = max_e;
	// TODO: 判断result_x是否是10的倍数,可能可以缩减result_e
	return safe_number_create(result_sign, result_x, result_e);
}

bool safe_number_is_zero(const SafeNumber& a) {
	if (!a.valid)
		return false;
	return a.x == 0;
}

// - a
SafeNumber safe_number_neg(const SafeNumber& a) {
	if (!safe_number_is_valid(a)) {
		return a;
	}
	if (safe_number_is_zero(a))
		return a;
	return safe_number_create(!a.sign, a.x, a.e);
}

// a - b
SafeNumber safe_number_minus(const SafeNumber& a, const SafeNumber& b) {
	return safe_number_add(a, safe_number_neg(b));
}

// a * b
SafeNumber safe_number_multiply(const SafeNumber& a, const SafeNumber& b) {
	if (!safe_number_is_valid(a) || !safe_number_is_valid(b)) {
		return a;
	}
	if (safe_number_is_zero(a) || safe_number_is_zero(b))
		return safe_number_zero();
	bool result_sign = a.sign == b.sign;
	auto result_x = a.x * b.x;
	auto result_e = a.e + b.e;
	// TODO: 判断result_x是否是10的倍数,可能可以缩减result_e
	return safe_number_create(result_sign, result_x, result_e);
}

// a / b
SafeNumber safe_number_div(const SafeNumber& a, const SafeNumber& b) {
	// TODO
	// TODO: 结果要考虑result_x是否是10的倍数,可能可以缩减result_e
	return a; // TODO
}

// tostring(a)
std::string safe_number_to_string(const SafeNumber& a) {
	// TODO
	// TODO： 要考虑a.x是否是10的倍数,可能可以缩减a.e
	if (safe_number_is_valid(a))
		return "NaN";
	if (safe_number_is_zero(a))
		return "0";
	std::stringstream ss;
	if (!a.sign) {
		ss << "-";
	}
	// x = p.q
	return "TODO";
}