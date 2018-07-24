/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "model_mixes.h"

#define SET_DIRTY()     storageDirty(EE_MODEL)

uint8_t getMixesCount()
{
  uint8_t count = 0;
  uint8_t ch ;

  for (int i=MAX_MIXERS-1; i>=0; i--) {
    ch = mixAddress(i)->srcRaw;
    if (ch != 0) {
      count++;
    }
  }
  return count;
}

bool reachMixesLimit()
{
  if (getMixesCount() >= MAX_MIXERS) {
    POPUP_WARNING(STR_NOFREEMIXER);
    return true;
  }
  return false;
}


class MixEditWindow: public Page {
  public:
    MixEditWindow(int8_t channel, uint8_t mixIndex):
      Page(),
      channel(channel),
      mixIndex(mixIndex)
    {
      buildBody(&body);
      buildHeader(&header);
    }

  protected:
    uint8_t channel;
    uint8_t mixIndex;
    Window * updateCurvesWindow = nullptr;
    Choice * curveTypeChoice = nullptr;

    void buildHeader(Window * window) {
      new StaticText(window, { 70, 4, 100, 20 }, STR_MIXER, MENU_TITLE_COLOR);
      char s[16];
      new StaticText(window, { 70, 28, 100, 20 }, getSourceString(s, MIXSRC_CH1 + channel), MENU_TITLE_COLOR);
    }

    void updateCurves() {
      GridLayout grid(*updateCurvesWindow);
      updateCurvesWindow->clear();

      MixData * mix = mixAddress(mixIndex) ;

      new StaticText(updateCurvesWindow, grid.getLabelSlot(true), STR_CURVE);
      curveTypeChoice = new Choice(updateCurvesWindow, grid.getFieldSlot(2,0), "\004DiffExpoFuncCstm", 0, CURVE_REF_CUSTOM,
                 GET_DEFAULT(mix->curve.type),
                 [=](int32_t newValue) { mix->curve.type = newValue;
                     SET_DIRTY();
                     updateCurves();
                     curveTypeChoice->setFocus();
                 });

      switch (mix->curve.type) {
        case CURVE_REF_DIFF:
        case CURVE_REF_EXPO:
          // TODO GVAR
          new NumberEdit(updateCurvesWindow, grid.getFieldSlot(2,1), -100, 100, 1, GET_SET_DEFAULT(mix->curve.value), 0, nullptr, "%");
          break;
        case CURVE_REF_FUNC:
          new Choice(updateCurvesWindow, grid.getFieldSlot(2,1), STR_VCURVEFUNC, 0, CURVE_BASE-1, GET_SET_DEFAULT(mix->curve.value));
          break;
        case CURVE_REF_CUSTOM:
          new CustomCurveChoice(updateCurvesWindow, grid.getFieldSlot(2,1), -MAX_CURVES, MAX_CURVES, GET_SET_DEFAULT(mix->curve.value));
          break;
      }
    }

