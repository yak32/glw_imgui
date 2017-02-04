#include "texture_atlas.h"
#include <algorithm>

TextureAtlas::TextureAtlas(): _width(0), _height(0), _shelves_height(0){
}

bool TextureAtlas::create(unsigned int width, unsigned int height){
	_shelves.reserve(height/5);
	_width = width;
	_height = height;
	return true;
}

bool TextureAtlas::add_box(unsigned int width, unsigned int height, unsigned int* x, unsigned int* y){
	if (width > _width || height > _height)
		return false;

	// let's find appropriate shelf
	shelf_t* best = nullptr;
	for(auto& s: _shelves){
		if (width <= s.available_width && s.height >= height){
			best = &s;
			break;
		}
	}
	if (best == nullptr){
		// new shelf
		shelf_t s;
		s.height = height;
		s.available_width = _width - width;
		s.y = _shelves_height;

		if (_shelves_height + height > _height)
			return false; // no place

		*x = 0;
		*y = _shelves_height;

		_shelves_height += height;
		auto pos = std::lower_bound(_shelves.begin(), _shelves.end(), s, [width, height]
				(const shelf_t& left, const shelf_t& right) {
					return left.height < right.height;
				});
		_shelves.insert(pos, s);
	}
	else{
		// reuse existing shelf
		*x = _width - best->available_width;
		*y = best->y;
		best->available_width -= width;
	}
	return true;
}

bool TextureAtlas::remove_box(unsigned int x, unsigned int y){
	return true;
}
