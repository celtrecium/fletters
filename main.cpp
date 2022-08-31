#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <tdlcpp>
#include <type_traits>
#include <vector>
#include <functional>
#include <thread>
#include <iostream>

extern "C" {
  #include <u8string.h>
  #include <qwerty.h>
}

#define GAME_NAME "ПАДАЮЩИЕ БУКОВКИ :-)"
#define GAME_NAME_LEN 20


static const char *MAIN_MENU_ITEMS[] = {
  "   Старт   ",
  " Настройки ",
  "   Выход   "
};

static const char *SETTINGS_MENU_ITEMS[] = {
  "       Алфавит        ",
  " Частота падения букв ",
};

static const char *ALPHABET_MENU_ITEMS[] = {
  " Латиница  ",
  " Кириллица "
};

static const char *LATIN_ALPHABET = "abcdefghijklmnopqrstuvwxyz";
static const char *CYRILLIC_ALPHABET = "абвгдежзийклмнопрстуфхцчшщъыьэюя";

  class Letter {
  public:
    u8char_t letter;
    TDL::Point point;

    Letter(const u8char_t letter, const TDL::Point point);

    void moveDown();
  };

  Letter::Letter(const u8char_t letter_, const TDL::Point point_)
    : point(point_) {
    u8char_copy(this->letter, letter_);
  }

  void Letter::moveDown() {
    point.setY(point.getY() + 1);
  }
  
  class Keyboard : public qw_key_t {
  public:
    Keyboard();
    ~Keyboard();
    void update();
  };

  Keyboard::Keyboard() {
    qw_initialize();

    qw_key_t k = qw_get_key();

    key = k.key;
    strcpy(unicode_char, k.unicode_char);
  }

  void Keyboard::update() {
    qw_key_t k = qw_get_key();

    key = k.key;
    strcpy(unicode_char, k.unicode_char);
  }

  Keyboard::~Keyboard() {
    qw_end();
  }

  class Timer {
  public:
    uint64_t timer;
    uint64_t update();

    Timer();
  };

  uint64_t Timer::update() {
    if(timer == UINT64_MAX)
      timer = 0;

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    return ++timer;
  }

  Timer::Timer() : timer(0) {}

  struct Settings {
    const char *alphabet;
    uint64_t lettersSpawnFrequency;
  };
  
  class FLGame {
  private:
    std::vector<Letter> letters;
    TDL::Canvas &canv;
    TDL::Style letterStyle;
    Keyboard keyboard;
    Timer timer;
    const Settings settings;
    u8string_t alphabet;
    
    void display();
    void spawnLetter();
    void killLetter(qw_key_t key);
    void missLetter(int i);
    void showGameOver();
    void printLetter(Letter letter);
    void moveLettersDown();
    
  public:
    uint64_t speed;
    uint64_t score;
    uint64_t missedLettersCount;

    FLGame(TDL::Canvas &canv, Keyboard &keyboard, const Settings settings_);
  };

  FLGame::FLGame(TDL::Canvas &canv_, Keyboard &keyboard_, const Settings settings_)
    : canv(canv_),
      letterStyle(TDL::PointColor(TDL::Color::Default, TDL::Color::Default), TDL::Attributes::Bold),
      keyboard(keyboard_),
      settings(settings_),
      alphabet(u8string((cstr)settings.alphabet)),
      speed(1),
      score(0),
      missedLettersCount(0) {
    timer = Timer();
    srand((unsigned int)time(NULL));

    spawnLetter();
    
    while(keyboard.key != QW_KEY_ESC) {
      keyboard.update();

      if(keyboard.key != 0) {
        killLetter(static_cast<qw_key_t>(keyboard));
        display();
      }

      if(timer.update() % (200 - speed * 10) == 0) {
        moveLettersDown();
        display();
      }

      if(timer.timer % (1400 - 100 * settings.lettersSpawnFrequency) == 0) {
        spawnLetter();
        display();
      }

      if(missedLettersCount == 10) {
        showGameOver();
        std::this_thread::sleep_for(std::chrono::milliseconds(8000));
        
        break;
      }
    }

    u8string_free(&alphabet);
  }

  void FLGame::showGameOver() {
    TDL::Text gameover("Вы проиграли!",
                       TDL::Style(TDL::PointColor(TDL::Color::Red, TDL::Color::BrightWhite),
                                  TDL::Attributes::NullAttribute));

    TDL::Point textPos((int)((canv.getCanvasSize().getWidth() - gameover.getString<u8string_t>().length) / 2),
                       (int)(canv.getCanvasSize().getHeight() / 2));

    canv.setCursorPosition(textPos);
    canv.print(gameover);
    canv.display();
  }
  
  void FLGame::missLetter(int i) {
    letters.erase(letters.begin() + i);
    ++missedLettersCount;
  }

  void FLGame::spawnLetter() {
    u8char_t letter;
    u8char_copy(letter, alphabet.string[(size_t)rand() % alphabet.length]);
    
    letters.push_back(Letter(letter, TDL::Point(rand() % (int)(canv.getCanvasSize().getWidth()), 3)));
  }
  
  void FLGame::killLetter(qw_key_t key) {
    for(size_t i = 0; i < letters.size(); ++i) {
      if(key.key == *letters[i].letter || u8char_compare(key.unicode_char, letters[i].letter)) {
        letters.erase(letters.begin() + (int)(i));
        ++score;

        if(score % 20 == 0)
          ++speed;

        break;
      }
    }  
  }

  void FLGame::printLetter(Letter letter) {
    canv.setCursorPosition(letter.point);
    canv.print(TDL::Text(letter.letter, letterStyle));
  }

  void FLGame::moveLettersDown() {
    for(auto &l : letters)
      l.moveDown();

    for(size_t i = 0; i < letters.size(); ++i) {
      if(letters[i].point.getY() >= (int)(canv.getCanvasSize().getHeight()))
        missLetter((int)i--);
    }
  }
  
  void FLGame::display() {
    TDL::Style textStyle(TDL::PointColor(TDL::Color::BrightWhite, TDL::Color::Black), TDL::Attributes::Bold);
    
    canv.clear();

    for(auto &l : letters)
      printLetter(l);

    canv.setCursorPosition(TDL::Point(2, 1));
    canv.print(TDL::Text(std::string(" Очки: " + std::to_string(score) + ' ').c_str(), textStyle));

    canv.setCursorPosition(TDL::Point(canv.getCursorPosition().getX() + 2, canv.getCursorPosition().getY()));
    canv.print(TDL::Text(std::string(" Упущено: " + std::to_string(missedLettersCount) + ' ').c_str(), textStyle));
    
    canv.display();
  }

  enum class UIElementType {
    Menu,
    SpinBox
  };
  
  class UIElement {
  public:
    TDL::Point position;
    UIElementType type;

    UIElement(TDL::Point position_, UIElementType type_);

    virtual void print(TDL::Canvas &canv);
  };
  
  UIElement::UIElement(TDL::Point position_, UIElementType type_)
    : position(position_), type(type_) {}

  void UIElement::print(TDL::Canvas &canv) {
    (void)canv;
  }
  
  class Menu : public UIElement {
  private:
    std::vector<TDL::Text> items;
    const char **cstrItems;
    size_t selectedItem;
    TDL::Style selectedStyle;
    TDL::Style notSelectedStyle;

  public:
    Menu(TDL::Point position_, const char **items_, size_t itemsNumber);

    const char *getSelectedItem();
    size_t getSelectedItemNumber();
    void selectDown();
    void selectUp();
    void print(TDL::Canvas &canv) override;
  };

  Menu::Menu(TDL::Point position_, const char **items_, size_t itemsNumber)
    : UIElement(position_, UIElementType::Menu),
      cstrItems(items_),
      selectedItem(0),
      selectedStyle(TDL::PointColor(TDL::Color::BrightWhite, TDL::Color::Black), TDL::Attributes::Bold),
      notSelectedStyle(TDL::PointColor(TDL::Color::Default, TDL::Color::BrightWhite), TDL::Attributes::NullAttribute)
  {
    for(size_t i = 0; i < itemsNumber; ++i)
      items.push_back(TDL::Text(cstrItems[i], notSelectedStyle));

    items[selectedItem].setStyle(selectedStyle);
  }

  const char *Menu::getSelectedItem() {
    return cstrItems[selectedItem];
  }

  size_t Menu::getSelectedItemNumber() {
    return selectedItem;
  }
  
  void Menu::selectDown() {
    if(selectedItem == items.size() - 1)
      return;

    items[selectedItem].setStyle(notSelectedStyle);
    items[++selectedItem].setStyle(selectedStyle);
  }

  void Menu::selectUp() {
    if(selectedItem == 0)
      return;

    items[selectedItem].setStyle(notSelectedStyle);
    items[--selectedItem].setStyle(selectedStyle);
  }

  void Menu::print(TDL::Canvas &canv) {
    TDL::Point itemPos(position);
    
    for(auto &i : items) {
      canv.setCursorPosition(itemPos);
      canv.print(i);

      itemPos.setY(itemPos.getY() + 1);
    }
  }

  class SpinBox : public UIElement {
  private:
    TDL::Text text;
    size_t value;
    size_t minimum;
    size_t maximum;

    void updateText();

  public:
    SpinBox(TDL::Point position_, size_t min, size_t max);

    size_t getValue();
    void inc();
    void dec();
    void print(TDL::Canvas &canv) override;
  };
 
  void SpinBox::updateText() {
    text.setString("<| " + std::to_string(value) + " |>");
  }
  
  SpinBox::SpinBox(TDL::Point position_, size_t min, size_t max)
    : UIElement(position_, UIElementType::SpinBox),
      text(std::string("<| " + std::to_string(min) + " |>"),
           TDL::Style(TDL::PointColor(TDL::Color::Default, TDL::Color::Default), TDL::Attributes::NullAttribute)),
      value(min),
      minimum(min),
      maximum(max) {}

  size_t SpinBox::getValue() {
    return value;
  }

  void SpinBox::inc() {
    if(value == maximum)
      return;

    ++value;
    updateText();
  }
  
  void SpinBox::dec() {
    if(value == minimum)
      return;

    --value;
    updateText();
  }

  void SpinBox::print(TDL::Canvas &canv) {
    canv.setCursorPosition(position);
    canv.print(text);
  }
  
  class TitleScreen {
  private:
    Menu mainMenu;
    Menu settingsMenu;
    Menu alphabetMenu;
    SpinBox frequencySetting;
    TDL::Text titleText;
    TDL::Point titleTextPos;
    TDL::Canvas &canv;
    Keyboard &keyboard;
    UIElement *currentUIElement;

    void update();

    size_t selectMenuItem();
    
    void enterMainMenu();
    void enterSettingsMenu();
    void enterAlphabetMenu();
    void enterFrequencySpinBox();
    
  public:
    bool isStartAborted;
    Settings settings;

    TitleScreen(TDL::Canvas &canv_, Keyboard &keyboard_);
  };

  TitleScreen::TitleScreen(TDL::Canvas &canv_, Keyboard &keyboard_)
    : mainMenu(TDL::Point(((int)canv_.getCanvasSize().getWidth() - 11) / 2,
                          (int)canv_.getCanvasSize().getHeight() / 2), MAIN_MENU_ITEMS, 3),
      settingsMenu(mainMenu.position, SETTINGS_MENU_ITEMS, 2),
      alphabetMenu(mainMenu.position, ALPHABET_MENU_ITEMS, 2),
      frequencySetting(mainMenu.position, 1, 9),
      titleText(GAME_NAME, TDL::Style(TDL::PointColor(TDL::Color::Default, TDL::Color::BrightWhite),
                                      TDL::Attributes::Bold)),
      titleTextPos((int)(canv_.getCanvasSize().getWidth() - GAME_NAME_LEN) / 2,
                   (int)canv_.getCanvasSize().getHeight() / 2 - 4),
      canv(canv_),
      keyboard(keyboard_),
      currentUIElement(&mainMenu),
      isStartAborted(false){
    settingsMenu.position.setX(((int)canv.getCanvasSize().getWidth() - 22) / 2);
    frequencySetting.position.setX(((int)canv.getCanvasSize().getWidth() - 7) / 2);

    settings.alphabet = LATIN_ALPHABET;
    settings.lettersSpawnFrequency = 1;
    
    enterMainMenu();
  }

  size_t TitleScreen::selectMenuItem() {
    while(true) {
      keyboard.update();
      
      switch (keyboard.key) {
      case QW_KEY_DOWN:
        static_cast<Menu*>(currentUIElement)->selectDown();
        update();

        break;
      case QW_KEY_UP:
        static_cast<Menu*>(currentUIElement)->selectUp();
        update();

        break;
      case QW_KEY_RETURN:
        return static_cast<Menu*>(currentUIElement)->getSelectedItemNumber();

      default:
        break;
      }
    }
  }
  
  void TitleScreen::enterMainMenu() {
    while(true) {
      currentUIElement = &mainMenu;
      update();
      
      switch(selectMenuItem()) {
        // Старт
      case 0:
        return;
        // Настройки
      case 1:
        enterSettingsMenu();

        break;
        // Выход
      case 2:
        isStartAborted = true;
        return;
      }
    }
  }

  void TitleScreen::enterSettingsMenu() {
    currentUIElement = &settingsMenu;
    update();

    while(true) {
      switch (selectMenuItem()) {
        // Алфавит
      case 0:
        enterAlphabetMenu();
        return;

        // Частота падения букв
      case 1:
        enterFrequencySpinBox();
        return;
      }
    }
  }

  void TitleScreen::enterAlphabetMenu() {
    currentUIElement = &alphabetMenu;
    update();

    while(true) {
      switch (selectMenuItem()) {
        // Латиница
      case 0:
        settings.alphabet = LATIN_ALPHABET;
        return;

        // Кириллица
      case 1:
        settings.alphabet = CYRILLIC_ALPHABET;
        return;
      }
    }
  }

  void TitleScreen::enterFrequencySpinBox() {
    currentUIElement = &frequencySetting;
    update();
    
    while(true) {
      keyboard.update();
      
      switch (keyboard.key) {
      case QW_KEY_LEFT:
        static_cast<SpinBox*>(currentUIElement)->dec();
        update();

        break;
      case QW_KEY_RIGHT:
        static_cast<SpinBox*>(currentUIElement)->inc();
        update();

        break;
      case QW_KEY_RETURN:
        settings.lettersSpawnFrequency = static_cast<SpinBox*>(currentUIElement)->getValue();
        return;

      default:
        break;
      }
    }
  }

  void TitleScreen::update() {
    canv.clear();
    canv.setCursorPosition(titleTextPos);
    canv.print(titleText);

    currentUIElement->print(canv);

    canv.display();
  }

int main() {
  TDL::Canvas canv;
  Keyboard keyboard;

  TDL::Terminal::setCursor(false);
  TDL::Terminal::setAlternateScreen(true);

  TitleScreen mainMenu(canv, keyboard);

  if(!mainMenu.isStartAborted) {
    FLGame game(canv, keyboard, mainMenu.settings);

    TDL::Terminal::setCursor(true);
    TDL::Terminal::setAlternateScreen(false);

    std::cout <<
      "Игра окончена\n"
      "- Набрано очков:      " << std::to_string(game.score) << "\n"
      "- Финальная скорость: " << std::to_string(game.speed) << "\n"
      "- Упущено букв:       " << std::to_string(game.missedLettersCount) << std::endl;
  } else {
    TDL::Terminal::setCursor(true);
    TDL::Terminal::setAlternateScreen(false);
  }

  return 0;
}
