#pragma once
#include <vector>
#include <string>
#include <map>
#include <uvm/uvm_storage_value.h>

enum UvmTypeInfoEnum
{
	LTI_OBJECT = 0,
	LTI_NIL = 1,
	LTI_STRING = 2,
	LTI_INT = 3,
	LTI_NUMBER = 4,
	LTI_BOOL = 5,
	LTI_TABLE = 6,
	LTI_FUNCTION = 7, // coroutine as function type
	LTI_UNION = 8,
	LTI_RECORD = 9, // , type <RecordName> = { <name> : <type> , ... }
	LTI_GENERIC = 10, // £¬
	LTI_ARRAY = 11, // £¬
	LTI_MAP = 12, // £¬£¬key
	LTI_LITERIAL_TYPE = 13, // £¬literal type //union£¬: "Male" | "Female"
	LTI_STREAM = 14, // Stream£¬
	LTI_UNDEFINED = 100 // £¬undefined
};

class UvmModuleByteStream {
public:
    bool is_bytes;
    std::vector<char> buff;
    std::vector<std::string> contract_apis;
    std::vector<std::string> offline_apis;
    std::vector<std::string> contract_emit_events;
    std::string contract_id;
    std::string contract_name;
    int  contract_level;
    int  contract_state;
    // storage
    std::map<std::string, uvm::blockchain::StorageValueTypes> contract_storage_properties;

    // API args
    std::map<std::string, std::vector<UvmTypeInfoEnum>> contract_api_arg_types;

public:
    UvmModuleByteStream();
    virtual ~UvmModuleByteStream();
};

typedef UvmModuleByteStream* UvmModuleByteStreamP;
