/*
** $Id: ltable.c,v 2.117 2015/11/19 19:16:22 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/

#define ltable_cpp
#define LUA_CORE

#include <uvm/lprefix.h>


/*
** Implementation of tables (aka arrays, objects, or hash tables).
** Tables keep its elements in two parts: an array part and a hash part.
** Non-negative integer keys are all candidates to be kept in the array
** part. The actual size of the array is the largest 'n' such that
** more than half the slots between 1 and n are in use.
** Hash uses a mix of chained scatter table with Brent's variation.
** A main invariant of these tables is that, if an element is not
** in its main position (i.e. the 'original' position that its hash gives
** to it), then the colliding element is in its own main position.
** Hence even when the load factor reaches 100%, performance remains good.
*/

#include <math.h>
#include <limits.h>

#include <map>
#include <vector>

#include <uvm/lua.h>

#include <uvm/ldebug.h>
#include <uvm/ldo.h>
#include <uvm/lgc.h>
#include <uvm/lmem.h>
#include <uvm/lobject.h>
#include <uvm/lstate.h>
#include <uvm/lstring.h>
#include <uvm/ltable.h>
#include <uvm/lvm.h>
#include <uvm/uvm_lutil.h>
#include <uvm/uvm_api.h>


/*
** Maximum size of array part (MAXASIZE) is 2^MAXABITS. MAXABITS is
** the largest integer such that MAXASIZE fits in an unsigned int.
*/
#define MAXABITS	cast_int(sizeof(int) * CHAR_BIT - 1)
#define MAXASIZE	(1u << MAXABITS)

/*
** Maximum size of hash part is 2^MAXHBITS. MAXHBITS is the largest
** integer such that 2^MAXHBITS fits in a signed int. (Note that the
** maximum number of elements in a table, 2^MAXABITS + 2^MAXHBITS, still
** fits comfortably in an unsigned int.)
*/
#define MAXHBITS	(MAXABITS - 1)


#define hashpow2(t,n)		(gnode(t, lmod((n), sizenode(t))))

#define hashstr(t,str)		hashpow2(t, (str)->hash())
#define hashboolean(t,p)	hashpow2(t, p)
#define hashint(t,i)		hashpow2(t, i)


/*
** for some types, it is better to avoid modulus by power of 2, as
** they tend to have many 2 factors.
*/
#define hashmod(t,n)	(gnode(t, ((n) % ((sizenode(t)-1)|1))))


#define hashpointer(t,p)	hashmod(t, point2uint(p))


/*
** Hash for floating-point numbers.
** The main computation should be just
**     n = frexp(n, &i); return (n * INT_MAX) + i
** but there are some numerical subtleties.
** In a two-complement representation, INT_MAX does not has an exact
** representation as a float, but INT_MIN does; because the absolute
** value of 'frexp' is smaller than 1 (unless 'n' is inf/NaN), the
** absolute value of the product 'frexp * -INT_MIN' is smaller or equal
** to INT_MAX. Next, the use of 'unsigned int' avoids overflows when
** adding 'i'; the use of '~u' (instead of '-u') avoids problems with
** INT_MIN.
*/
#if !defined(l_hashfloat)
static int l_hashfloat(lua_Number n) {
    int i;
    lua_Integer ni;
    n = l_mathop(frexp)(n, &i) * -cast_num(INT_MIN);
    if (!lua_numbertointeger(n, &ni)) {  /* is 'n' inf/-inf/NaN? */
        lua_assert(luai_numisnan(n) || l_mathop(fabs)(n) == cast_num(HUGE_VAL));
        return 0;
    }
    else {  /* normal case */
        unsigned int u = lua_cast(unsigned int, i) + lua_cast(unsigned int, ni);
        return cast_int(u <= lua_cast(unsigned int, INT_MAX) ? u : ~u);
    }
}
#endif

