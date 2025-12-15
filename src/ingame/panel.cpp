#include "panel.h"
#include <iostream>
#include "../functions.h"
#include "../game.h"
#include "textbox.h"

Panel::Panel(SDL_Renderer* renderer) : renderer(renderer) {}
Panel::~Panel() { cleanup(); }

bool Panel::create(SDL_Renderer* rend, int px, int py, int pw, int ph)
{
    cleanup();
    renderer = rend;
    if (!renderer) return false;
    x = px; y = py; w = pw; h = ph;
    return true;
}

void Panel::cleanup()
{
    // free any owned resources
    children.clear();
    if (bgTexture) {
        SDL_DestroyTexture(bgTexture);
        bgTexture = nullptr;
    }
}

void Panel::setPosition(int px, int py) { x = px; y = py; }

bool Panel::setBackgroundFromFile(const std::string& path)
{
    if (!renderer) return false;
    SDL_Texture* t = IMG_LoadTexture(renderer, path.c_str());
    if (!t) {
        std::cerr << "Panel::setBackgroundFromFile failed to load " << path << " | " << SDL_GetError() << "\n";
        return false;
    }

    float texW = 0.0f, texH = 0.0f;
    if (!SDL_GetTextureSize(t, &texW, &texH)) {
        std::cerr << "Panel::setBackgroundFromFile failed to get texture size " << path << " | " << SDL_GetError() << "\n";
        SDL_DestroyTexture(t);
        return false;
    }
    
    if (bgTexture) SDL_DestroyTexture(bgTexture);
    bgTexture = t;

    // set panel size to the image's size only if panel size is not already set.
    // This allows callers to create a panel at a specific size (e.g. 1750x900)
    // and have the background stretched to that size rather than forcing the
    // panel to the image's native dimensions.
    if (w == 0 && h == 0) {
        w = static_cast<int>(texW);
        h = static_cast<int>(texH);
    }

    return true;
}

void Panel::clearBackground()
{
    if (bgTexture) {
        SDL_DestroyTexture(bgTexture);
        bgTexture = nullptr;
    }
}

Button* Panel::addButton(int localX, int localY, int bw, int bh,
                         const std::string& text, int fontSize,
                         SDL_Color textColor, const std::string& fontPath,
                         HAlign halign, VAlign valign)
{
    if (!renderer) return nullptr;
    Child c;
    c.type = Child::Type::Button;
    c.localX = localX;
    c.localY = localY;
    c.w = bw; c.h = bh;
    c.halign = halign; c.valign = valign;
    c.button = std::make_unique<Button>(renderer);
    // Button::create will attempt to load images for the given stage string.
    // Passing bw/bh==0 lets the Button measure its size from the normal texture.
    c.button->create(renderer, 0, 0, bw, bh, text, fontSize, textColor, fontPath);

    // If button measured its own size, update child width/height for layout.
    int measuredW = c.button->getWidth();
    int measuredH = c.button->getHeight();
    if (measuredW > 0) c.w = measuredW;
    if (measuredH > 0) c.h = measuredH;

    children.push_back(std::move(c));
    return children.back().button.get();
}

Text* Panel::addText(const std::string& fontPath, int fontSize,
                     const std::string& textStr, SDL_Color color,
                     int localX, int localY, HAlign halign, VAlign valign)
{
    if (!renderer) return nullptr;
    Child c;
    c.type = Child::Type::Text;
    c.localX = localX;
    c.localY = localY;
    c.halign = halign; c.valign = valign;
    c.text = std::make_unique<Text>(renderer);
    if (!c.text->create(renderer, fontPath, fontSize, textStr, color)) {
        // If creating text fails, still keep placeholder but return null
        children.push_back(std::move(c));
        return children.back().text.get();
    }
    // store measured w/h for layout
    c.w = c.text->getWidth();
    c.h = c.text->getHeight();
    children.push_back(std::move(c));
    return children.back().text.get();
}

void Panel::addImage(SDL_Texture* tex, int localX, int localY, int iw, int ih, HAlign halign, VAlign valign)
{
    if (!tex) return;
    Child c;
    c.type = Child::Type::Image;
    c.image = tex;
    c.localX = localX; c.localY = localY; c.w = iw; c.h = ih;
    c.halign = halign; c.valign = valign;
    children.push_back(std::move(c));
}

