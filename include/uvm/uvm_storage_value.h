#pragma once


enum UvmOutsideObjectTypes
{
    OUTSIDE_STREAM_STORAGE_TYPE = 0
};

typedef enum UvmStorageValueType
{
    LVALUE_INTEGER = 0, LVALUE_NUMBER = 1, LVALUE_TABLE = 2, LVALUE_STRING = 3,
    LVALUE_BOOLEAN = 4, LVALUE_USERDATA = 5, LVALUE_NIL = 6, LVALUE_ARRAY = 7
    , LVALUE_STREAM = 9

    , LVALUE_NOT_SUPPORT = 100
} UvmStorageValueType;


namespace uvm {
    namespace blockchain {

        enum StorageValueTypes
        {
            storage_value_null = 0,
            storage_value_int = 1,
            storage_value_number = 2,
            storage_value_bool = 3,
            storage_value_string = 4,
            storage_value_stream = 5,

            storage_value_unknown_table = 50,
            storage_value_int_table = 51,
            storage_value_number_table = 52,
            storage_value_bool_table = 53,
            storage_value_string_table = 54,
            storage_value_stream_table = 55,

            storage_value_unknown_array = 100,
            storage_value_int_array = 101,
            storage_value_number_array = 102,
            storage_value_bool_array = 103,
            storage_value_string_array = 104,
            storage_value_stream_array = 105,

            storage_value_userdata = 201,
            storage_value_not_support = 202
        };


        inline bool is_null_storage_value_type(StorageValueTypes type)
        {
            return type == StorageValueTypes::storage_value_null;
        }


        inline bool is_any_base_storage_value_type(StorageValueTypes type)
        {
            return ((type >= StorageValueTypes::storage_value_int)
                && (type <= StorageValueTypes::storage_value_stream));
        }

        inline bool is_any_table_storage_value_type(StorageValueTypes type)
        {
            return (type >= StorageValueTypes::storage_value_unknown_table)
                && (type <= StorageValueTypes::storage_value_stream_table);
        }

        inline bool is_any_array_storage_value_type(StorageValueTypes type)
        {
            return (type >= StorageValueTypes::storage_value_unknown_array)
                && (type <= StorageValueTypes::storage_value_stream_array);
        }

        inline StorageValueTypes get_storage_base_type(StorageValueTypes type)
        {
            switch (type)
            {
            case StorageValueTypes::storage_value_bool:
            case StorageValueTypes::storage_value_bool_array:
            case StorageValueTypes::storage_value_bool_table:
                return StorageValueTypes::storage_value_bool;
            case StorageValueTypes::storage_value_int:
            case StorageValueTypes::storage_value_int_array:
            case StorageValueTypes::storage_value_int_table:
                return StorageValueTypes::storage_value_int;
            case StorageValueTypes::storage_value_number:
            case StorageValueTypes::storage_value_number_array:
            case StorageValueTypes::storage_value_number_table:
                return StorageValueTypes::storage_value_number;
            case StorageValueTypes::storage_value_string:
            case StorageValueTypes::storage_value_string_array:
            case StorageValueTypes::storage_value_string_table:
                return StorageValueTypes::storage_value_string;
            case StorageValueTypes::storage_value_stream:
            case StorageValueTypes::storage_value_stream_array:
            case StorageValueTypes::storage_value_stream_table:
                return StorageValueTypes::storage_value_stream;
            default:
                return StorageValueTypes::storage_value_null;
            }
        }

