# glw_imgui - C++ IMGUI implementation
Immediate Mode UI C++ implementation.

IMGUI is a code-driven GUI system.

### Examples
```c++

// simple button
if (gui.button("Cancel"))
	do_cancel_action();

```

### Gallery
[![ScreenShot](/../feature-screenshots/screenshots/IMGUI_toolbars.png)](https://www.youtube.com/watch?v=TlJiuguyLVo)

### Example
```c++
#include "glw_json.h"

using namespace json;

struct vec3{
  float x,y,z;
};

template<typename T>
bool serialize(T& t, vec3& v){
	SERIALIZE(x);
	SERIALIZE(y);
	SERIALIZE(z);
	return true;
}

int main(){

	vec3 v;
	if (load_object_from_file("test.json", v) != JSON_OK)
		return 0;

	if (save_object_to_file("test2.json", v) != JSON_OK)
		return 0;

	return 1;
}
```

To avoid hash maps with allocations, values should be serialized in alphabetical order.
BTW, realization with hash map was 2x times slower (for avarage sized json object with around 50 properties)
With Sublime Text 2/3: select everything inside serialize() and press F9 - it will sort properties automatically.


## FAQ

The license?
This library is in the public domain. You can do what you want, but I would appreciate attribution.

### Dependencies
 STL, c-runtime.

### Performance
JSON loading - allocation-free parsing, N Log(N) performance where N is number of properties in JSON object.
   Parsing speed is comparable to RapidJSON, saving is slower.

JSON saving - linear performance, std::ofstream is used to save data ( some allocations )

### Tests
 Run cmake on CMakeLists.txt in tests folder. Tests require C++11 support to compile (glw_json.h doesn't).

### Compartibility
  Tested with Visual Studio 2013, 2015, GCC 4.3.6 to GCC 7.0 (HEAD), Clang 3.0 to 3.9.0 (trunc), compartible with C++98, C++11 and C++14.

### More examples
```c++

```