Textbox* Panel::addTextbox(int localX, int localY, int w, int h,
                           const std::string& bgPath,
                           const std::string& placeholderText,
                           int fontSize, SDL_Color textColor, const std::string& fontPath,
                           HAlign halign, VAlign valign)
{
    if (!renderer) return nullptr;
    Child c;
    c.type = Child::Type::Textbox;
    c.localX = localX;
    c.localY = localY;
    c.w = w; c.h = h;
    c.halign = halign; c.valign = valign;
    c.textbox = std::make_unique<Textbox>(renderer);
    if (!c.textbox->create(renderer, 0, 0, w, h, bgPath, placeholderText, fontSize, textColor, fontPath)) {
        std::cerr << "Panel::addTextbox - failed to create textbox\n";
        return nullptr;
    }
    children.push_back(std::move(c));
    return children.back().textbox.get();
}

SDL_FRect Panel::computeChildDst(const Child& c) const
{
    float absX = static_cast<float>(x);
    float absY = static_cast<float>(y);
    // compute anchor point from panel + alignment
    float baseX = absX;
    float baseY = absY;
    switch (c.halign) {
        case HAlign::Left:   baseX += static_cast<float>(c.localX); break;
        case HAlign::Center: baseX += (w - c.w) * 0.5f + static_cast<float>(c.localX); break;
        case HAlign::Right:  baseX += static_cast<float>(w - c.w - c.localX); break;
    }
    switch (c.valign) {
        case VAlign::Top:    baseY += static_cast<float>(c.localY); break;
        case VAlign::Middle: baseY += (h - c.h) * 0.5f + static_cast<float>(c.localY); break;
        case VAlign::Bottom: baseY += static_cast<float>(h - c.h - c.localY); break;
    }
    SDL_FRect dst = { baseX, baseY, static_cast<float>(c.w), static_cast<float>(c.h) };
    return dst;
}

void Panel::handleEvent(const SDL_Event& e)
{
    // Always forward keyboard/text events and mouse events to textboxes,
    // because textboxes need to react to keyboard input even if the event
    // isn't a mouse event.
    // First, update textbox positions/sizes and forward the event.
    for (auto &c : children) {
        if (c.type == Child::Type::Textbox && c.textbox) {
            SDL_FRect dst = computeChildDst(c);
            c.textbox->setPosition(static_cast<int>(dst.x), static_cast<int>(dst.y));
            c.textbox->setSize(static_cast<int>(dst.w), static_cast<int>(dst.h));
            c.textbox->handleEvent(e); // forward ALL events (mouse + keyboard + text)
        }
    }

    // Now handle mouse-only behavior for clicking buttons
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP ||
        e.type == SDL_EVENT_MOUSE_MOTION || e.type == SDL_EVENT_MOUSE_WHEEL)
    {
        int mx = 0, my = 0;
        if (e.type == SDL_EVENT_MOUSE_MOTION) { mx = e.motion.x; my = e.motion.y; }
        else { mx = e.button.x; my = e.button.y; }

        auto inside = [&](int mx_, int my_)->bool {
            return mx_ >= x && mx_ < x + w && my_ >= y && my_ < y + h;
        };
        if (!inside(mx, my)) return;

        // forward mouse to the first button under the cursor (topmost)
        for (auto &c : children) {
            if (c.type != Child::Type::Button) continue;
            SDL_FRect dst = computeChildDst(c);
            bool overChild = (mx >= dst.x && mx < dst.x + dst.w && my >= dst.y && my < dst.y + dst.h);
            if (overChild) {
                c.button->setPosition(static_cast<int>(dst.x), static_cast<int>(dst.y));
                c.button->setSize(static_cast<int>(dst.w), static_cast<int>(dst.h));
                c.button->handleEvent(e);
                return;
            }
        }
    }
}

void Panel::render()
{
    if (!renderer) return;

    // draw background (stretched to panel size)
    if (bgTexture) {
        SDL_FRect dst = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) };
        SDL_RenderTexture(renderer, bgTexture, nullptr, &dst);
    }

    // render children
    for (const auto &c : children) {
        SDL_FRect dst = computeChildDst(c);
        switch (c.type) {
            case Child::Type::Button:
                if (c.button) {
                    c.button->setPosition(static_cast<int>(dst.x), static_cast<int>(dst.y));
                    c.button->setSize(static_cast<int>(dst.w), static_cast<int>(dst.h));
                    c.button->render();
                }
                break;

            case Child::Type::Text:
                if (c.text) {
                    c.text->setPosition(static_cast<int>(dst.x), static_cast<int>(dst.y));
                    c.text->render();
                }
                break;

            case Child::Type::Image:
                if (c.image) {
                    SDL_RenderTexture(renderer, c.image, nullptr, &dst);
                }
                break;
            
            case Child::Type::Textbox:
                if (c.textbox) {
                    c.textbox->setPosition(static_cast<int>(dst.x), static_cast<int>(dst.y));
                    c.textbox->setSize(static_cast<int>(dst.w), static_cast<int>(dst.h));
                    c.textbox->render();
                }
                break;
        }
    }
}

