// glw_json.h - v0.1 - json parser for C/C++ - public domain
// Iakov Sumygin, May 2016

#ifndef _GLW_JSON_
#define _GLW_JSON_

#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include <algorithm>
#include <limits.h>
#include <string.h>
#include <cmath>
#include <assert.h>

// maximum count of properties per any JSON object
#define MAX_JSON_PROPS 300
#define SERIALIZE(prop) t.process(#prop, v.prop)
#define GLW_UNUSED(x) (void)x
#ifndef LOG_ERROR
	#define LOG_ERROR(...)
#endif
namespace json {

const char quote = '\"';

const int JSON_OK = 0;
const int JSON_FAIL = -1;
enum JSON_OPTIONS {
	RESET_IF_ABSENT = 1, // if property is absent, reset variable to 0
	ERROR_IF_ABSENT = 2  // if property is absent, return error
};

// struct to hold refs to name-values pairs from json file
struct string_ref {
	const char* str;
	size_t len;
};
struct property {
	string_ref param, value;
};
typedef property prop_map[MAX_JSON_PROPS];

#define SERIALIZE_NAME (out << '\"' << name << "\": ")

// **************************************** loading JSON ***********************************
inline const char* skip_ws(const char* in) {
	while (*in && ::isspace(*in)) in++;
	return in;
}
inline const char* search_ws(const char* in) {
	while (*in && !::isspace(*in)) in++;
	return in;
}
// end of property
inline const char* search_eop(const char* in) {
	while (*in && *in != ',' && *in != '}' && *in != ']' && !isspace(*in)) in++;
	return in;
}
inline const char* search_eov(const char* in) {
	while (*in && *in != ',' && *in != ']' && !isspace(*in)) in++;
	return in;
}
inline const char* search_obj_end(const char* in, char open_brace, char close_brace) {
	int opened = 1;
	while (*in) {
		if (*in == close_brace)
			--opened;
		else if (*in == open_brace)
			++opened;
		if (!opened)
			return in + 1; // success - we found closing
		++in;
	}
	return in - 1; // error - end of file
}
inline const char* find_ch(const char* in, char ch) {
	while (*in && *in != ch) in++;
	return in;
}
inline size_t get_value_len(const char* in) {
	if (*in == '{') {
		// special case - for object as value, value len includes whitespace after the object
		const char* brace = search_obj_end(in + 1, '{', '}'); // embedded object
		return skip_ws(brace) - in;
	}
	if (*in == '[')
		return search_obj_end(in + 1, '[', ']') - in; // embedded array
	return (*in == quote ? find_ch(in + 1, quote) + 1 : search_eop(in)) - in;
}
inline const char* load_props(const char* in, const char* end, prop_map& props,
							  size_t& props_size) {
	in = skip_ws(in);
	if (*in == '}')
		return in; // special case - empty json

	props_size = 0;
	while (*in) {
		in = skip_ws(in);
		if (*in != quote)
			return in - 1; // error
		// parse value name
		property p;
		p.param.str = in + 1;
		p.param.len = find_ch(p.param.str, quote) - p.param.str;
		in = p.param.str + p.param.len;
		if (!*in)
			return in - 1;
		in = skip_ws(in + 1);
		if (*in != ':')
			return in - 1; // error

		// parse value
		in = skip_ws(in + 1);
		p.value.str = in;
		p.value.len = get_value_len(in);
		in += p.value.len;
		if (in >= end)
			return end - 1; // error - value is not closed properly

		in = skip_ws(in);

		// register name-value pair
		if (props_size >= MAX_JSON_PROPS - 1)
			return in - 1; // not enough space - error
		props[props_size++] = p;

		if (*in != ',') {
			if (*in != '}')
				return in - 1; // error
			return in;		   // success - next symbol after brace
		}
		++in;
	}
	return in - 1; // error
}
inline bool operator<(const string_ref& s1, const string_ref& s2) {
	size_t i = 0;
	while (i < s1.len && i < s2.len) {
		if (s1.str[i] < s2.str[i])
			return true;
		if (s2.str[i] < s1.str[i])
			return false;
		++i;
	}
	return s1.len < s2.len;
}
inline bool compare_props(const property& p1, const property& p2) {
	return p1.param < p2.param;
}
inline const char* load(const char* str, size_t len, int& v, int options) {
	GLW_UNUSED(options);
	char* pEnd;
	errno = 0;
	long int l = strtol(str, &pEnd, 10);
	if (pEnd < str + len)
		l = strtol(str, &pEnd, 16);
	v = (int)l;
	return (pEnd < str + len) || (errno == ERANGE) ? str : pEnd;
}
inline const char* load(const char* str, size_t len, unsigned int& v, int options) {
	GLW_UNUSED(options);
	char* pEnd;
	errno = 0;
	unsigned long l = strtoul(str, &pEnd, 10);
	v = (unsigned int)l;
	return errno != ERANGE && pEnd == str + len ? pEnd : str;
}
template <typename c, typename t, typename a>
const char* load(const char* str, size_t len, std::basic_string<c, t, a>& v, int options) {
	GLW_UNUSED(options);
	if (len < 2)
		return str; // it's not a string
	v.assign(str + 1, len - 2);
	return str + len;
}
inline const char* load(const char* str, size_t len, float& v, int options) {
	GLW_UNUSED(options);
	char* pEnd;
	v = strtof(str, &pEnd);
	if (v == HUGE_VAL || v == -HUGE_VAL)
		return str;

	return pEnd < str + len ? str : str + len;
}
inline const char* load(const char* str, size_t len, bool& v, int options) {
	GLW_UNUSED(options);
	if (len == 4 && !strncmp(str, "true", len))
		v = true;
	else if (len == 5 && !strncmp(str, "false", len))
		v = false;
	else
		return str;
	return str + len;
}
inline int get_error_line(const char* in, const char* pos) {
	int lines = 0;
	in = skip_ws(in);
	while (*in && in < pos) {
		if (*in == '\n')
			++lines;
		++in;
	}
	return lines + 1;
}
template <typename V> const char* load(const char* in, size_t len, V& t, int options);

// overrides for embedded objects and pointers to object
template <typename V> const char* load(const char* obj_start, size_t len, V*& value, int options) {
	if (len == 4 && strncmp(obj_start, "null", len) == 0){
		value = nullptr;
		return obj_start + len;
	}
	value = new V;
	return load(obj_start, len, *value, options);
}
template <typename containter_type>
const char* load_container(const char* in, size_t len, containter_type& t, int options) {
	GLW_UNUSED(len);
	if (*in != '[')
		return in - 1;

	std::insert_iterator<containter_type> it(t, t.end());
	typename containter_type::value_type v;
	do {
		in = skip_ws(in + 1);
		size_t len = get_value_len(in);
		if (load(in, len, v, options) != in + len)
			return in - 1;
		in += len;
		it = v;
		in = skip_ws(in);
		if (*in != ',') {
			if (*in != ']')
				return in - 1;
			else {
				++in;
				break; // end of array
			}
			++in;
		}
	} while (true);
	return in; // success
}
template <typename T, typename A>
const char* load(const char* in, size_t len, std::vector<T, A>& t, int options) {
	return load_container(in, len, t, options);
}
struct LoadObject {
	LoadObject(const prop_map& _props, size_t _props_size, const char* _start, int _options)
		: props(_props), props_size(_props_size), current(0), options(_options), error_pos(NULL),
		  start(_start), _prev_name("") {}
	const prop_map& props;
	size_t props_size;
	size_t current;
	int options;
	const char* error_pos;
	const char* start;
	const char* _prev_name;
	template <typename V> bool process(const char* name, V& value) {
#ifndef NDEBUG
		// this check could fail if name is dynamically allocated and was freed after usage
		// if Sublime Text 3 is used, select everything inside serialize() and press F9
		if (strcmp(_prev_name, name) >= 0)
			assert(false && "variables should be serialized in alphabetical order");
		_prev_name = name;
#endif
		if (current < props_size &&
			strncmp(name, props[current].param.str, props[current].param.len) == 0) {
			const char* end =
				load(props[current].value.str, props[current].value.len, value, options);
			if (end == props[current].value.str + props[current].value.len) {
				++current;
				return true;
			}
			error_pos = end;
		}
		else if (!(options & ERROR_IF_ABSENT)) {
			if (options & RESET_IF_ABSENT)
				value = V();
			return true;
		}
		current = props_size; // skip parsing
		return false;
	}
};
template <typename V> const char* load(const char* in, size_t len, V& t, int options) {
	const char* start = in;
	prop_map props;
	size_t props_size = 0;
	in = skip_ws(in);

	if (*in != '{')
		return in - 1;										 // error
	in = load_props(in + 1, start + len, props, props_size); // load prop map for child object
	if (*in != '}')
		return in - 1; // error

	std::sort(props, props + props_size, compare_props);

	LoadObject serializer(props, props_size, start, options);
	if (!serialize(serializer, t))
		return serializer.error_pos;

	if (serializer.current < serializer.props_size)
		return props[serializer.current].param.str;

	in = skip_ws(in + 1); // skip whitespace after
	return in;
}
template <typename T>
int load_object_from_file(const char* filename, T& t, int options = RESET_IF_ABSENT) {
	std::ifstream is(filename, std::ios::binary);
	if (!is)
		return false;

	is.seekg(0, is.end);
	int length = (int)is.tellg();
	is.seekg(0, is.beg);
	char* buffer = new char[length + 1];
	is.read(buffer, length);
	buffer[length] = 0;
	is.close();
	const char* pos = load(buffer, length + 1, t, options);
	delete[] buffer;
	if (pos != buffer + length) {
		return get_error_line(buffer, pos);
	}
	return JSON_OK;
}
template <typename T>
int load_object_from_string(const char* json_string, T& t, size_t len = (size_t)-1,
							int options = RESET_IF_ABSENT) {
	if (len == (size_t)-1)
		len = strlen(json_string);
	const char* pos = load(json_string, len + 1, t, options);
	if (pos != json_string + len) {
		return get_error_line(json_string, pos); // return error position
	}
	return JSON_OK;
}
// **************************************** saving JSON ***********************************

inline void save(std::ostream& out, int& t, int tabs) {
	GLW_UNUSED(tabs);
	out << t;
}
inline void save(std::ostream& out, unsigned int& t, int tabs) {
	GLW_UNUSED(tabs);
	out << t;
}
inline void save(std::ostream& out, bool& t, int tabs) {
	GLW_UNUSED(tabs);
	out << (t ? "true" : "false");
}
inline void save(std::ostream& out, float& t, int tabs) {
	GLW_UNUSED(tabs);
	out << t;
}
inline void save(std::ostream& out, const char* t, int tabs) {
	GLW_UNUSED(tabs);
	out << quote << t << quote;
}
template <typename c, typename tr, typename a>
void save(std::ostream& out, std::basic_string<c, tr, a>& t, int tabs) {
	GLW_UNUSED(tabs);
	out << quote << t << quote;
}

template <typename T> bool save(std::ostream& out, T& t, int tabs);
template <typename T> bool save(std::ostream& out, T*& t, int tabs){
	if (t)
		return save(out, *t, tabs);
	out << "null";
	return true;
}
template <typename iterator>
bool save_container(std::ostream& out, iterator begin, iterator end, int tabs) {
	out << '[';
	for (iterator i = begin; i != end; ++i) {
		if (i != begin)
			out << ',';
		save(out, *i, tabs + 1);
	}
	out << ']';
	return out.fail();
}
template <typename T, typename A> bool save(std::ostream& out, std::vector<T, A>& t, int tabs) {
	return save_container(out, t.begin(), t.end(), tabs);
}
struct SaveObject {
	std::ostream& out;
	bool first;
	int tabs;
	SaveObject(std::ostream& _out, int _tabs = 1) : out(_out), first(true), tabs(_tabs) {}
	template <typename T> bool process(const char* name, T& t) {
		if (first)
			first = false;
		else
			out << ",\n";
		for (int i = 0; i < tabs; ++i) out << '\t';
		out << quote << name << quote << ": ";
		save(out, t, tabs);
		if (out.fail())
			return false;
		return true;
	}
};
template <typename T> bool save(std::ostream& out, T& t, int tabs) {
	out << "{\n";
	SaveObject serializer(out, tabs + 1);
	serialize(serializer, t); // serialize should be defined for type T
	out << "\n";
	for (int i = 0; i < tabs; ++i) out << '\t';
	out << '}';
	return !out.fail();
}
template <typename T> int save_object_to_stream(T& t, std::ostream& out) {
	if (!json::save(out, t, 0))
		return JSON_FAIL;
	out << "\n";
	return JSON_OK;
}
// serialize single object
template <typename T> int save_object_to_file(const char* filename, T& t) {
	try {
		std::ofstream out(filename);
		return json::save_object_to_stream(t, out);
	}
	catch (std::exception& e) {
		GLW_UNUSED(e);
		LOG_ERROR("saving json (%s) failed, error: %s", filename, e.what());
		return JSON_FAIL;
	}
	return JSON_OK;
}
}

#endif // _GLW_JSON_
