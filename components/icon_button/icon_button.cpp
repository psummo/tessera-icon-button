#include "icon_button.h"
#include <cstring>
#include <string>

namespace esphome::icon_button {

using tessera::Font;
namespace ui = tessera::ui;

// The icons the editor offers (the choice values of the manifest) and their codepoints in Tessera's icon set.
struct IconEntry {
  const char *name;
  uint32_t code;
};
static const IconEntry ICONS[] = {
    {"plus", 0xF0415},
    {"minus", 0xF0374},
    {"chevron_up", 0xF0143},
    {"chevron_down", 0xF0140},
    {"arrow_up", 0xF005D},
    {"arrow_down", 0xF0045},
    {"fire", 0xF0238},
    {"power", 0xF0425},
    {"play", 0xF040A},
    {"pause", 0xF03E4},
    {"stop", 0xF04DB},
    {"check", 0xF012C},
    {"close", 0xF0156},
    {"lock", 0xF033E},
    {"lock_open", 0xF033F},
    {"bell", 0xF009A},
    {"home", 0xF02DC},
    {"volume_plus", 0xF075D},
    {"volume_minus", 0xF075E},
    {"skip_next", 0xF04AD},
    {"skip_previous", 0xF04AE},
    {"thermometer", 0xF050F},
    {"thermostat", 0xF0393},
    {"radiator", 0xF0438},
    {"snowflake", 0xF0717},
    {"fan", 0xF0210},
    {"lightbulb", 0xF0335},
    {"flash", 0xF0241},
    {"script", 0xF0BC2},
    {"tap", 0xF12A8},
};

static uint32_t icon_code(const char *name) {
  for (const IconEntry &e : ICONS)
    if (strcmp(e.name, name) == 0) return e.code;
  return ICONS[0].code;
}

// The colour option (normal, accent, busy, alert, good) as a tone of the theme.
static tessera::Tone tone_for(const char *name) {
  if (strcmp(name, "accent") == 0) return tessera::Tone::ACCENT;
  if (strcmp(name, "busy") == 0) return tessera::Tone::BUSY;
  if (strcmp(name, "alert") == 0) return tessera::Tone::ALERT;
  if (strcmp(name, "good") == 0) return tessera::Tone::GOOD;
  return tessera::Tone::NORMAL;
}

class ButtonTile : public tessera::Tile {
 public:
  void create(const tessera::TileContext &c) override {
    entity_ = c.entity ? c.entity : "";
    tone_ = tone_for(c.options["color"] | "normal");
    show_name_ = c.options["show_name"] | false;
    const std::string name = c.name ? c.name : "";
    const int width = c.width, height = c.height;

    // A thin frame: a block in the border's colour with a card-coloured block inside it, both made before the icon so the
    // icon is drawn on top.
    const char *border = c.options["border"] | "none";
    if (strcmp(border, "none") != 0) {
      const char *w = c.options["border_width"] | "thin";
      const int t = ui::px(strcmp(w, "thick") == 0 ? 5 : strcmp(w, "normal") == 0 ? 3 : 2);
      lv_obj_t *outer = ui::block(c.parent, theme::CARD);
      ui::set_fill(outer, tone_for(border), theme::CARD);
      lv_obj_set_size(outer, width, height);
      lv_obj_set_pos(outer, 0, 0);
      lv_obj_t *inner = ui::block(c.parent, theme::CARD);
      lv_obj_set_size(inner, width - 2 * t, height - 2 * t);
      lv_obj_set_pos(inner, t, t);
    }

    icon_ = ui::label(c.parent, Font::ICON, entity_.empty() ? theme::MUTED : theme::INK);
    ui::set_tone(icon_, entity_.empty() ? tessera::Tone::NORMAL : tone_, entity_.empty() ? theme::MUTED : theme::INK);
    ui::set_text(icon_, ui::icon(icon_code(c.options["icon"] | "plus")));
    lv_obj_set_width(icon_, width);
    lv_obj_set_style_text_align(icon_, LV_TEXT_ALIGN_CENTER, 0);
    const int ih = ui::line_height(Font::ICON);

    int nh = 0;
    if (show_name_ && !name.empty()) {
      name_ = ui::label(c.parent, Font::BODY, theme::MUTED);
      ui::set_text(name_, name);
      lv_obj_set_width(name_, width);
      lv_obj_set_style_text_align(name_, LV_TEXT_ALIGN_CENTER, 0);
      nh = ui::line_height(Font::BODY);
    }
    const int top = (height - ih - nh) / 2;
    lv_obj_set_pos(icon_, 0, top);
    if (name_) lv_obj_set_pos(name_, 0, top + ih);
  }

  void on_tap() override {
    if (!entity_.empty()) tessera::action("script.turn_on", entity_);
  }

  void on_theme() override {
    ui::set_tone(icon_, entity_.empty() ? tessera::Tone::NORMAL : tone_, entity_.empty() ? theme::MUTED : theme::INK);
    if (name_) ui::set_color(name_, theme::MUTED);
  }

 private:
  std::string entity_;
  tessera::Tone tone_ = tessera::Tone::NORMAL;
  bool show_name_ = false;
  lv_obj_t *icon_{}, *name_{};
};

void IconButton::setup() { add_tile("button", [this]() { return new ButtonTile(); }); }

}  // namespace esphome::icon_button