    void buildBody(Window * window) {
      GridLayout grid(*window);
      grid.spacer(8);

      MixData * mix = mixAddress(mixIndex) ;

      // Mix name
      new StaticText(window, grid.getLabelSlot(true), STR_MIXNAME);
      new TextEdit(window, grid.getFieldSlot(), mix->name, sizeof(mix->name));
      grid.nextLine();

      // Source
      new StaticText(window, grid.getLabelSlot(true), STR_SOURCE);
      new SourceChoice(window, grid.getFieldSlot(), MIXSRC_LAST, GET_SET_DEFAULT(mix->srcRaw));
      grid.nextLine();

      // Weight
      new StaticText(window, grid.getLabelSlot(true), STR_WEIGHT);
      // TODO GVAR ?
      new NumberEdit(window, grid.getFieldSlot(), -100, 100, 1, GET_SET_DEFAULT(mix->weight), 0, nullptr, "%");
      grid.nextLine();

      // Offset
      new StaticText(window, grid.getLabelSlot(true), STR_OFFSET);
      new NumberEdit(window, grid.getFieldSlot(), GV_RANGELARGE_OFFSET_NEG, GV_RANGELARGE_OFFSET, 1, GET_SET_DEFAULT(mix->offset), 0, nullptr, "%");
      grid.nextLine();

      // Trim
      new StaticText(window, grid.getLabelSlot(true), STR_TRIM);
      new CheckBox(window, grid.getFieldSlot(), GET_SET_INVERTED(mix->carryTrim));
      grid.nextLine();

      // Curve
      updateCurvesWindow = new Window(window, { 0, grid.getWindowHeight(), LCD_W, 0 });
      updateCurves();
      grid.addWindow(updateCurvesWindow);

      // Flight modes
      new StaticText(window, grid.getLabelSlot(true), STR_FLMODE);
      for (uint8_t i=0; i<MAX_FLIGHT_MODES; i++) {
        char fm[2] = { char('0' + i), '\0' };
        if (i > 0 && (i % 4) == 0)
          grid.nextLine();
        new TextButton(window, grid.getFieldSlot(4, i % 4), fm,
                                       [=]() -> uint8_t {
                                           BF_BIT_FLIP(mix->flightModes, BF_BIT(i));
                                           SET_DIRTY();
                                           return bool(BF_SINGLE_BIT_GET(mix->flightModes, i));
                                       },
                                       bool(BF_SINGLE_BIT_GET(mix->flightModes, i)));
      }
      grid.nextLine();

      // Switch
      new StaticText(window, grid.getLabelSlot(true), STR_SWITCH);
      new SwitchChoice(window, grid.getFieldSlot(), MixesContext, GET_SET_DEFAULT(mix->swtch));
      grid.nextLine();

      // Warning
      new StaticText(window, grid.getLabelSlot(true), STR_MIXWARNING);
      new NumberEdit(window, grid.getFieldSlot(2, 0), 0, 3, 1, GET_SET_DEFAULT(mix->mixWarn), 0, nullptr, nullptr, STR_OFF);
      grid.nextLine();

      // Multiplex
      new StaticText(window, grid.getLabelSlot(true), STR_MULTPX);
      new Choice(window, grid.getFieldSlot(), STR_VMLTPX, 0, 2, GET_SET_DEFAULT(mix->mltpx));
      grid.nextLine();

      // Delay up
      new StaticText(window, grid.getLabelSlot(true), STR_DELAYUP);
      new NumberEdit(window, grid.getFieldSlot(2, 0), 0, DELAY_MAX, 10 / DELAY_STEP,
                     GET_DEFAULT(mix->delayUp),
                     SET_VALUE(mix->delayUp, newValue),
                     PREC1, nullptr, "s");
      grid.nextLine();

      // Delay down
      new StaticText(window, grid.getLabelSlot(true), STR_DELAYDOWN);
      new NumberEdit(window, grid.getFieldSlot(2, 0), 0, DELAY_MAX, 10 / DELAY_STEP,
                     GET_DEFAULT(mix->delayDown),
                     SET_VALUE(mix->delayDown, newValue),
                     PREC1, nullptr, "s");
      grid.nextLine();

      // Slow up
      new StaticText(window, grid.getLabelSlot(true), STR_SLOWUP);
      new NumberEdit(window, grid.getFieldSlot(2, 0), 0, DELAY_MAX, 10 / DELAY_STEP,
                     GET_DEFAULT(mix->speedUp),
                     SET_VALUE(mix->speedUp, newValue),
                     PREC1, nullptr, "s");
      grid.nextLine();

      // Slow down
      new StaticText(window, grid.getLabelSlot(true), STR_SLOWDOWN);
      new NumberEdit(window, grid.getFieldSlot(2, 0), 0, DELAY_MAX, 10 / DELAY_STEP,
                     GET_DEFAULT(mix->speedDown),
                     SET_VALUE(mix->speedDown, newValue),
                     PREC1, nullptr, "s");
      grid.nextLine();

      window->setInnerHeight(grid.getWindowHeight());
    }
};

ModelMixesPage::ModelMixesPage():
  PageTab(STR_MIXER, ICON_MODEL_MIXER)
{
}

class MixLineButton : public Button {
  public:
    MixLineButton(Window * parent, const rect_t & rect, uint8_t mixIndex, std::function<uint8_t(void)> onPress):
      Button(parent, rect, onPress),
      mixIndex(mixIndex)
    {
      const MixData & mix = g_model.mixData[mixIndex];
      if (mix.swtch || mix.curve.value != 0 || mix.flightModes) {
        setHeight(45);
      }
    }

    bool isActive() {
      return isMixActive(mixIndex);
    }

    virtual void checkEvents() override
    {
      if (active != isActive()) {
        invalidate();
        active = !active;
      }
    }

    void paintMixFlightModes(BitmapBuffer * dc, FlightModesType value) {
      dc->drawBitmap(146, 24, mixerSetupFlightmodeBitmap);
      coord_t x = 166;
      for (int i=0; i<MAX_FLIGHT_MODES; i++) {
        char s[] = " ";
        s[0] = '0' + i;
        if (value & (1 << i)) {
          dc->drawSolidFilledRect(x, 40, 8, 3, SCROLLBOX_COLOR);
          dc->drawText(x, 23, s, SMLSIZE);
        }
        else {
          dc->drawText(x, 23, s, SMLSIZE | TEXT_DISABLE_COLOR);
        }
        x += 8;
      }
    }