/*
** returns the index for 'key' if 'key' is an appropriate key to live in
** the array part of the table, 0 otherwise.
*/
static unsigned int arrayindex(const TValue *key) {
    if (ttisinteger(key)) {
        lua_Integer k = ivalue(key);
        if (0 < k && (lua_Unsigned)k <= MAXASIZE)
            return lua_cast(unsigned int, k);  /* 'key' is an appropriate array index */
    }
    return 0;  /* 'key' did not match some condition */
}

static std::string lua_value_to_str(const TValue *key)
{
	if (ttisstring(key))
	{
		return std::string(svalue(key));
	}
	else if (ttisinteger(key))
	{
		return std::to_string((lua_Integer)nvalue(key));
	}
	else if (ttisnumber(key))
	{
		return std::to_string(nvalue(key));
	}
	else
		return "";
}


static bool val_to_table_key(const TValue* key, std::string& out) {
	if (ttisnil(key)) {
		return false;
	}
	else if (ttisfloat(key)) {
		lua_Integer k;
		if (luaV_tointeger(key, &k, 0)) {
			out = std::to_string(k);
			return true;
		}
		else {
			return false;
		}
	}
	else if (ttisinteger(key)) {
		lua_Integer k;
		if (luaV_tointeger(key, &k, 0)) {
			out = std::to_string(k);
			return true;
		}
		else {
			return false;
		}
	}
	else if (ttisstring(key)) {
		out = getstr(gco2ts(key->value_.gco));
		return true;
	}
	else {
		return false;
	}
}


static unsigned int findindex_of_sorted_table(lua_State *L, uvm_types::GcTable *t, StkId key) {
	unsigned int i = arrayindex(key);
	if (i != 0 && i <= t->sizearray)  /* is 'key' inside array part? */
		return i;  /* yes; that's the index */
	else {
		std::string key_str;
		if (!val_to_table_key(key, key_str)) {
			return 0;
		}
		int k=0;
		for (auto iter = t->entries.begin(); iter != t->entries.end(); iter++) {
			k++;
			std::string iter_key_str;
			if(val_to_table_key(&iter->first, iter_key_str) && iter_key_str == key_str)
				return k + t->sizearray;
		}
		return 0;
	}
}


int luaH_next(lua_State *L, uvm_types::GcTable *t, StkId key) {
    unsigned int i = findindex_of_sorted_table(L, t, key);  /* find original element */
    if (nullptr == t)
        return 0;
	
    for (; i < t->sizearray; i++) {  /* try first array part */
        if (!ttisnil(&t->array[i])) {  /* a non-nil value? */
            setivalue(key, i + 1);
            setobj2s(L, key + 1, &t->array[i]);
            return 1;
        }
    }

	auto i_in_hash_part = i - t->sizearray;
	int k = 0;
	for (auto it = t->entries.begin(); it != t->entries.end(); it++) {
		k++;
		if (k >= i_in_hash_part + 1) {
			if (!ttisnil(&it->second)) {  // a non-nil value?
				setobj2s(L, key, &it->first);
				setobj2s(L, key + 1, &it->second);
				return 1;
			}
		}
	}
	return 0;
}


/*
** {=============================================================
** Rehash
** ==============================================================
*/

/*
** Compute the optimal size for the array part of table 't'. 'nums' is a
** "count array" where 'nums[i]' is the number of integers in the table
** between 2^(i - 1) + 1 and 2^i. 'pna' enters with the total number of
** integer keys in the table and leaves with the number of keys that
** will go to the array part; return the optimal size.
*/
static unsigned int computesizes(unsigned int nums[], unsigned int *pna) {
    int i;
    unsigned int twotoi;  /* 2^i (candidate for optimal size) */
    unsigned int a = 0;  /* number of elements smaller than 2^i */
    unsigned int na = 0;  /* number of elements to go to array part */
    unsigned int optimal = 0;  /* optimal size for array part */
    /* loop while keys can fill more than half of total size */
    for (i = 0, twotoi = 1; *pna > twotoi / 2; i++, twotoi *= 2) {
        if (nums[i] > 0) {
            a += nums[i];
            if (a > twotoi / 2) {  /* more than half elements present? */
                optimal = twotoi;  /* optimal size (till now) */
                na = a;  /* all elements up to 'optimal' will go to array part */
            }
        }
    }
    lua_assert((optimal == 0 || optimal / 2 < na) && na <= optimal);
    *pna = na;
    return optimal;
}


