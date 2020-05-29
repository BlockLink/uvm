#ifndef uvm_error_h
#define uvm_error_h

#define lerror_set(L, error, error_format, ...) do {			 \
     if (nullptr != error && strlen(error) < 1)					\
     {						\
        char error_msg[LUA_COMPILE_ERROR_MAX_LENGTH+1];		 \
        memset(error_msg, 0x0, sizeof(error_msg));		     \
        snprintf(error_msg, LUA_COMPILE_ERROR_MAX_LENGTH, error_format, ##__VA_ARGS__);				\
        error_msg[LUA_COMPILE_ERROR_MAX_LENGTH-1] = '\0';       \
        memcpy(error, error_msg, sizeof(char)*(1 + strlen(error_msg)));								\
     }												\
     global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, error_format, ##__VA_ARGS__);		\
} while(0)

#define lcompile_error_set(L, error, error_format, ...) do {	   \
    lerror_set(L, error, error_format, ##__VA_ARGS__);		   \
    if (strlen(L->compile_error) < 1)						  \
    {														   \
        snprintf(L->compile_error, LUA_COMPILE_ERROR_MAX_LENGTH-1, error_format, ##__VA_ARGS__);			\
    }															\
} while(0)

#define lcompile_error_get(L, error) {					   \
    if (error && strlen(error)<1 && strlen(L->compile_error) > 0)			\
    {															   \
        memcpy(error, L->compile_error, sizeof(char) * (strlen(L->compile_error) + 1));			\
    } else if(nullptr != error && strlen(error)<1 && strlen(L->runerror) > 0)			   \
	{			\
		 memcpy(error, L->runerror, sizeof(char) * (strlen(L->runerror) + 1));			\
			}		\
}

#define lmalloc_error(L) do { \
		global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "malloc memory error"); \
	} while(0)

#endif // !uvm_error_h