    void paintMixLine(BitmapBuffer * dc) {
      const MixData & mix = g_model.mixData[mixIndex];

      // first line ...
      drawNumber(dc, 3, 2, mix.weight, 0, 0, nullptr, "%");
      // TODO gvarWeightItem(MIX_LINE_WEIGHT_POS, y, md, RIGHT | attr | (isMixActive(i) ? BOLD : 0), event);

      drawSource(dc, 60, 2, mix.srcRaw);

      if (mix.name[0]) {
        dc->drawBitmap(146, 4, mixerSetupLabelBitmap);
        dc->drawSizedText(166, 2, mix.name, sizeof(mix.name), ZCHAR);
      }

      // second line ...
      if (mix.swtch) {
        dc->drawBitmap(3, 24, mixerSetupSwitchBitmap);
        drawSwitch(21, 22, mix.swtch);
      }

      if (mix.curve.value != 0 ) {
        dc->drawBitmap(60, 24, mixerSetupCurveBitmap);
        drawCurveRef(dc, 80, 22, mix.curve);
      }

      if (mix.flightModes) {
        paintMixFlightModes(dc, mix.flightModes);
      }
    }

    virtual void paint(BitmapBuffer * dc) override {
      if (active)
        dc->drawSolidFilledRect(1, 1, rect.w-2, rect.h-2, CURVE_AXIS_COLOR);
      paintMixLine(dc);
      drawSolidRect(dc, 0, 0, rect.w, rect.h, 2, hasFocus() ? SCROLLBOX_COLOR : CURVE_AXIS_COLOR);
    }

  protected:
    uint8_t mixIndex;
    bool active = false;
};

void insertMix(uint8_t idx, uint8_t channel)
{
  pauseMixerCalculations();
  MixData * mix = mixAddress(idx);
  memmove(mix+1, mix, (MAX_MIXERS-(idx+1))*sizeof(MixData));
  memclear(mix, sizeof(MixData));
  mix->destCh = channel;
  mix->srcRaw = channel+1;
  if (!isSourceAvailable(mix->srcRaw)) {
    mix->srcRaw = (channel > 3 ? MIXSRC_Rud - 1 + channel : MIXSRC_Rud - 1 + channel_order(channel));
    while (!isSourceAvailable(mix->srcRaw)) {
      mix->srcRaw += 1;
    }
  }
  mix->weight = 100;
  resumeMixerCalculations();
  storageDirty(EE_MODEL);
}

void ModelMixesPage::rebuild(Window * window)
{
  coord_t scrollPosition = window->getScrollPositionY();
  window->clear();
  build(window);
  window->setScrollPositionY(scrollPosition);
}

void ModelMixesPage::editMix(Window * window, uint8_t channel, uint8_t mixIndex)
{
  Window * mixWindow = new MixEditWindow(channel, mixIndex);
  mixWindow->setCloseHandler([=]() {
    rebuild(window);
  });
}

void ModelMixesPage::build(Window * window)
{
  GridLayout grid(*window);
  grid.spacer(8);
  grid.setLabelWidth(66);
  grid.setLabelPaddingRight(6);

  window->clear();

  const BitmapBuffer * const mixerMultiplexBitmap[] = {
    mixerSetupAddBitmap,
    mixerSetupMultiBitmap,
    mixerSetupReplaceBitmap
  };

  char s[16];
  int mixIndex = 0;
  MixData * mix = g_model.mixData;
  for (int8_t ch=0; ch<MAX_OUTPUT_CHANNELS; ch++) {
    if (mix->srcRaw > 0 && mix->destCh == ch) {
      new TextButton(window, grid.getLabelSlot(), getSourceString(s, MIXSRC_CH1 + ch),
                     [=]() -> uint8_t {
                       return 0;
                     }, 0);
      uint8_t count = 0;
      while (mix->srcRaw > 0 && mix->destCh == ch) {
        Button * button = new MixLineButton(window, grid.getFieldSlot(), mixIndex,
                          [=]() -> uint8_t {
                            Menu * menu = new Menu();
                            menu->addLine(STR_EDIT, [=]() {
                              menu->deleteLater();
                              editMix(window, ch, mixIndex);
                            });
                            if (!reachMixesLimit()) {
                              menu->addLine(STR_INSERT_BEFORE, [=]() {
                                menu->deleteLater();
                                insertMix(mixIndex, ch);
                                editMix(window, ch, mixIndex);
                              });
                              menu->addLine(STR_INSERT_AFTER, [=]() {
                                menu->deleteLater();
                                insertMix(mixIndex+1, ch);
                                editMix(window, ch, mixIndex);
                              });
                              // TODO STR_COPY
                            }
                            // TODO STR_MOVE
                            menu->addLine(STR_DELETE, [=]() {
                              menu->deleteLater();
                              deleteMix(mixIndex);
                              rebuild(window);
                            });
                            return FOCUS_STATE;
                          });

        if (count++ > 0) {
          new StaticBitmap(window, {35, button->top() + (button->height() - 17) / 2, 25, 17}, mixerMultiplexBitmap[mix->mltpx]);
        }

        grid.spacer(button->height() + 5);
        ++mixIndex;
        ++mix;
      }
    }
    else {
      new TextButton(window, grid.getLabelSlot(), getSourceString(s, MIXSRC_CH1 + ch),
                     [=]() -> uint8_t {
                       insertMix(mixIndex, ch);
                       editMix(window, ch, mixIndex);
                       return FOCUS_STATE;
                     }, 0);
      grid.nextLine();
    }
  }

  window->setInnerHeight(grid.getWindowHeight());
}

