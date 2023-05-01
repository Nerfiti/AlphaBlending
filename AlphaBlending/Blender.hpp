#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

struct Blender
{
    const char  *img_name        = "img.png";
    const char  *background_name = "background.jpg";
    unsigned int xPos            = 0;
    unsigned int yPos            = 0;
};


void    GetBlendParams  (const int argc, const char *argv[], Blender *blender);
int     CheckParams     (Blender *blender, sf::Image *img, sf::Image *back);
void    CheckEvents     (sf::RenderWindow *window);
int     AlphaBlending   (Blender *blender);

void Blend(const sf::Uint8 *img, const sf::Uint8 *back, int imgSizeX, int imgSizeY, 
            int backSizeX, int xPos, int yPos, sf::Uint8 *result);

void OptimizeBlend(const sf::Uint8 *img, const sf::Uint8 *back, int imgSizeX, int imgSizeY, int backSizeX,
                int xPos, int yPos, sf::Uint8 *result);