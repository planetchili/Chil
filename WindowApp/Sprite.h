#pragma once
#include <Core/src/gfx/d12/SpriteBatcher.h>

class IAnimation
{
public:
	// step time
	// animation state information should be stored in sprite instance
	// Animation (or sprite definition) can give an opaque pointer to the sprite to keep info (std::any, or final class and cast)
};

class ISpriteDefinition;

class ISprite;