int Panel::getX() const { return x; }
int Panel::getY() const { return y; }
int Panel::getWidth() const { return w; }
int Panel::getHeight() const { return h; }

IngamePanel::IngamePanel(SDL_Renderer* renderer) : Panel(renderer) {}

bool IngamePanel::initForStage(Game* owner,
                               int winW, int mapPxW, int winH, int mapPxH)
{
    // load background
    std::string panelPath = "assets/images/panel/ingamePanel.png";
    if (!setBackgroundFromFile(panelPath)) return false;

    // compute position like 
    int panelX = winW - mapPxW - getWidth() - ((winW - mapPxW) * 10 / 100);
    int panelY = (winH - mapPxH) / 2 + (mapPxH - getHeight());
    setPosition(panelX, panelY);

    // add title image centered near top (3% down), size 300x200
    SDL_Texture* titleTex = IMG_LoadTexture(renderer, "assets/images/title.png");
    if (titleTex) {
        int yText = static_cast<int>(getHeight() * 0.03f);
        addImage(titleTex, 0, yText, 300, 200, HAlign::Center, VAlign::Top);
    } else {
        std::cerr << "IngamePanel::initForStage failed to load title image: " << SDL_GetError() << "\n";
    }

    // add buttons (centered, stacked with 16px padding)
    SDL_Color btnCol = { 0xf9, 0xf2, 0x6a, 0xFF };
    const int padding = 16;
    const int widthBtn = 350;
    const int heightBtn = 85;

    int yUndo = static_cast<int>(getHeight() * 0.35f);
    Button* undoBtn = addButton(0, yUndo, widthBtn, heightBtn, "UNDO", 72, btnCol, "assets/font.ttf", HAlign::Center, VAlign::Top);
    if (undoBtn) {
        undoBtn->setLabelPositionPercent(0.5f, 0.70f);
        if (owner) undoBtn->setCallback([owner]() { undo(owner); });
    }

    int yRedo = yUndo + (undoBtn ? undoBtn->getHeight() : 0) + padding;
    Button* redoBtn = addButton(0, yRedo, widthBtn, heightBtn, "REDO", 72, btnCol, "assets/font.ttf", HAlign::Center, VAlign::Top);
    if (redoBtn) {
        redoBtn->setLabelPositionPercent(0.5f, 0.70f);
        if (owner) redoBtn->setCallback([owner]() { redo(owner); });
    }

    int yReset = yRedo + (redoBtn ? redoBtn->getHeight() : 0) + padding;
    Button* resetBtn = addButton(0, yReset, widthBtn, heightBtn, "RESET", 72, btnCol, "assets/font.ttf", HAlign::Center, VAlign::Top);
    if (resetBtn) {
        resetBtn->setLabelPositionPercent(0.5f, 0.70f);
        if (owner) resetBtn->setCallback([owner]() { reset(owner); });
    }

    int ySettings = yReset + (resetBtn ? resetBtn->getHeight() : 0) + padding;
    Button* settingsBtn = addButton(0, ySettings, widthBtn, heightBtn, "SETTINGS", 72, btnCol, "assets/font.ttf", HAlign::Center, VAlign::Top);
    if (settingsBtn) {
        settingsBtn->setLabelPositionPercent(0.5f, 0.70f);
        if (owner) settingsBtn->setCallback([owner]() { settings(owner); });
    }
    return true;
}

AccountPanel::AccountPanel(SDL_Renderer* renderer) : Panel(renderer) {}

