#include "uvm/safenumber.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <cstdio>
#include <cmath>

const std::string NaN_str = "NaN";

static uint64_t uint64_pow(uint64_t a, int p) {
    if(a==10) {
        switch (p) {
            case 0: return 1;
            case 1: return 10;
            case 2: return 100;
            case 3: return 1000;
            case 4: return 10000;
            case 5: return 100000;
            case 6: return 1000000;
            case 7: return 10000000L;
            case 8: return 100000000L;
            case 9: return 1000000000L;
            case 16: return 10000000000000000L;
            default: {}
        }
    }
	return static_cast<uint64_t >(std::pow(static_cast<uint64_t>(a), p));
}


SimpleUint128 simple_uint128_create(uint64_t big, uint64_t low) {
	SimpleUint128 result;
	result.big = big;
	result.low = low;
	return result;
}

const SimpleUint128 uint128_0 = simple_uint128_create(0, 0);
const SimpleUint128 uint128_1 = simple_uint128_create(0, 1);
const SimpleUint128 uint128_10 = simple_uint128_create(0, 10);

bool simple_uint128_gt(const SimpleUint128& a, const SimpleUint128& b) {
	if (a.big == b.big){
		return (a.low > b.low);
	}
	return (a.big > b.big);
}

bool simple_uint128_eq(const SimpleUint128& a, const SimpleUint128& b) {
	return ((a.big == b.big) && (a.low == b.low));
}

bool simple_uint128_ge(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_gt(a, b) || simple_uint128_eq(a, b);
}

SimpleUint128 simple_uint128_add(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_create(a.big + b.big + ((a.low + b.low) < a.low), a.low + b.low);
}

SimpleUint128 simple_uint128_minus(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_create(a.big - b.big - ((a.low - b.low) > a.low), a.low - b.low);
}

SimpleUint128 simple_uint128_neg(const SimpleUint128& a) {
	SimpleUint128 reverse_a;
	reverse_a.big = ~a.big;
	reverse_a.low = ~a.low;
	return simple_uint128_add(reverse_a, uint128_1);
}

SimpleUint128 simple_uint128_multi(const SimpleUint128& a, const SimpleUint128& b) {
	// split values into 4 32-bit parts
	uint64_t top[4] = {a.big >> 32, a.big & 0xffffffff, a.low >> 32, a.low & 0xffffffff};
	uint64_t bottom[4] = {b.big >> 32, b.big & 0xffffffff, b.low >> 32, b.low & 0xffffffff};
	uint64_t products[4][4];

	// multiply each component of the values
	for(int y = 3; y > -1; y--){
		for(int x = 3; x > -1; x--){
			products[3 - x][y] = top[x] * bottom[y];
		}
	}

	// first row
	uint64_t fourth32 = (products[0][3] & 0xffffffff);
	uint64_t third32  = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
	uint64_t second32 = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
	uint64_t first32  = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);

	// second row
	third32  += (products[1][3] & 0xffffffff);
	second32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
	first32  += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);

	// third row
	second32 += (products[2][3] & 0xffffffff);
	first32  += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);

	// fourth row
	first32  += (products[3][3] & 0xffffffff);

	// move carry to next digit
	third32  += fourth32 >> 32;
	second32 += third32  >> 32;
	first32  += second32 >> 32;

	// remove carry from current digit
	fourth32 &= 0xffffffff;
	third32  &= 0xffffffff;
	second32 &= 0xffffffff;
	first32  &= 0xffffffff;

	// combine components
	auto result_big = (first32 << 32) | second32;
	auto result_low = (third32 << 32) | fourth32;
	return simple_uint128_create(result_big, result_low);
}

SimpleUint128 simple_uint128_shift_left(const SimpleUint128& a, const SimpleUint128& b) {
	const uint64_t shift = b.low;
	if (((bool) b.big) || (shift >= 128)){
		return uint128_0;
	}
	else if (shift == 64){
		return simple_uint128_create(a.low, 0);
	}
	else if (shift == 0){
		return a;
	}
	else if (shift < 64){
		return simple_uint128_create((a.big << shift) + (a.low >> (64 - shift)), a.low << shift);
	}
	else if ((128 > shift) && (shift > 64)){
		return simple_uint128_create(a.low << (shift - 64), 0);
	}
	else{
		return uint128_0;
	}
}

