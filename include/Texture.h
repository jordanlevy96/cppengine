#pragma once

class Texture
{
public:
    Texture(char *filepath);
    char *filepath;
    int width, height, numChannels;
    unsigned int texture;
};