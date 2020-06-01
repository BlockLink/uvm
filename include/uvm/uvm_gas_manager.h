#pragma once
#include <stdint.h>

struct lua_State;

namespace uvm
{
    namespace lua
    {
        namespace lib
        {

			class GasManager
			{
			private:
				lua_State* _L;
			public:
				GasManager(lua_State* L);
				void add_gas(int64_t gas);
				int64_t gas() const;
				int64_t* gas_ref() const;
				int64_t* gas_ref_or_new();
			};

        }
    }
}