SimpleUint128 simple_uint128_shift_right(const SimpleUint128& a, const SimpleUint128& b) {
	const uint64_t shift = b.low;
	if (((bool) b.big) || (shift >= 128)){
		return uint128_0;
	}
	else if (shift == 64){
		return simple_uint128_create(0, a.big);
	}
	else if (shift == 0){
		return a;
	}
	else if (shift < 64){
		return simple_uint128_create(a.big >> shift, (a.big << (64 - shift)) + (a.low >> shift));
	}
	else if ((128 > shift) && (shift > 64)){
		return simple_uint128_create(0, (a.big >> (shift - 64)));
	}
	else{
		return uint128_0;
	}
}

SimpleUint128 simple_uint128_bit_and(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_create(a.big & b.big, a.low & b.low);
}

std::pair<SimpleUint128, SimpleUint128> simple_uint128_divmod(const SimpleUint128& a, const SimpleUint128& b) {
	if (b.big == 0 && b.low == 0){
		throw std::domain_error("Error: division or modulus by 0");
	}
	else if (b.big == 0 && b.low == 1){
		return std::make_pair(a, uint128_0);
	}
	else if (a.big == b.big && a.low == b.low){
		return std::make_pair(uint128_1, uint128_0);
	}
	else if ((a.big == 0 && a.low == 0) || (a.big < b.big || (a.big==b.big && a.low<b.low))){
		return std::make_pair(uint128_0, a);
	}
	SimpleUint128 div_result = simple_uint128_create(0, 0);
	SimpleUint128 mod_result = simple_uint128_create(0, 0);
	for(uint8_t x = simple_uint128_bits(a); x > 0; x--){
		div_result = simple_uint128_shift_left(div_result, uint128_1);
		mod_result = simple_uint128_shift_left(mod_result, uint128_1);

		auto tmp1 = simple_uint128_shift_right(a, simple_uint128_create(0, x-1U));
		auto tmp2 = simple_uint128_bit_and(tmp1, uint128_1);
		if (tmp2.big || tmp2.low){
			mod_result = simple_uint128_add(mod_result, uint128_1);
		}
		if (simple_uint128_ge(mod_result, b)){
			mod_result = simple_uint128_minus(mod_result, b);
			div_result = simple_uint128_add(div_result, uint128_1);
		}
	}
	return std::make_pair(div_result, mod_result);
}

uint8_t simple_uint128_bits(const SimpleUint128& a) {
	uint8_t out = 0;
	if (a.big){
		out = 64;
		uint64_t up = a.big;
		while (up){
			up >>= 1;
			out++;
		}
	}
	else{
		uint64_t low = a.low;
		while (low){
			low >>= 1;
			out++;
		}
	}
	return out;
}

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
const uint64_t largest_x = uint64_pow(10, 16);

bool safe_number_is_valid(const SafeNumber& a) {
	return a.valid;
}


static SafeNumber compress_number(const SafeNumber& a) {
	// compress a's value. If a.x is a multiple of 10, the value of a.e can be reduced accordingly.
	if(!safe_number_is_valid(a)) {
		return a;
	}
	uint64_t x = a.x;
	uint32_t e = a.e;
	// when a.x * b.x is too big. should short result x and result e. Requires certainty and does not require complete accuracy
	while(x > largest_x) {
		if(e == 0)
			break;
		x = x / 10;
		--e;
	}
	uint32_t tmp_x_large_count = e;
	while(tmp_x_large_count > 8) { // eg. when a = 1.1234567890123
		x = x / 10;
		tmp_x_large_count--;
	}
	while(tmp_x_large_count < e) {
		x = x * 10;
		++tmp_x_large_count;
	}
	while(x > 0 && (x/10 * 10 == x) && e > 0) {
		x = x/10;
		e--;
	}
	auto sign = a.sign;
	if(e > 8) {
		x = 0;
		e = 0;
		sign = true;
	}
	SafeNumber n;
	n.valid = true;
	n.sign = sign;
	n.x = x;
	n.e = e;
	return n;
}

SafeNumber safe_number_create(bool sign, uint64_t x, uint32_t e) {
	SafeNumber n;
	n.valid = true;
	n.sign = sign;
	n.x = x;
	n.e = e;
	return compress_number(n);
}


