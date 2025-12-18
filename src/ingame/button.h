#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <functional>
#include <memory>
#include "../ui/text.h"

class Button {
private:
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texNormal = nullptr;
    SDL_Texture* texOnClick = nullptr;
    SDL_Rect rect{0,0,0,0};
    bool clicked = false;

    std::unique_ptr<Text> label;
    std::function<void()> onClick;

    // label position as percentage of available free space:
    // 0.0 = left/top, 0.5 = center, 1.0 = right/bottom
    float labelRelX = 0.5f;
    float labelRelY = 0.5f;

    SDL_Texture* loadTexture(const std::string& p1);

    // update label pixel pos from rect + labelRelX/labelRelY
    void updateLabelPosition();

public:
    Button(SDL_Renderer* renderer = nullptr);
    ~Button();

    // Create button using stage name; will try both "asset/..." and "assets/..." paths
    bool create(SDL_Renderer* renderer,
                int x, int y, int w, int h,
                const std::string& text = "", int fontSize = 16,
                SDL_Color textColor = {255,255,255,255},
                const std::string& fontPath = "asset/font.ttf");

    // position / size / text
    void setPosition(int x, int y);
    void setSize(int w, int h);

    // accessors for current position
    int getX() const;
    int getY() const;
    // measured image size (if created from texture)
    int getWidth() const;
    int getHeight() const;
    // set label anchor inside button (percent of available space: 0..1)
    void setLabelPositionPercent(float relX, float relY);
    void setText(const std::string& text);
    void setTextFontSize(int size);
    void setTextColor(SDL_Color col);

    // event handling and rendering
    void handleEvent(const SDL_Event& e);
    void render();

    // callback
    void setCallback(std::function<void()> cb);

    // allow wrapping the label inside the button (0 = no wrap)
    void setLabelWrapWidth(int w);

    // cleanup resources
    void cleanup();
};