void displayMixStatus(uint8_t channel);

int getMixesLinesCount()
{
  int lastch = -1;
  uint8_t count = MAX_OUTPUT_CHANNELS;
  for (int i=0; i<MAX_MIXERS; i++) {
    bool valid = mixAddress(i)->srcRaw;
    if (!valid)
      break;
    int ch = mixAddress(i)->destCh;
    if (ch == lastch) {
      count++;
    }
    else {
      lastch = ch;
    }
  }
  return count;
}

void deleteMix(uint8_t idx)
{
  pauseMixerCalculations();
  MixData * mix = mixAddress(idx);
  memmove(mix, mix+1, (MAX_MIXERS-(idx+1))*sizeof(MixData));
  memclear(&g_model.mixData[MAX_MIXERS-1], sizeof(MixData));
  resumeMixerCalculations();
  storageDirty(EE_MODEL);
}

void insertMix(uint8_t idx)
{
  insertMix(idx, s_currCh+1);
}

void copyMix(uint8_t idx)
{
  pauseMixerCalculations();
  MixData * mix = mixAddress(idx);
  memmove(mix+1, mix, (MAX_MIXERS-(idx+1))*sizeof(MixData));
  resumeMixerCalculations();
  storageDirty(EE_MODEL);
}

bool swapMixes(uint8_t & idx, uint8_t up)
{
  MixData * x, * y;
  int8_t tgt_idx = (up ? idx-1 : idx+1);

  x = mixAddress(idx);

  if (tgt_idx < 0) {
    if (x->destCh == 0)
      return false;
    x->destCh--;
    return true;
  }

  if (tgt_idx == MAX_MIXERS) {
    if (x->destCh == MAX_OUTPUT_CHANNELS-1)
      return false;
    x->destCh++;
    return true;
  }

  y = mixAddress(tgt_idx);
  uint8_t destCh = x->destCh;
  if(!y->srcRaw || destCh != y->destCh) {
    if (up) {
      if (destCh>0) x->destCh--;
      else return false;
    }
    else {
      if (destCh<MAX_OUTPUT_CHANNELS-1) x->destCh++;
      else return false;
    }
    return true;
  }

  pauseMixerCalculations();
  memswap(x, y, sizeof(MixData));
  resumeMixerCalculations();

  idx = tgt_idx;
  return true;
}

enum MixFields {
  MIX_FIELD_NAME,
  MIX_FIELD_SOURCE,
  MIX_FIELD_WEIGHT,
  MIX_FIELD_OFFSET,
  MIX_FIELD_TRIM,
  CASE_CURVES(MIX_FIELD_CURVE)
  CASE_FLIGHT_MODES(MIX_FIELD_FLIGHT_MODE)
  MIX_FIELD_SWITCH,
  // MIX_FIELD_WARNING,
  MIX_FIELD_MLTPX,
  MIX_FIELD_DELAY_UP,
  MIX_FIELD_DELAY_DOWN,
  MIX_FIELD_SLOW_UP,
  MIX_FIELD_SLOW_DOWN,
  MIX_FIELD_COUNT
};

void gvarWeightItem(coord_t x, coord_t y, MixData *md, LcdFlags attr, event_t event)
{
  u_int8int16_t weight;
  MD_WEIGHT_TO_UNION(md, weight);
  weight.word = GVAR_MENU_ITEM(x, y, weight.word, GV_RANGELARGE_WEIGHT_NEG, GV_RANGELARGE_WEIGHT, attr, 0, event);
  MD_UNION_TO_WEIGHT(weight, md);
}