        inline StorageValueTypes get_item_type_in_table_or_array(StorageValueTypes type)
        {
            switch (type)
            {
            case StorageValueTypes::storage_value_int_table:
            case StorageValueTypes::storage_value_int_array:
                return StorageValueTypes::storage_value_int;
            case StorageValueTypes::storage_value_bool_table:
            case StorageValueTypes::storage_value_bool_array:
                return StorageValueTypes::storage_value_bool;
            case StorageValueTypes::storage_value_number_table:
            case StorageValueTypes::storage_value_number_array:
                return StorageValueTypes::storage_value_number;
            case StorageValueTypes::storage_value_string_table:
            case StorageValueTypes::storage_value_string_array:
                return StorageValueTypes::storage_value_string;
            case StorageValueTypes::storage_value_stream_table:
            case StorageValueTypes::storage_value_stream_array:
                return StorageValueTypes::storage_value_stream;
            case StorageValueTypes::storage_value_unknown_table:
            case StorageValueTypes::storage_value_unknown_array:
                return StorageValueTypes::storage_value_null;
            default:
                return StorageValueTypes::storage_value_null;
            }
        }

    }
}


#define lua_storage_is_table(t) (uvm::blockchain::is_any_table_storage_value_type(t)||uvm::blockchain::is_any_array_storage_value_type(t))
#define lua_storage_is_array(t) (uvm::blockchain::is_any_array_storage_value_type(t))
#define lua_storage_is_hashtable(t) (uvm::blockchain::is_any_table_storage_value_type(t))

struct UvmStorageValue;

struct lua_table_binary_function
{	// base class for binary functions
    typedef std::string first_argument_type;
    typedef std::string second_argument_type;
    typedef bool result_type;
};

struct lua_table_less
    : public lua_table_binary_function
{	// functor for operator<
    bool operator()(const std::string& _Left, const std::string& _Right) const
    {	// apply operator< to operands
        if (_Left.length() != _Right.length())
            return _Left.length() < _Right.length();
        return _Left < _Right;
    }
};

typedef std::map<std::string, struct UvmStorageValue, struct lua_table_less> UvmTableMap;

typedef UvmTableMap* UvmTableMapP;

struct UvmStorageChangeItem;


typedef union UvmStorageValueUnion
{
    lua_Integer int_value;
    lua_Number number_value;
    bool bool_value;
    char* string_value;
    UvmTableMapP table_value;
    void* userdata_value;
    void* pointer_value;
} UvmStorageValueUnion;

typedef struct UvmStorageValue
{
    uvm::blockchain::StorageValueTypes type;
    union UvmStorageValueUnion value;
    UvmStorageValue();
    static UvmStorageValue from_int(int val);
    static UvmStorageValue from_string(char* val);
    void try_parse_type(uvm::blockchain::StorageValueTypes new_type);
    void try_parse_to_int_type();
    void try_parse_to_number_type();
    static bool is_same_base_type_with_type_parse(uvm::blockchain::StorageValueTypes type1, uvm::blockchain::StorageValueTypes type2);
    bool equals(UvmStorageValue& other);
} UvmStorageValue;

typedef struct UvmStorageChangeItem
{
    std::string contract_id;
    std::string key;
    jsondiff::DiffResult diff;
    cbor_diff::DiffResult cbor_diff;
    std::string fast_map_key;
    bool is_fast_map = false;
    struct UvmStorageValue before;
    struct UvmStorageValue after;

    std::string full_key() const;

} UvmStorageChangeItem;



typedef std::list<UvmStorageChangeItem> UvmStorageChangeList;

typedef std::list<UvmStorageChangeItem> UvmStorageTableReadList;

struct UvmStorageValue lua_type_to_storage_value_type(lua_State* L, int index);

bool luaL_commit_storage_changes(lua_State* L);

bool lua_push_storage_value(lua_State* L, const UvmStorageValue& value);

UvmStorageValue json_to_uvm_storage_value(lua_State* L, jsondiff::JsonValue json_value);
jsondiff::JsonValue uvm_storage_value_to_json(UvmStorageValue value);

UvmStorageValue cbor_to_uvm_storage_value(lua_State* L, cbor::CborObject* cbor_value);
cbor::CborObjectP uvm_storage_value_to_cbor(UvmStorageValue value);

typedef std::unordered_map<std::string, UvmStorageChangeItem> ContractChangesMap;

typedef std::shared_ptr<ContractChangesMap> ContractChangesMapP;

typedef std::unordered_map<std::string, ContractChangesMapP> AllContractsChangesMap;

