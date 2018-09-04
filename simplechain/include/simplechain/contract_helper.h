#pragma once
#include <simplechain/config.h>

namespace simplechain {
	class ContractHelper
	{
	public:
		static int common_fread_int(FILE* fp, int* dst_int);
		static int common_fwrite_int(FILE* fp, const int* src_int);
		static int common_fwrite_stream(FILE* fp, const void* src_stream, int len);
		static int common_fread_octets(FILE* fp, void* dst_stream, int len);
		static std::string to_printable_hex(unsigned char chr);
		static int save_code_to_file(const std::string& name, UvmModuleByteStream *stream, char* err_msg);
		static uvm::blockchain::Code load_contract_from_file(const fc::path &path);
	};
}