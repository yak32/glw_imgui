#ifndef _TEXTURE_ATLAS_
#define _TEXTURE_ATLAS_

#include <vector>

class TextureAtlas{
struct shelf_t{
	unsigned int y;
	unsigned int height;
	unsigned int available_width;
};

public:
	TextureAtlas();
	bool create(unsigned int width, unsigned int height);
	bool add_box(unsigned int width, unsigned int height, unsigned int* x, unsigned int* y);
	bool remove_box(unsigned int x, unsigned int y);
protected:
	std::vector<shelf_t> _shelves;
	unsigned int _width, _height;
	unsigned int _shelves_height;
};

#endif //_TEXTURE_ATLAS_