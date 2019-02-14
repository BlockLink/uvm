#include <cbor_diff/cbor_diff_tests.h>
#include <cborcpp/cbor.h>
#include <cbor_diff/cbor_diff.h>
#include <jsondiff/jsondiff.h>
#include <iostream>

namespace cbor_diff {

	using namespace std;
	using namespace cbor;
	using namespace cbor_diff;

	void test_cbor_diff() {
		{
			double a = 1.2345678901;
			auto json_encoded = jsondiff::json_dumps(a);
			auto a_encoded = cbor_encode(CborObject::from_float64(a));
			auto json_decoded = jsondiff::json_loads(json_encoded).as_double();
			auto a_decoded = cbor_decode(a_encoded);
			auto cbor_decoded_value = a_decoded->as_float64();
			printf("a: %.10f, a_encoded: %s, json_encoded: %s, json_decoded: %.10f, a_decoded: %.10f", a, a_encoded, json_encoded, json_decoded, cbor_decoded_value);
		}
		{
			CborDiff differ;
			auto a = CborObject::from_int(123);
			CborMapValue b_map;
			b_map["a"] = CborObject::from_bool(true);
			auto hello = CborObject::from_string("hello");
			b_map["b"] = hello;
			auto b = CborObject::from(b_map);
			auto c = CborObject::from(1.23456789);
			auto origin = cbor_encode(a);
			auto result = cbor_encode(b);
			auto c_encoded = cbor_encode(c);
			auto a_decoded = cbor_decode(origin);
			auto b_decoded = cbor_decode(result);
			auto c_decoded = cbor_decode(c_encoded);
			cout << "origin: " << origin << " result: " << result << " c: " << c << endl;
			cout << "a_decoded: " << a_decoded->as_int() << endl;
			cout << "c_decoded: " << c_decoded->as_float64() << endl;
			auto diff_result = differ.diff_by_hex(std::string(origin), std::string(result));
			auto diff_result_str = diff_result->pretty_str();
			std::cout << diff_result_str << std::endl;
			auto patched = differ.patch_by_string(origin, diff_result);
			std::cout << "patched: " << patched->str() << std::endl;
			assert(cbor_encode(patched) == cbor_encode(cbor_decode(result)));
		}
		{
			CborDiff differ;
			auto a = CborObject::from_int(123);
			CborMapValue b_map;
			b_map["a"] = CborObject::from_bool(true);
			auto hello = CborObject::from_string("hello");
			b_map["b"] = hello;
			auto b = CborObject::from(b_map);
			auto null_val = CborObject::create_null();
			auto origin = null_val;
			auto result = b;
			auto diff_result = differ.diff(origin, result);
			auto diff_result_str = diff_result->pretty_str();
			std::cout << diff_result_str << std::endl;
			auto patched = differ.patch(origin, diff_result);
			std::cout << "patched: " << patched->str() << std::endl;
			assert(cbor_encode(patched) == cbor_encode(result));

			auto rollbacked = differ.rollback(result, diff_result);
			std::cout << "rollbacked: " << cbor_encode(rollbacked) << std::endl;
			assert(cbor_encode(rollbacked) == cbor_encode(origin));
		}
		{
			CborDiff differ;
			auto origin = CborObject::create_map({
				{ "foo", CborObject::from_int(42) },
				{ "bar", CborObject::from_int(100) },
				{ "boz", CborObject::create_array({
				CborObject::from_int(1), CborObject::from_int(2),CborObject::from_int(3), CborObject::from_int(4), CborObject::from_int(5), CborObject::from_int(6)
			}) },
			{ "array", CborObject::create_array({
				CborObject::from_int(1), CborObject::from_int(2),CborObject::from_int(3), CborObject::from_int(4), CborObject::from_int(5), CborObject::from_int(6)
			}) },
			{ "fubar", CborObject::create_map({
				{ "kaboom", CborObject::create_map({
					{ "note", CborObject::from_string("We're running dangerously low on metasyntatic variables here") },
					{ "afoo", CborObject::create_map({
						{ "abar", CborObject::from_string("raba") },
						{ "aboz", CborObject::from_string("zoba") },
						{ "afubar", CborObject::from_string("rabufa") }
			}) },
			{ "akaboom", CborObject::from_int(200) }
			}) }
			}) }
			});
			auto result = CborObject::create_map({
				{ "foo", CborObject::from_int(42) },
				{ "bar", CborObject::from_int(100) },
				{ "boz", CborObject::create_map({
					{ "a", CborObject::create_array({
				CborObject::from_int(1), CborObject::from_int(2),CborObject::from_int(3)
			}) }
			}) },
			{ "array", CborObject::create_array({
				CborObject::from_int(0), CborObject::from_int(1), CborObject::from_int(3), CborObject::from_int(4),CborObject::from_int(6),CborObject::from_int(7), CborObject::from_int(5)
			}) },
			{ "fubar", CborObject::create_map({
				{ "kaboom", CborObject::create_map({
					{ "note", CborObject::from_string("We're running dangerously low on metasyntatic variables here") },
					{ "afoo", CborObject::create_map({
						{ "abar", CborObject::from_string("raba") },
						{ "aboz", CborObject::from_string("zozoba") },
						{ "afubar", CborObject::from_string("rabufa") },
						{ "c", CborObject::from_int(123) }
			}) },
			{ "akaboom", CborObject::from_int(200) }
			}) }
			}) }
			});
			auto diff_result = differ.diff(origin, result);
			auto diff_result_str = diff_result->pretty_str();
			auto diff_result_pretty_str = diff_result->pretty_diff_str();
			std::cout << "diff: " << std::endl << diff_result_str << std::endl;
			std::cout << "diff pretty: " << std::endl << diff_result_pretty_str << std::endl;
			auto patched = differ.patch(origin, diff_result);
			std::cout << "patched: " << patched->str() << std::endl;
			assert(cbor_encode(patched) == cbor_encode(result));

			auto rollbacked = differ.rollback(result, diff_result);
			std::cout << "rollbacked: " << rollbacked->str() << std::endl;
			assert(cbor_encode(rollbacked) == cbor_encode(origin));
		}
		{
			// test big int and big double
			auto a = CborObject::from_extra_integer(6000000000, true);
			auto b = CborObject::from(1.23456789);
			const auto& a_json_str = cbor_encode(a);
			const auto& b_json_str = cbor_encode(b);
			auto a_loaded = cbor_decode(a_json_str);
			auto b_loaded = cbor_decode(b_json_str);
			auto a_type = a_loaded->object_type();
			assert(a_loaded->is_extra_int() && a_loaded->as_extra_int() == a->as_extra_int());
			assert(b_loaded->is_float() && abs(b_loaded->as_float64() - b->as_float64()) < 0.0001);
			std::cout << "big int and big double tests passed" << std::endl;
		}
	}
}