bool menuModelMixOne(event_t event)
{
  MixData * md2 = mixAddress(s_currIdx) ;

  SUBMENU_WITH_OPTIONS(STR_MIXER, ICON_MODEL_MIXER, MIX_FIELD_COUNT, OPTION_MENU_NO_SCROLLBAR, { 0, 0, 0, 0, 0, CASE_CURVES(1) CASE_FLIGHT_MODES((MAX_FLIGHT_MODES-1) | NAVIGATION_LINE_BY_LINE) 0 /*, ...*/ });
  putsChn(50, 3+FH, md2->destCh+1, MENU_TITLE_COLOR);
  displayMixStatus(md2->destCh);

  // The separation line between 2 columns
  lcdDrawSolidVerticalLine(MENU_COLUMN2_X-10, DEFAULT_SCROLLBAR_Y-FH, DEFAULT_SCROLLBAR_H+5, TEXT_COLOR);

  int8_t sub = menuVerticalPosition;
  int8_t editMode = s_editMode;

  for (int k=0; k<2*NUM_BODY_LINES; k++) {
    coord_t y;
    if (k >= NUM_BODY_LINES) {
      y = MENU_CONTENT_TOP - FH + (k-NUM_BODY_LINES)*FH;
    }
    else {
      y = MENU_CONTENT_TOP - FH + k*FH;
    }
    int8_t i = k;

    LcdFlags attr = (sub==i ? (editMode>0 ? BLINK|INVERS : INVERS) : 0);
    switch(i) {
      case MIX_FIELD_NAME:
        lcdDrawText(MENUS_MARGIN_LEFT, y, STR_MIXNAME);
        editName(MIXES_2ND_COLUMN, y, md2->name, sizeof(md2->name), event, attr);
        break;
      case MIX_FIELD_SOURCE:
        lcdDrawText(MENUS_MARGIN_LEFT, y, NO_INDENT(STR_SOURCE));
        drawSource(MIXES_2ND_COLUMN, y, md2->srcRaw, attr);
        if (attr) CHECK_INCDEC_MODELSOURCE(event, md2->srcRaw, 1, MIXSRC_LAST);
        break;
      case MIX_FIELD_WEIGHT:
        lcdDrawText(MENUS_MARGIN_LEFT, y, STR_WEIGHT);
        gvarWeightItem(MIXES_2ND_COLUMN, y, md2, attr|LEFT, event);
        break;
      case MIX_FIELD_OFFSET:
      {
        lcdDrawText(MENUS_MARGIN_LEFT, y, NO_INDENT(STR_OFFSET));
        u_int8int16_t offset;
        MD_OFFSET_TO_UNION(md2, offset);
        offset.word = GVAR_MENU_ITEM(MIXES_2ND_COLUMN, y, offset.word, GV_RANGELARGE_OFFSET_NEG, GV_RANGELARGE_OFFSET, attr|LEFT, 0, event);
        MD_UNION_TO_OFFSET(offset, md2);
#if 0
        drawOffsetBar(x+MIXES_2ND_COLUMN+22, y, md2);
#endif
        break;
      }
      case MIX_FIELD_TRIM:
        lcdDrawText(MENUS_MARGIN_LEFT, y, STR_TRIM);
        drawCheckBox(MIXES_2ND_COLUMN, y, !md2->carryTrim, attr);
        if (attr) md2->carryTrim = !checkIncDecModel(event, !md2->carryTrim, 0, 1);
        break;
#if defined(CURVES)
      case MIX_FIELD_CURVE:
        lcdDrawText(MENUS_MARGIN_LEFT, y, STR_CURVE);
        editCurveRef(MIXES_2ND_COLUMN, y, md2->curve, event, attr);
        break;
#endif
#if defined(FLIGHT_MODES)
      case MIX_FIELD_FLIGHT_MODE:
        lcdDrawText(MENUS_MARGIN_LEFT, y, STR_FLMODE);
        md2->flightModes = editFlightModes(MIXES_2ND_COLUMN, y, event, md2->flightModes, attr);
        break;
#endif
      case MIX_FIELD_SWITCH:
        lcdDrawText(MENUS_MARGIN_LEFT, y, STR_SWITCH);
        md2->swtch = editSwitch(MIXES_2ND_COLUMN, y, md2->swtch, attr, event);
        break;
      case MIX_FIELD_MLTPX:
        lcdDrawText(MENUS_MARGIN_LEFT, y, STR_MULTPX);
        md2->mltpx = editChoice(MIXES_2ND_COLUMN, y, STR_VMLTPX, md2->mltpx, 0, 2, attr, event);
        break;
      case MIX_FIELD_DELAY_UP:
        lcdDrawText(MENU_COLUMN2_X+MENUS_MARGIN_LEFT, y, STR_DELAYUP);
        md2->delayUp = editDelay(MENU_COLUMN2_X, y, event, attr, md2->delayUp);
        break;
      case MIX_FIELD_DELAY_DOWN:
        lcdDrawText(MENU_COLUMN2_X+MENUS_MARGIN_LEFT, y, STR_DELAYDOWN);
        md2->delayDown = editDelay(MENU_COLUMN2_X, y, event, attr, md2->delayDown);
        break;
      case MIX_FIELD_SLOW_UP:
        lcdDrawText(MENU_COLUMN2_X+MENUS_MARGIN_LEFT, y, STR_SLOWUP);
        md2->speedUp = editDelay(MENU_COLUMN2_X, y, event, attr, md2->speedUp);
        break;
      case MIX_FIELD_SLOW_DOWN:
        lcdDrawText(MENU_COLUMN2_X+MENUS_MARGIN_LEFT, y, STR_SLOWDOWN);
        md2->speedDown = editDelay(MENU_COLUMN2_X, y, event, attr, md2->speedDown);
        break;
    }
  }

  return true;
}

