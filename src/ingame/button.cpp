#include "button.h"
#include <iostream>

Button::Button(SDL_Renderer* renderer) : renderer(renderer) {}
Button::~Button() { cleanup(); }

SDL_Texture* Button::loadTexture(const std::string& path) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    if (!tex)
        std::cerr << "Failed to load: " << path << " | " << SDL_GetError() << std::endl;
    return tex;
}

bool Button::create(SDL_Renderer* rend,
                    int x, int y, int w, int h,
                    const std::string& text, int fontSize,
                    SDL_Color textColor, const std::string& fontPath)
{
    cleanup();
    renderer = rend;
    if (!renderer) { std::cerr << "Button::create - no renderer\n"; return false; }

    texNormal = loadTexture("assets/images/button/button_normal.png");
    texOnClick = loadTexture("assets/images/button/button_onClick.png");

    rect.x = x; rect.y = y;

    // if caller passed zero width or height, derive size from normal texture
    rect.w = w;
    rect.h = h;
    if ((rect.w == 0 || rect.h == 0) && texNormal) {
        float tw = 0.0f, th = 0.0f;
        if (SDL_GetTextureSize(texNormal, &tw, &th)) {
            if (rect.w == 0) rect.w = static_cast<int>(tw);
            if (rect.h == 0) rect.h = static_cast<int>(th);
        }
    }

    if (!text.empty()) {
        label = std::make_unique<Text>(renderer);
        if (!label->create(renderer, fontPath, fontSize, text, textColor)) {
            label.reset();
            // not fatal — button works with image only
        }
    }

    // ensure label positioned correctly with current rect and rel anchors
    updateLabelPosition();

    return true;
}

int Button::getWidth() const { return rect.w; }
int Button::getHeight() const { return rect.h; }
int Button::getX() const { return rect.x; }
int Button::getY() const { return rect.y; }

void Button::setText(const std::string& text) {
    if (!label) label = std::make_unique<Text>(renderer);
    label->setText(text);
    updateLabelPosition();
}
void Button::setTextFontSize(int size) {
    if (!label) return;
    label->setFontSize(size);
    updateLabelPosition();
}
void Button::setTextColor(SDL_Color col) {
    if (!label) return;
    label->setColor(col);
    updateLabelPosition();
}

void Button::setLabelPositionPercent(float relX, float relY)
{
    if (relX < 0.0f) relX = 0.0f;
    if (relX > 1.0f) relX = 1.0f;
    if (relY < 0.0f) relY = 0.0f;
    if (relY > 1.0f) relY = 1.0f;
    labelRelX = relX;
    labelRelY = relY;
    updateLabelPosition();
}

void Button::updateLabelPosition()
{
    if (!label) return;
    // position label within button similar to Panel alignment:
    // x = rect.x + (rect.w - labelW) * relX
    // y = rect.y + (rect.h - labelH) * relY
    int lw = label->getWidth();
    int lh = label->getHeight();
    int lx = rect.x + static_cast<int>((rect.w - lw) * labelRelX);
    int ly = rect.y + static_cast<int>((rect.h - lh) * labelRelY);
    label->setPosition(lx, ly);
}

void Button::handleEvent(const SDL_Event& e)
{
    if (!renderer) return;

    // Only care about mouse events
    if (e.type != SDL_EVENT_MOUSE_BUTTON_DOWN && e.type != SDL_EVENT_MOUSE_BUTTON_UP && e.type != SDL_EVENT_MOUSE_MOTION)
        return;

    // Normalize mouse coords by renderer scale so logical rect matches physical events
    float scaleX = 1.0f, scaleY = 1.0f;
    SDL_GetRenderScale(renderer, &scaleX, &scaleY);

    int mx = 0, my = 0;
    if (e.type == SDL_EVENT_MOUSE_MOTION) {
        mx = static_cast<int>(e.motion.x / scaleX);
        my = static_cast<int>(e.motion.y / scaleY);
    } else {
        mx = static_cast<int>(e.button.x / scaleX);
        my = static_cast<int>(e.button.y / scaleY);
    }

    bool inside = (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h);

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
        clicked = inside;
        std::cerr << "Button::handleEvent - MOUSE_DOWN inside=" << inside << " rect=("
                  << rect.x << "," << rect.y << "," << rect.w << "," << rect.h << ")\n";
    } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
        std::cerr << "Button::handleEvent - MOUSE_UP inside=" << inside << " clicked=" << clicked << "\n";
        if (clicked && inside) {
            std::cerr << "Button::handleEvent - invoking callback onClick set=" << (onClick ? "yes" : "no") << "\n";
            // visual feedback then callback
            if (texOnClick) {
                clicked = true;
                render();
                SDL_RenderPresent(renderer);
                SDL_Delay(100);
                clicked = false;
            } else {
                SDL_Delay(100);
            }
            if (onClick) onClick();
        }
        clicked = false;
    }
}

void Button::render()
{
    if (!renderer) return;
    SDL_Texture* use = (clicked && texOnClick) ? texOnClick : texNormal;
    if (use) {
        SDL_FRect dst = { (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };
        SDL_RenderTexture(renderer, use, NULL, &dst);
    } else {
        // fallback: draw a simple rectangle to indicate button area
        SDL_SetRenderDrawColor(renderer, clicked ? 200 : 120, 120, 120, 255);
        SDL_FRect dst = { (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };
        SDL_RenderFillRect(renderer, &dst);
    }

    if (label) {
        // ensure label uses current rel anchors
        updateLabelPosition();
        label->render();
    }
}

void Button::setCallback(std::function<void()> cb) { onClick = std::move(cb); }

// new: forward wrap width to label (if Text supports it)
void Button::setLabelWrapWidth(int w)
{
    if (!label) return;
    // Text API expected: setWrapWidth(int). If your Text class uses a different name,
    // adapt this call accordingly.
    // Example assumed method:
    // label->setWrapWidth(w);
    // If Text doesn't have wrap method, consider implementing wrap support in Text.
    #if 1
    // try to call via known symbol; replace with actual API if different
    label->setWrapWidth(w);
    #endif
}

void Button::cleanup()
{
    if (texNormal) { SDL_DestroyTexture(texNormal); texNormal = nullptr; }
    if (texOnClick)  { SDL_DestroyTexture(texOnClick);  texOnClick = nullptr; }
    if (label) { label->cleanup(); label.reset(); }
}

void Button::setPosition(int x, int y)
{
    rect.x = x;
    rect.y = y;
    if (label) {
        updateLabelPosition();
    }
}

void Button::setSize(int w, int h)
{
    rect.w = w;
    rect.h = h;
    if (label) {
        updateLabelPosition();
    }
}