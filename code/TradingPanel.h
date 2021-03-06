// TradingPanel.h

#ifndef TRADING_PANEL_H_
#define TRADING_PANEL_H_

#include "Panel.h"

class PlayerInfo;
class System;



// Overlay on the PlanetPanel showing commodity prices and inventory, and allowing
// buying and selling. This also lets you sell any plundered outfits you are
// carrying, although if you want to sell only certain ones and not others you
// will need to select them individually in the outfitter panel.
class TradingPanel : public Panel {
public:
  explicit TradingPanel(PlayerInfo &player);
  ~TradingPanel();

  virtual void Step() override;
  virtual void Draw() override;


protected:
  // Only override the ones you need; the default action is to return false.
  virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
  virtual bool Click(int x, int y, int clicks) override;


private:
  void Buy(int64_t amount);


private:
  PlayerInfo &player;
  const System &system;
  const int COMMODITY_COUNT;

  // Remember whether the "sell all" button will sell all outfits, or sell
  // everything except outfits.
  bool sellOutfits = false;

  // Keep track of how much we sold and how much profit was made.
  int tonsSold = 0;
  int64_t profit = 0;
};



#endif