#define _STR_MAX(x) PSTR("/" #x)
#define STR_MAX(x) _STR_MAX(x)

#define MIX_LINE_WEIGHT_POS     105
#define MIX_LINE_SRC_POS        120
#define MIX_LINE_CURVE_ICON     175
#define MIX_LINE_CURVE_POS      195
#define MIX_LINE_SWITCH_ICON    260
#define MIX_LINE_SWITCH_POS     280
#define MIX_LINE_DELAY_SLOW_POS 340
#define MIX_LINE_NAME_FM_ICON   370
#define MIX_LINE_NAME_FM_POS    390
#define MIX_LINE_SELECT_POS     50
#define MIX_LINE_SELECT_WIDTH   (LCD_W-MIX_LINE_SELECT_POS-15)
#define MIX_STATUS_BAR_W        130
#define MIX_STATUS_BAR_H        13
#define MIX_STATUS_CHAN_BAR     MENUS_MARGIN_LEFT + 45
#define MIX_STATUS_ICON_MIXER   MIX_STATUS_CHAN_BAR + 140
#define MIX_STATUS_ICON_TO      MIX_STATUS_ICON_MIXER + 20
#define MIX_STATUS_ICON_OUTPUT  MIX_STATUS_ICON_TO + 35
#define MIX_STATUS_OUT_NAME     MIX_STATUS_ICON_OUTPUT + 25
#define MIX_STATUS_OUT_BAR      LCD_W - MENUS_MARGIN_LEFT - MIX_STATUS_BAR_W

void lineMixSurround(coord_t y, LcdFlags flags=CURVE_AXIS_COLOR)
{
  lcdDrawRect(MIX_LINE_SELECT_POS, y-INVERT_VERT_MARGIN+1, MIX_LINE_SELECT_WIDTH, INVERT_LINE_HEIGHT, 1, s_copyMode == COPY_MODE ? SOLID : DOTTED, flags);
}

void onMixesMenu(const char * result)
{
  uint8_t chn = mixAddress(s_currIdx)->destCh + 1;

  if (result == STR_EDIT) {
    pushMenu(menuModelMixOne);
  }
  else if (result == STR_INSERT_BEFORE || result == STR_INSERT_AFTER) {
    if (!reachMixesLimit()) {
      s_currCh = chn;
      if (result == STR_INSERT_AFTER) { s_currIdx++; menuVerticalPosition++; }
      insertMix(s_currIdx);
      pushMenu(menuModelMixOne);
    }
  }
  else if (result == STR_COPY || result == STR_MOVE) {
    s_copyMode = (result == STR_COPY ? COPY_MODE : MOVE_MODE);
    s_copySrcIdx = s_currIdx;
    s_copySrcCh = chn;
    s_copySrcRow = menuVerticalPosition;
  }
  else if (result == STR_DELETE) {
    deleteMix(s_currIdx);
  }
}

void displayMixStatus(uint8_t channel)
{
  lcdDrawNumber(MENUS_MARGIN_LEFT, MENU_FOOTER_TOP, channel + 1, MENU_TITLE_COLOR, 0, "CH", NULL);
  drawSingleMixerBar(MIX_STATUS_CHAN_BAR, MENU_FOOTER_TOP + 4, MIX_STATUS_BAR_W, MIX_STATUS_BAR_H, channel);

  lcd->drawBitmap(MIX_STATUS_ICON_MIXER, MENU_FOOTER_TOP, mixerSetupMixerBitmap);
  lcd->drawBitmap(MIX_STATUS_ICON_TO, MENU_FOOTER_TOP, mixerSetupToBitmap);
  lcd->drawBitmap(MIX_STATUS_ICON_OUTPUT, MENU_FOOTER_TOP, mixerSetupOutputBitmap);

  if (g_model.limitData[channel].name[0] == '\0')
    lcdDrawNumber(MIX_STATUS_OUT_NAME, MENU_FOOTER_TOP, channel + 1, MENU_TITLE_COLOR, 0, "CH", NULL);
  else
    lcdDrawSizedText(MIX_STATUS_OUT_NAME, MENU_FOOTER_TOP, g_model.limitData[channel].name, sizeof(g_model.limitData[channel].name), MENU_TITLE_COLOR | LEFT | ZCHAR);
  drawSingleOutputBar(MIX_STATUS_OUT_BAR, MENU_FOOTER_TOP + 4, MIX_STATUS_BAR_W, MIX_STATUS_BAR_H, channel);
}

