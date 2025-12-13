#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <vector>
#include <memory>
#include <functional> // added
#include "button.h"
#include "../text.h"
#include "../user.h"
#include "textbox.h"

class Panel {
public:
    enum class HAlign { Left, Center, Right };
    enum class VAlign { Top, Middle, Bottom };

    Panel(SDL_Renderer* renderer = nullptr);
    ~Panel();

    // create / lifecycle
    bool create(SDL_Renderer* renderer, int x, int y, int w, int h);
    void cleanup();

    // panel transform
    void setPosition(int x, int y);

    // background
    bool setBackgroundFromFile(const std::string& path); // loads texture and owns it
    void clearBackground();

    // create & add children (Panel owns them)
    Button* addButton(int localX, int localY, int w, int h,
                      const std::string& text = "",
                      int fontSize = 16,
                      SDL_Color textColor = {255,255,255,255},
                      const std::string& fontPath = "asset/font.ttf",
                      HAlign halign = HAlign::Left,
                      VAlign valign = VAlign::Top);

    Text* addText(const std::string& fontPath, int fontSize,
                  const std::string& text,
                  SDL_Color color,
                  int localX, int localY,
                  HAlign halign = HAlign::Left,
                  VAlign valign = VAlign::Top);

    // add existing image (panel takes ownership=false), size w/h are local
    void addImage(SDL_Texture* tex, int localX, int localY, int w, int h,
                  HAlign halign = HAlign::Left,
                  VAlign valign = VAlign::Top);

    // add textbox
    Textbox* addTextbox(int localX, int localY, int w, int h,
                    const std::string& bgPath,
                    const std::string& placeholderText,
                    int fontSize = 72,
                    SDL_Color textColor = {255,255,255,255},
                    const std::string& fontPath = "assets/font.ttf",
                    HAlign halign = HAlign::Left,
                    VAlign valign = VAlign::Top);

    // event forwarding (mouse coords are translated into panel-local for children)
    void handleEvent(const SDL_Event& e);

    // draw all children (positions are relative to panel)
    void render();

    // getters
    int getX() const;
    int getY() const;
    int getWidth() const;
    int getHeight() const;

protected:
    SDL_Renderer* renderer = nullptr;
private:
    struct Child {
        enum class Type { Button, Text, Image, Textbox } type;
        std::unique_ptr<Button> button;
        std::unique_ptr<Text> text;
        std::unique_ptr<Textbox> textbox;
        SDL_Texture* image = nullptr;
        int localX = 0;
        int localY = 0;
        int w = 0, h = 0;
        HAlign halign = HAlign::Left;
        VAlign valign = VAlign::Top;
    };

    SDL_Texture* bgTexture = nullptr;
    int x = 0, y = 0, w = 0, h = 0;
    std::vector<Child> children;

    // helpers
    SDL_FRect computeChildDst(const Child& c) const;
};

class Game;

class IngamePanel : public Panel {
public:
    IngamePanel(SDL_Renderer* renderer = nullptr);
    bool initForStage(const std::string& stage, Game* owner,
                      int winW, int mapPxW, int winH, int mapPxH);
};

class AccountPanel : public Panel {
public:
    AccountPanel(SDL_Renderer* renderer = nullptr);
    ~AccountPanel() = default;

    // initialize panel UI. hasUserFile == true when users file exists.
    // onChanged() called when account state changes (e.g. signin created file)
    bool init(User* user, bool hasUserFile, int winW, int winH, std::function<void()> onChanged = nullptr);
};
class SettingsPanel : public AccountPanel {
    public:
        SettingsPanel(SDL_Renderer* renderer = nullptr);
        ~SettingsPanel() = default;
    
        // initialize settings panel UI
        // isInGame: true nếu đang trong Game, false nếu đang trong Start screen
        // onQuitGame: callback để thoát game (chỉ dùng khi isInGame = true)
        bool init(User* user, int winW, int winH, std::function<void()> onChanged = nullptr, 
                  bool isInGame = false, std::function<void()> onQuitGame = nullptr);
    };