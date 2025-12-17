#include "stages.h"
#include <iostream>
#include <cmath>

Stages::Stages(SDL_Renderer* renderer) : renderer(renderer) {}
Stages::~Stages() { /* ...cleanup... */ }

bool Stages::init(SDL_Renderer* rend, User* user_, int w, int h, std::function<void(char)> onSelectCb)
{
    if (!rend) return false;
    renderer = rend;
    user = user_;
    onSelect = std::move(onSelectCb);
    winW = w; winH = h;

    // load three stage preview textures: assets/images/background/background1_1.png .. background3_1.png
    for (int i = 0; i < 3; ++i) {
        std::string path = "assets/images/background/background" +  std::to_string(i + 1) + ".png";
        tex[i] = IMG_LoadTexture(renderer, path.c_str());
        if (!tex[i]) {
            std::cerr << "Stages::init - failed load " << path << " | " << SDL_GetError() << "\n";
        }
    }

    // prepare question mark text
    qmarkText.create(renderer, "assets/font.ttf", 72, "?", {255,255,255,255});

    // Set selected stage dựa trên stage hiện tại của user
    if (user) {
        char stageChar = user->getStage();
        if (stageChar >= '1' && stageChar <= '3') {
            selected = static_cast<int>(stageChar - '1'); // '1' -> 0, '2' -> 1, '3' -> 2
        } else {
            selected = 0; // mặc định stage 1 nếu stage không hợp lệ
        }
    } else {
        selected = 0; // không có user thì mặc định stage 1
    }
    
    prevSelected = selected;
    slideAnim = 0.0f;
    sliding = false;

    return true;
}

void Stages::startSlideTo(int newIndex)
{
    if (newIndex < 0) newIndex = 0;
    if (newIndex > 2) newIndex = 2;
    if (newIndex == selected) return;
    prevSelected = selected;
    selected = newIndex;
    slideAnim = 0.0f;
    sliding = true;
    slideStart = SDL_GetTicks();
}

void Stages::handleEvent(const SDL_Event& e)
{
    if (!renderer) return;

    if (e.type == SDL_EVENT_KEY_DOWN) {
        if (e.key.key == SDLK_LEFT) {
            int target = selected - 1;
            if (target >= 0) startSlideTo(target);
        } else if (e.key.key == SDLK_RIGHT) {
            int target = selected + 1;
            if (target <= 2) startSlideTo(target);
        }
        return;
    }

    // Normalize mouse coords by renderer scale (Start sets SDL_SetRenderScale on resize)
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

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        SDL_FRect centerDst = computeDstForIndex(selected);
        if (mx >= centerDst.x && mx < centerDst.x + centerDst.w && my >= centerDst.y && my < centerDst.y + centerDst.h) {
            if (isIndexUnlocked(selected)) {
                if (onSelect) {
                    char s = static_cast<char>('1' + selected);
                    onSelect(s);
                }
            }
        }
    }
}

void Stages::update()
{
    if (!sliding) return;
    Uint32 now = SDL_GetTicks();
    Uint32 dt = now - slideStart;
    if (dt >= slideDur) {
        slideAnim = 1.0f;
        sliding = false;
    } else {
        slideAnim = static_cast<float>(dt) / static_cast<float>(slideDur);
        // ease out
        slideAnim = 1.0f - std::pow(1.0f - slideAnim, 3);
    }
}