static int countint(const TValue *key, unsigned int *nums) {
    unsigned int k = arrayindex(key);
    if (k != 0) {  /* is 'key' an appropriate array index? */
        nums[luaO_ceillog2(k)]++;  /* count as such */
        return 1;
    }
    else
        return 0;
}

void luaH_resize(lua_State *L, uvm_types::GcTable *t, unsigned int nasize,
    unsigned int nhsize) {
    unsigned int i;
    int j;
    unsigned int oldasize = t->array.size();
	if (nasize > oldasize)  /* array part must grow? */
	{
		for (auto i = 0; i < nasize - oldasize; i++) {
			t->array.push_back(*luaO_nilobject);
		}
		t->sizearray = nasize;
	}
	else {
		t->array.resize(nasize);
		t->sizearray = nasize;
	}
}


void luaH_resizearray(lua_State *L, uvm_types::GcTable *t, unsigned int nasize) {
	int nsize = t->entries.size();
    luaH_resize(L, t, nasize, nsize);
}


/*
** }=============================================================
*/


uvm_types::GcTable *luaH_new(lua_State *L) {
	auto o = L->gc_state->gc_new_object<uvm_types::GcTable>();
    return o;
}


void luaH_free(lua_State *L, uvm_types::GcTable *t) {
    luaM_free(L, t);
}

/*
** inserts a new key into a hash table; first, check whether key's main
** position is free. If not, check whether colliding node is in its main
** position or not: if it is not, move colliding node to an empty place and
** put new key in its main position; otherwise (colliding node is in its main
** position), new key goes to an empty position.
*/
TValue *luaH_newkey(lua_State *L, uvm_types::GcTable *t, const TValue *key) {
    TValue aux;
    if (ttisnil(key)) luaG_runerror(L, "table index is nil");
    else if (ttisfloat(key)) {
        lua_Integer k;
        if (luaV_tointeger(key, &k, 0)) {  /* index is int? */
            setivalue(&aux, k);
            key = &aux;  /* insert it as an integer */
        }
        else if (luai_numisnan(fltvalue(key)))
            luaG_runerror(L, "table index is NaN");
	}
	TValue key_obj(*key);
	t->entries[key_obj] = *luaO_nilobject;
	for (auto& p : t->entries) {
		std::string p_key_str;
		std::string key_str;
		if (ttislightuserdata(&p.first) && ttislightuserdata(key) && p.first.value_.p == key->value_.p) {
			return &p.second;
		}
		if (val_to_table_key(&p.first, p_key_str) && val_to_table_key(key, key_str) && p_key_str == key_str)
			return &p.second;
	}
}


/*
** search function for integers
*/
const TValue *luaH_getint(uvm_types::GcTable *t, lua_Integer key) {
    /* (1 <= key && key <= t->sizearray) */
    if (l_castS2U(key) - 1 < t->sizearray)
        return &t->array.at(key - 1);
    else {
		const auto& key_str = std::to_string(key);
		for (const auto& p : t->entries) {
			std::string p_key_str;
			if (val_to_table_key(&p.first, p_key_str) && p_key_str == key_str)
				return &p.second;
		}
		return luaO_nilobject;
    }
}


/*
** search function for short strings
*/
const TValue *luaH_getshortstr(uvm_types::GcTable *t, uvm_types::GcString *key) {
	std::string key_str = key->value;
	for (auto& p : t->entries) {
		std::string p_key_str;
		if (val_to_table_key(&p.first, p_key_str) && p_key_str == key_str)
			return &p.second;
	}
	return luaO_nilobject;
}


/*
** "Generic" get version. (Not that generic: not valid for integers,
** which may be in array part, nor for floats with integral values.)
*/
static const TValue *getgeneric(uvm_types::GcTable *t, const TValue *key) {
	std::string key_str;
	if (!val_to_table_key(key, key_str)) {
		return luaO_nilobject;
	}
	for (auto& p : t->entries) {
		std::string p_key_str;
		if (val_to_table_key(&p.first, p_key_str) && p_key_str == key_str)
			return &p.second;
	}
	return luaO_nilobject;
}