bool AccountPanel::init(User* user, bool hasUserFile, int winW, int winH, std::function<void()> onChanged)
{
    // create panel at the requested size
    if (!create(renderer, 0, 0, winW, winH)) {
        std::cerr << "AccountPanel::init - failed to create panel\n";
        return false;
    }

    if (!setBackgroundFromFile("assets/images/panel/settingsPanel.png")) {
        std::cerr << "AccountPanel::init - failed to load background\n";
        return false;
    }

    const SDL_Color titleCol = { 255, 0, 0, 255 };
    const SDL_Color btnCol = { 0xf9, 0xf2, 0x6a, 0xFF };

    // On first run (no user file): show account creation UI with textboxes
    if (!hasUserFile) {
        // Title: start at panel Y + 35% of panel height
        // Make title font ~20% of panel height (clamped)
        int titleFontSize = 72;
        int titleLocalY = static_cast<int>(getHeight() * 0.35f);
        addText("assets/font.ttf", titleFontSize, "CREATE AN ACCOUNT", titleCol, 0, titleLocalY, HAlign::Center, VAlign::Top);

        // Cursor starts below the title (title height + padding)
        int paddingAfterTitle = 30;
        int cursorY = titleLocalY + titleFontSize + paddingAfterTitle;

        // Textbox sizes and spacing
        int tbW = 1500;
        int tbH = 85;
        int tbSpacing = 24;

        // Username textbox (centered)
        Textbox* usernameTb = addTextbox(0, cursorY, tbW, tbH, "assets/images/textbox/inputTextbox.png", "USERNAME", 72, {0,0,0,255}, "assets/font.ttf", HAlign::Center, VAlign::Top);
        cursorY += tbH + tbSpacing;

        // Password textbox (centered)
        Textbox* passwordTb = addTextbox(0, cursorY, tbW, tbH, "assets/images/textbox/inputTextbox.png", "PASSWORD", 72, {0,0,0,255}, "assets/font.ttf", HAlign::Center, VAlign::Top);
        cursorY += tbH + 40; // a bit more space before confirm

        // Confirm button centered below textboxes
        Button* confirmBtn = addButton(0, cursorY, 350, 85, "CONFIRM", 72, {0xf9,0xf2,0x6a,0xFF}, "assets/font.ttf", HAlign::Center, VAlign::Top);
        if (confirmBtn) {
            confirmBtn->setLabelPositionPercent(0.5f, 0.70f);
            confirmBtn->setCallback([user, usernameTb, passwordTb, onChanged]() {
                if (!user || !usernameTb || !passwordTb) return;
                std::string username = usernameTb->getText();
                std::string password = passwordTb->getText();
                if (username.empty() || password.empty()) {
                    std::cerr << "AccountPanel: username or password is empty\n";
                    return;
                }
                bool ok = user->signin(username, password);
                if (!ok) {
                    std::cerr << "AccountPanel: signin failed\n";
                } else {
                    if (onChanged) onChanged();
                }
            });
        }
    } else {
        // User file exists: show login options (existing code)
        const int BtnW = 350;
        const int BtnH = 85;
        const int Padding = 16;
        const int FontSize = 72;

        auto onAccount = [&user]() -> bool {
            if (!user) return false;
            std::string uname = user->getUsername();
            if (uname.empty()) return false;
            if (uname.size() == 1 && uname[0] == '\0') return false;
            return true;
        };

        std::vector<std::string> labels;
        labels.push_back("SIGN IN");
        labels.push_back("LOG IN");
        if (onAccount()) labels.push_back("LOG OUT");

        int n = static_cast<int>(labels.size());
        int totalH = n * BtnH + (n - 1) * Padding;
        int startY = (getHeight() - totalH) / 2;

        for (int i = 0; i < n; ++i) {
            int localY = startY + i * (BtnH + Padding);
            Button* b = addButton(0, localY, BtnW, BtnH, labels[i], FontSize, btnCol, "assets/font.ttf", HAlign::Center, VAlign::Top);
            if (!b) continue;
            b->setLabelPositionPercent(0.5f, 0.70f);

            std::string lbl = labels[i];
            if (lbl == "SIGN IN") {
                b->setCallback([user, onChanged]() {
                    if (!user) return;
                    bool ok = user->signin("player", "");
                    if (!ok) std::cerr << "AccountPanel: signin failed\n";
                    if (onChanged) onChanged();
                });
            } else if (lbl == "LOG IN") {
                b->setCallback([]() {
                    // TODO: open login dialog
                });
            } else if (lbl == "LOG OUT") {
                b->setCallback([user, onChanged]() {
                    if (!user) return;
                    user->logout();
                    if (onChanged) onChanged();
                });
            }
        }
    }

    return true;
}