SDL_FRect Stages::computeDstForIndex(int idx, float extraShift) const
{
    // center of window
    float cx = winW * 0.5f;
    float cy = winH * 0.5f;

    // compute logical offset in "slots" from selected index, using animation blending between prevSelected->selected
    float animProgress = slideAnim;
    float blendedSelected = static_cast<float>(prevSelected) * (1.0f - animProgress) + static_cast<float>(selected) * animProgress;

    // difference from blended center
    float slot = static_cast<float>(idx) - blendedSelected;
    // position X = center + slot*(thumb width + gap)
    float baseGap = static_cast<float>(thumbW + gap);
    float xCenter = cx + slot * baseGap + extraShift;
    // choose size: if idx equals current selected use center size, else thumbnail size
    bool isCenter = (idx == selected && !sliding) || (idx == prevSelected && sliding && slideAnim < 0.5f) || (idx == selected && sliding && slideAnim >= 0.5f);
    float w = isCenter ? static_cast<float>(centerW) : static_cast<float>(thumbW);
    float h = isCenter ? static_cast<float>(centerH) : static_cast<float>(thumbH);

    SDL_FRect dst;
    dst.w = w;
    dst.h = h;
    dst.x = xCenter - w * 0.5f;
    dst.y = cy - h * 0.5f;
    return dst;
}

bool Stages::isIndexUnlocked(int idx) const
{
    if (!user) return false;
    
    char u = user->getStage(); // e.g. '0','1','2','3'
    
    // Stage 1 (idx 0) luôn unlock (dù chưa chơi hay đã chơi)
    if (idx == 0) {
        return true;
    }
    
    // Stage 2 (idx 1) unlock nếu đã thắng stage 1 (stage >= '2')
    // Stage 3 (idx 2) unlock nếu đã thắng stage 2 (stage >= '3')
    char requiredStage = static_cast<char>('1' + idx); // idx 1 -> '2', idx 2 -> '3'
    return u >= requiredStage;
}

void Stages::render()
{
    if (!renderer) return;
    // update animation progress
    update();

    // draw three previews: loop i=0..2
    for (int i = 0; i < 3; ++i) {
        SDL_FRect dst = computeDstForIndex(i);
        // choose appearance
        if (isIndexUnlocked(i)) {
            int unlockedStage = static_cast<int>(user->getStage() - '0'); // numeric: 1,2,3
            if (unlockedStage >= i + 1) {
                SDL_RenderTexture(renderer, tex[i], nullptr, &dst);
            }
            if (unlockedStage == i) {
                 // simulate blur/fade: render texture dimmed
                // modulate color to desaturate (grayish) and alpha
                SDL_SetTextureBlendMode(tex[i], SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(tex[i], 200);
                SDL_SetTextureColorMod(tex[i], 180, 180, 180);
                SDL_RenderTexture(renderer, tex[i], nullptr, &dst);
                // reset mods (best effort)
                SDL_SetTextureAlphaMod(tex[i], 255);
                SDL_SetTextureColorMod(tex[i], 255, 255, 255);
        }
        } else {
                // locked: render box with "?" centered and border (double-render)
                SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
                SDL_RenderFillRect(renderer, &dst);
                // double-render border: two offset rects
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                SDL_FRect r1 = dst; r1.x -= 2; r1.y -= 2; r1.w += 4; r1.h += 4;
                SDL_RenderRect(renderer, &r1);
                SDL_FRect r2 = dst; r2.x += 2; r2.y += 2; r2.w -= 4; r2.h -= 4;
                SDL_RenderRect(renderer, &r2);
                // render "?" text centered
                int qx = static_cast<int>(dst.x + dst.w * 0.5f - qmarkText.getWidth() * 0.5f);
                int qy = static_cast<int>(dst.y + dst.h * 0.5f - qmarkText.getHeight() * 0.5f);
                qmarkText.setPosition(qx, qy);
                qmarkText.render();
            }
        // draw a subtle border for all thumbnails (double render)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_FRect border1 = dst; border1.x -= 1; border1.y -= 1; border1.w += 2; border1.h += 2;
        SDL_RenderRect(renderer, &border1);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
        SDL_FRect border2 = dst; border2.x += 1; border2.y += 1; border2.w -= 2; border2.h -= 2;
        SDL_RenderRect(renderer, &border2);
    }

    // highlight center with a larger border glow
    SDL_FRect centerDst = computeDstForIndex(selected);
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 160);
    SDL_FRect glow = centerDst; glow.x -= 4; glow.y -= 4; glow.w += 8; glow.h += 8;
    SDL_RenderRect(renderer, &glow);
}