const SafeNumber sn_0 = safe_number_zero();
const SafeNumber sn_1 = safe_number_create(true, 1, 0);
const SafeNumber sn_nan = safe_number_create_invalid();

SafeNumber safe_number_create(const std::string& str) {
	// xxxxx.xxxxx to SafeNumber
	auto str_size = str.size();
	auto str_chars = str.c_str();
	if (0 == str_size) {
		return sn_nan;
	}
	if (str_size > 40) {
		return sn_nan;
	}
	if (str == NaN_str) {
		return sn_nan;
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
				return sn_nan;
			}
			pos++;
			after_dot = true;
			continue;
		}
		x = 10 * x + c_int;
		if (after_dot) {
			e++;
		}
		pos++;
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
	uint64_t ax = a.x * uint64_pow(10, max_e - a.e); // a = sign * (x*100) * 10^-(e+2=max_e) if max_e - a.e = 2
	uint64_t bx = b.x * uint64_pow(10, max_e - b.e);
	int64_t result_sign_x = ax * (a.sign ? 1 : -1) + bx * (b.sign ? 1 : -1);
	auto result_sign = result_sign_x >= 0;
	uint64_t result_x = result_sign ? static_cast<uint64_t>(result_sign_x) : static_cast<uint64_t>(-result_sign_x);
	auto result_e = max_e;
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
		return sn_0;
	bool result_sign = a.sign == b.sign;
	auto result_e = a.e + b.e;
	auto ax = simple_uint128_create(0, a.x);
	auto bx = simple_uint128_create(0, b.x);
	auto abx = simple_uint128_multi(ax, bx); // a.x * b.x
	auto uint128_10 = simple_uint128_create(0, 10);
	while(abx.big>0) { // when abx is overflow as uint64_t format
		if(result_e==0) {
			break;
		}
		abx = simple_uint128_divmod(abx, uint128_10).first; // abx = abx/10
		--result_e;
	}
	auto result_x = abx.low; // throw abx's overflow upper part
	return safe_number_create(result_sign, result_x, result_e);
}

// a / b = a.x / b.x * 10^(b-a)
SafeNumber safe_number_div(const SafeNumber& a, const SafeNumber& b) {
	if (!safe_number_is_valid(a) || !safe_number_is_valid(b)) {
		return a;
	}
	if (safe_number_is_zero(a))
		return sn_0;
	if (safe_number_is_zero(b))
		return sn_nan;
	auto sign = a.sign == b.sign;
	int32_t extra_e = a.e-b.e; // r.e need to add extra_e. if extra_e < -r.e then r.x = r.x * 10^(-extra_e - r.e) and r.e = 0
	SimpleUint128 big_a = simple_uint128_create(0, a.x);
	SimpleUint128 big_b = simple_uint128_create(0, b.x);
    // The resulting fractional form is big_a/big_b
	// Approximate the score out of the decimal number
	// Let the result be SafeNumber r, initial value is r.x = 0, r.e = -1
	uint64_t rx = 0; // r.x
	int32_t re = -1; // r.e
	// r.x = r.x* 10 + big_a/big_b, big_a = (big_a % big_b) * 10, r.e += 1, Repeat this step until r.e >= 8 or big_a == 0 or rx > largest_x
	do {
		const auto& divmod = simple_uint128_divmod(big_a, big_b);
		rx = rx * 10 + divmod.first.low; // ignore overflowed value
		big_a = simple_uint128_multi(divmod.second, uint128_10);
		++re;
	}while(re<16 && rx < largest_x && (big_a.big || big_a.low));
	if(extra_e>=-static_cast<int32_t>(re)) {
		re += extra_e;
	} else {
		rx = rx * uint64_pow(10, -extra_e - re);
		re = 0;
	}
	return safe_number_create(sign, rx, static_cast<uint32_t>(re));
}

// tostring(a)
std::string safe_number_to_string(const SafeNumber& a) {
	const auto& val = compress_number(a);
	if (!safe_number_is_valid(val))
		return NaN_str;
	if (safe_number_is_zero(val))
		return "0";
	std::stringstream ss;
	if (!val.sign) {
		ss << "-";
	}
	// x = p.q
	auto e10 = uint64_pow(10, val.e);
	auto p = static_cast<uint64_t>(val.x / e10);
	auto q = val.x % e10;
	ss << p;
	if(q>0) {
		ss << "." << q;
	}
	return ss.str();
}
