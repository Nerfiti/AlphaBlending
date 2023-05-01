#include <cstdio>
#include <cstdlib>
#include <immintrin.h>

#include "Blender.hpp"

//#define OPTIMIZE

static const int NumberOfTests = 5000;
static const unsigned char ZERO = 255u;

static const __m512i Alpha_mask = _mm512_set_epi8(  ZERO, 62, ZERO, 62, ZERO, 62, ZERO, 62,  
                                                    ZERO, 54, ZERO, 54, ZERO, 54, ZERO, 54,
                                                    ZERO, 46, ZERO, 46, ZERO, 46, ZERO, 46,  
                                                    ZERO, 38, ZERO, 38, ZERO, 38, ZERO, 38,  
                                                    ZERO, 30, ZERO, 30, ZERO, 30, ZERO, 30,  
                                                    ZERO, 22, ZERO, 22, ZERO, 22, ZERO, 22,  
                                                    ZERO, 14, ZERO, 14, ZERO, 14, ZERO, 14,  
                                                    ZERO,  6, ZERO,  6, ZERO,  6, ZERO,  6  );

static const __m512i _255 = _mm512_set1_epi16(255);


void GetBlendParams(const int argc, const char *argv[], Blender *blender)
{
    blender->xPos            = 0;
    blender->yPos            = 0;
    blender->img_name        = "img.png";
    blender->background_name = "background.jpg";

    if (argc < 3)
        return;
    
    blender->xPos = atoi(argv[1]);
    blender->yPos = atoi(argv[2]);

    if (argc < 4)
        return;

    blender->img_name        = argv[3];
    blender->background_name = argv[4];
}

int CheckParams(Blender *blender, sf::Image *img, sf::Image *back)
{
    const unsigned int xPos = blender->xPos;
    const unsigned int yPos = blender->yPos;
    sf::Vector2u imgSize    =  img->getSize();
    sf::Vector2u backSize   = back->getSize();

    if (xPos > backSize.x - imgSize.x || yPos > backSize.y - imgSize.y)
    {
        printf("Invalid image position: (%u:%u)\n", xPos, yPos);
        return -1;
    }
    if (imgSize.x > backSize.x || imgSize.y > backSize.y)
    {
        printf("Invalid images size:\n"
               "Img       - (%u:%u) \n"
               "Backgound - (%u:%u) \n", imgSize.x, imgSize.y, backSize.x, backSize.y);
        return -1;
    }

    return 0;
}

void CheckEvents(sf::RenderWindow *window)
{
    sf::Event event;
    while (window->pollEvent(event))
    {
        if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
            window->close();
    }
}

int AlphaBlending(Blender *blender)
{
    const unsigned int xPos = blender->xPos;
    const unsigned int yPos = blender->yPos;
    
    sf::Image img = {};
    if (!img.loadFromFile(blender->img_name))
        return -1;

    sf::Image background = {};
    if (!background.loadFromFile(blender->background_name))
        return -1;

    if (CheckParams(blender, &img, &background) != 0)
        return -1;

    printf("ImgSize  = (%d:%d)\n"
           "BackSize = (%d:%d)\n", img.getSize().x, img.getSize().y, background.getSize().x, background.getSize().y);

    const sf::Uint8 *image = img.getPixelsPtr();
    const sf::Uint8 *back  = background.getPixelsPtr();

    int backSize = background.getSize().x * background.getSize().y;
    sf::Uint8 *result = (sf::Uint8 *)calloc(backSize << 2, sizeof(sf::Uint8));
    if (!result)
    {
        printf("It's not possible to allocate memory for result array\n");
        return -1;
    }

    for (int i = 0; i < backSize << 2; ++i)
        result[i] = back[i];

    int imgSizeX  = img.getSize().x;
    int imgSizeY  = img.getSize().y;
    int backSizeX = background.getSize().x;
    int backSizeY = background.getSize().y;
    sf::Clock timer = {};
    for (int i = 0; i < NumberOfTests; ++i)
    {
        #ifdef OPTIMIZE
            OptimizeBlend(image, back, imgSizeX, imgSizeY, backSizeX, xPos, yPos, result);
        #else
            Blend(image, back, imgSizeX, imgSizeY, backSizeX, xPos, yPos, result);
        #endif
    }
    printf("Blending: %d times. %d ms.\n", NumberOfTests, timer.getElapsedTime().asMilliseconds());

    sf::RenderWindow window(sf::VideoMode(backSizeX, backSizeY), "Blender");

    while (window.isOpen())
    {
        CheckEvents(&window);

        sf::Texture texture = {};
        texture.create(backSizeX, backSizeY);
        texture.update(result);

        sf::Sprite sprite(texture);

        window.clear();
        window.draw(sprite);
        window.display();
    }

    free(result);
    return 0;
}

