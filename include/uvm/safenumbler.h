#pragma once
#include <cstdint>
#include <string>

struct SafeNumber {
	// sign(+/-) x * (10^-e)
	bool valid; // isNaN
	bool sign; // sign == true means positive
	uint64_t x;
	uint32_t e;
};

SafeNumber safe_number_zero();

// create NaN number
SafeNumber safe_number_create_invalid();

bool safe_number_is_valid(const SafeNumber& a);

SafeNumber safe_number_create(bool sign, uint64_t x, uint32_t e);

SafeNumber safe_number_create(const std::string& str);

// a + b
SafeNumber safe_number_add(const SafeNumber& a, const SafeNumber& b);

// a == 0
bool safe_number_is_zero(const SafeNumber& a);

// - a
SafeNumber safe_number_neg(const SafeNumber& a);

// a - b
SafeNumber safe_number_minus(const SafeNumber& a, const SafeNumber& b);

// a * b
SafeNumber safe_number_multiply(const SafeNumber& a, const SafeNumber& b);

// a / b
SafeNumber safe_number_div(const SafeNumber& a, const SafeNumber& b);

// tostring(a)
std::string safe_number_to_string(const SafeNumber& a);