bool menuModelMixAll(event_t event)
{
  const BitmapBuffer * mpx_mode[] = {
    mixerSetupAddBitmap,
    mixerSetupMultiBitmap,
    mixerSetupReplaceBitmap
  };


  uint8_t sub = menuVerticalPosition;

  if (s_editMode > 0) {
    s_editMode = 0;
  }

  uint8_t chn = mixAddress(s_currIdx)->destCh + 1;

  int linesCount = getMixesLinesCount();
  SIMPLE_MENU(STR_MIXER, MODEL_ICONS, menuTabModel, MENU_MODEL_MIXES, linesCount);

  switch (event) {
    case EVT_ENTRY:
    case EVT_ENTRY_UP:
      s_copyMode = 0;
      s_copyTgtOfs = 0;
      break;
    case EVT_KEY_LONG(KEY_EXIT):
      if (s_copyMode && s_copyTgtOfs == 0) {
        deleteMix(s_currIdx);
        killEvents(event);
        event = 0;
      }
      // no break
    case EVT_KEY_BREAK(KEY_EXIT):
      if (s_copyMode) {
        if (s_copyTgtOfs) {
          // cancel the current copy / move operation
          if (s_copyMode == COPY_MODE) {
            deleteMix(s_currIdx);
          }
          else {
            do {
              swapMixes(s_currIdx, s_copyTgtOfs > 0);
              s_copyTgtOfs += (s_copyTgtOfs < 0 ? +1 : -1);
            } while (s_copyTgtOfs != 0);
            storageDirty(EE_MODEL);
          }
          menuVerticalPosition = s_copySrcRow;
          s_copyTgtOfs = 0;
        }
        s_copyMode = 0;
        event = 0;
      }
      break;
    case EVT_KEY_BREAK(KEY_ENTER):
      if ((!s_currCh || (s_copyMode && !s_copyTgtOfs)) && !READ_ONLY()) {
        s_copyMode = (s_copyMode == COPY_MODE ? MOVE_MODE : COPY_MODE);
        s_copySrcIdx = s_currIdx;
        s_copySrcCh = chn;
        s_copySrcRow = sub;
        break;
      }
      // no break

    case EVT_KEY_LONG(KEY_ENTER):
      killEvents(event);
      if (s_copyTgtOfs) {
        s_copyMode = 0;
        s_copyTgtOfs = 0;
        return true;
      }
      else {
        if (READ_ONLY()) {
          if (!s_currCh) {
            pushMenu(menuModelMixOne);
          }
        }
        else {
          if (s_copyMode) s_currCh = 0;
          if (s_currCh) {
            if (reachMixesLimit()) break;
            insertMix(s_currIdx);
            pushMenu(menuModelMixOne);
            s_copyMode = 0;
            return true;
          }
          else {
            event = 0;
            s_copyMode = 0;
            POPUP_MENU_ADD_ITEM(STR_EDIT);
            POPUP_MENU_ADD_ITEM(STR_INSERT_BEFORE);
            POPUP_MENU_ADD_ITEM(STR_INSERT_AFTER);
            POPUP_MENU_ADD_ITEM(STR_COPY);
            POPUP_MENU_ADD_ITEM(STR_MOVE);
            POPUP_MENU_ADD_ITEM(STR_DELETE);
            POPUP_MENU_START(onMixesMenu);
          }
        }
      }
      break;

    case EVT_ROTARY_RIGHT:
    case EVT_ROTARY_LEFT:
      if (s_copyMode) {
        uint8_t next_ofs = (event==EVT_ROTARY_LEFT ? s_copyTgtOfs - 1 : s_copyTgtOfs + 1);

        if (s_copyTgtOfs==0 && s_copyMode==COPY_MODE) {
          // insert a mix on the same channel (just above / just below)
          if (reachMixesLimit()) break;
          copyMix(s_currIdx);
          if (event==EVT_ROTARY_RIGHT) s_currIdx++;
          else if (sub-menuVerticalOffset >= 6) menuVerticalOffset++;
        }
        else if (next_ofs==0 && s_copyMode==COPY_MODE) {
          // delete the mix
          deleteMix(s_currIdx);
          if (event==EVT_ROTARY_LEFT) s_currIdx--;
        }
        else {
          // only swap the mix with its neighbor
          if (!swapMixes(s_currIdx, event==EVT_ROTARY_LEFT)) break;
          storageDirty(EE_MODEL);
        }

        s_copyTgtOfs = next_ofs;
      }
      break;
  }

  char str[6];
  strAppendUnsigned(strAppend(strAppendUnsigned(str, getMixesCount()), "/"), MAX_MIXERS, 2);
  lcdDrawText(MENU_TITLE_NEXT_POS, MENU_TITLE_TOP+1, str, HEADER_COLOR);

  sub = menuVerticalPosition;
  s_currCh = 0;
  int cur = 0;
  int i = 0;

  for (int ch=1; ch<=MAX_OUTPUT_CHANNELS; ch++) {
    MixData * md;
    coord_t y = MENU_CONTENT_TOP + (cur-menuVerticalOffset)*FH;
    if (i<MAX_MIXERS && (md=mixAddress(i))->srcRaw && md->destCh+1 == ch) {
      if (cur-menuVerticalOffset >= 0 && cur-menuVerticalOffset < NUM_BODY_LINES) {
        putsChn(MENUS_MARGIN_LEFT, y, ch, 0); // show CHx
      }
      uint8_t mixCnt = 0;
      do {
        if (s_copyMode) {
          if (s_copyMode == MOVE_MODE && cur-menuVerticalOffset >= 0 && cur-menuVerticalOffset < NUM_BODY_LINES && s_copySrcCh == ch && s_copyTgtOfs != 0 && i == (s_copySrcIdx + (s_copyTgtOfs<0))) {
            lineMixSurround(y);
            cur++; y+=FH;
          }
          if (s_currIdx == i) {
            sub = menuVerticalPosition = cur;
            s_currCh = ch;
          }
        }
        else if (sub == cur) {
          s_currIdx = i;
          displayMixStatus(ch - 1);
        }
        if (cur-menuVerticalOffset >= 0 && cur-menuVerticalOffset < NUM_BODY_LINES) {
          LcdFlags attr = ((s_copyMode || sub != cur) ? 0 : INVERS);

          if (mixCnt > 0) {
            lcd->drawBitmap(10, y, mpx_mode[md->mltpx]);
          }

          drawSource(MIX_LINE_SRC_POS, y, md->srcRaw);

          if (mixCnt == 0 && md->mltpx == 1)
            lcdDrawText(MIX_LINE_WEIGHT_POS, y, "MULT!", RIGHT | attr | (isMixActive(i) ? BOLD : 0));
          else
            gvarWeightItem(MIX_LINE_WEIGHT_POS, y, md, RIGHT | attr | (isMixActive(i) ? BOLD : 0), event);

          // displayMixLine(y, md);

          BitmapBuffer *delayslowbmp[] = {mixerSetupSlowBitmap, mixerSetupDelayBitmap, mixerSetupDelaySlowBitmap};
          uint8_t delayslow = 0;
          if (md->speedDown || md->speedUp)
            delayslow = 1;
          if (md->delayUp || md->delayDown)
            delayslow += 2;
          if (delayslow)
            lcd->drawBitmap(MIX_LINE_DELAY_SLOW_POS, y + 2, delayslowbmp[delayslow - 1]);

          if (s_copyMode) {
            if ((s_copyMode==COPY_MODE || s_copyTgtOfs == 0) && s_copySrcCh == ch && i == (s_copySrcIdx + (s_copyTgtOfs<0))) {
              /* draw a border around the raw on selection mode (copy/move) */
              lineMixSurround(y);
            }
            if (cur == sub) {
              /* invert the raw when it's the current one */
              lineMixSurround(y, ALARM_COLOR);
            }
          }
        }
        cur++; y+=FH; mixCnt++; i++; md++;
      } while (i<MAX_MIXERS && md->srcRaw && md->destCh+1 == ch);
      if (s_copyMode == MOVE_MODE && cur-menuVerticalOffset >= 0 && cur-menuVerticalOffset < NUM_BODY_LINES && s_copySrcCh == ch && i == (s_copySrcIdx + (s_copyTgtOfs<0))) {
        lineMixSurround(y);
        cur++; y+=FH;
      }
    }
    else {
      uint8_t attr = 0;
      if (sub == cur) {
        s_currIdx = i;
        s_currCh = ch;
        if (!s_copyMode) {
          attr = INVERS;
        }
      }
      if (cur-menuVerticalOffset >= 0 && cur-menuVerticalOffset < NUM_BODY_LINES) {
        putsChn(MENUS_MARGIN_LEFT, y, ch, attr); // show CHx
        if (s_copyMode == MOVE_MODE && s_copySrcCh == ch) {
          lineMixSurround(y);
        }
      }
      cur++; y+=FH;
    }
  }

  if (sub >= linesCount-1) menuVerticalPosition = linesCount-1;

  return true;
}