const TValue *luaH_getstr(uvm_types::GcTable *t, uvm_types::GcString *key) {
	if (key->tt == LUA_TSHRSTR)
		return luaH_getshortstr(t, key);
    else {  /* for long strings, use generic case */
        TValue ko;
        setsvalue(lua_cast(lua_State *, nullptr), &ko, key);
        return getgeneric(t, &ko);
    }
}


/*
** main search function
*/
const TValue *luaH_get(uvm_types::GcTable *t, const TValue *key) {
    switch (ttype(key)) {
    case LUA_TSHRSTR: return luaH_getshortstr(t, tsvalue(key));
	case LUA_TLNGSTR: return luaH_getstr(t, tsvalue(key));
    case LUA_TNUMINT: return luaH_getint(t, ivalue(key));
    case LUA_TNIL: return luaO_nilobject;
    case LUA_TNUMFLT: {
        lua_Integer k;
        if (luaV_tointeger(key, &k, 0)) /* index is int? */
            return luaH_getint(t, k);  /* use specialized version */
        /* else... */
    }  /* FALLTHROUGH */
    default:
        return getgeneric(t, key);
    }
}


/*
** beware: when using this function you probably need to check a GC
** barrier and invalidate the TM cache.
*/
TValue *luaH_set(lua_State *L, uvm_types::GcTable *t, const TValue *key) {
    const TValue *p = luaH_get(t, key);
    if (p != luaO_nilobject)
        return lua_cast(TValue *, p);
    else return luaH_newkey(L, t, key);
}


void luaH_setint(lua_State *L, uvm_types::GcTable *t, lua_Integer key, TValue *value) {
    const TValue *p = luaH_getint(t, key);
	// TODO: sizearray change?
    TValue *cell;
    if (p != luaO_nilobject)
        cell = lua_cast(TValue *, p);
    else {
        TValue k;
        setivalue(&k, key);
        cell = luaH_newkey(L, t, &k);
    }
    setobj2t(L, cell, value);
}


static int unbound_search(uvm_types::GcTable *t, unsigned int j) {
    unsigned int i = j;  /* i is zero or a present index */
    j++;
    /* find 'i' and 'j' such that i is present and j is not */
    while (!ttisnil(luaH_getint(t, j))) {
        i = j;
        if (j > lua_cast(unsigned int, MAX_INT) / 2) {  /* overflow? */
            /* table was built with bad purposes: resort to linear search */
            i = 1;
            while (!ttisnil(luaH_getint(t, i))) i++;
            return i - 1;
        }
        j *= 2;
    }
    /* now do a binary search between them */
    while (j - i > 1) {
        unsigned int m = (i + j) / 2;
        if (ttisnil(luaH_getint(t, m))) j = m;
        else i = m;
    }
    return i;
}


/*
** Try to find a boundary in table 't'. A 'boundary' is an integer index
** such that t[i] is non-nil and t[i+1] is nil (and 0 if t[1] is nil).
*/
int luaH_getn(uvm_types::GcTable *t) {
    unsigned int j = t->sizearray;
    if (j > 0 && ttisnil(&t->array.at(j - 1))) {
        /* there is a boundary in the array part: (binary) search for it */
        unsigned int i = 0;
        while (j - i > 1) {
            unsigned int m = (i + j) / 2;
            if (ttisnil(&t->array.at(m - 1))) j = m;
            else i = m;
        }
        return i;
    }
    /* else must find a boundary in hash part */
    else if (t->entries.empty())  /* hash part is empty? */
        return j;  /* that is easy... */
    else return unbound_search(t, j);
}



#if defined(LUA_DEBUG)

Node *luaH_mainposition(const Table *t, const TValue *key) {
    return mainposition(t, key);
}

int luaH_isdummy(Node *n) { return isdummy(n); }

#endif