void OptimizeBlend(const sf::Uint8 *img, const sf::Uint8 *back, int imgSizeX, int imgSizeY, int backSizeX,
                int xPos, int yPos, sf::Uint8 *result)
{
    int writer = (xPos + (yPos * backSizeX)) << 2;
    int reader = 0;

    int imgSizeBuff = (imgSizeX*imgSizeY) << 2;
    int imgSizeX4   = imgSizeX << 2;

    while (reader < imgSizeBuff)
    {
        __m512i image = _mm512_loadu_si512(img + reader);
        __m512i backg = _mm512_loadu_si512(back + writer);   

        __m512i image_h = _mm512_cvtepu8_epi16(_mm512_extracti32x8_epi32(image, 1)); 
        __m512i image_l = _mm512_cvtepu8_epi16(_mm512_extracti32x8_epi32(image, 0));

        __m512i back_h = _mm512_cvtepu8_epi16(_mm512_extracti32x8_epi32(backg, 1)); 
        __m512i back_l = _mm512_cvtepu8_epi16(_mm512_extracti32x8_epi32(backg, 0)); 

        __m512i alpha_mask = Alpha_mask;

        __m512i alpha_l = _mm512_shuffle_epi8(image_l, alpha_mask);
        __m512i alpha_h = _mm512_shuffle_epi8(image_h, alpha_mask);

        image_l = _mm512_mullo_epi16(image_l, alpha_l);
        image_h = _mm512_mullo_epi16(image_h, alpha_h);

        back_l = _mm512_mullo_epi16(back_l, _mm512_sub_epi16(_255, alpha_l));
        back_h = _mm512_mullo_epi16(back_h, _mm512_sub_epi16(_255, alpha_h));

        __m512i result_l = _mm512_srli_epi16(_mm512_add_epi16(image_l, back_l), 8);
        __m512i result_h = _mm512_srli_epi16(_mm512_add_epi16(image_h, back_h), 8);

        __m256i result_l_small = _mm512_cvtepi16_epi8(result_l);
        __m256i result_h_small = _mm512_cvtepi16_epi8(result_h);

        __m512i colors = _mm512_castsi256_si512(result_l_small);
        colors = _mm512_inserti64x4(colors, result_h_small, 1);
        
        _mm512_storeu_si512(result + writer, colors);

        reader += 1 << 6;
        writer += 1 << 6;
        if ((reader % imgSizeX4) == 0)
            writer += (backSizeX - imgSizeX) << 2;
    }
}

void Blend(const sf::Uint8 *img, const sf::Uint8 *back, int imgSizeX, int imgSizeY, int backSizeX,
                int xPos, int yPos, sf::Uint8 *result)
{
    int writer = (xPos + (yPos * backSizeX)) << 2;
    int reader = 0;

    int imgSizeBuff = (imgSizeX*imgSizeY) << 2;
    int imgSizeX4   = imgSizeX << 2;

    while (reader < imgSizeBuff)
    {
        sf::Uint8 imgR  = img [reader    ];
        sf::Uint8 imgG  = img [reader + 1];
        sf::Uint8 imgB  = img [reader + 2];

        sf::Uint8 Alpha = img [reader + 3];
        reader += 4;
        
        sf::Uint8 backR = back[writer    ];
        sf::Uint8 backG = back[writer + 1];
        sf::Uint8 backB = back[writer + 2];
        
        result[writer    ] = ((imgR - backR)*Alpha + (backR << 8) - backR) >> 8;
        result[writer + 1] = ((imgG - backG)*Alpha + (backG << 8) - backG) >> 8;
        result[writer + 2] = ((imgB - backB)*Alpha + (backB << 8) - backB) >> 8;
        writer += 4;

        if ((reader % imgSizeX4) == 0)
        {
            writer += (backSizeX - imgSizeX) << 2;
        }
    }
}