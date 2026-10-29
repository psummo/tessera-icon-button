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

// The colour options as the theme's own colours: four tones of the screen and two of the Tessera mark.
enum class Paint : uint8_t { NORMAL, ACCENT, BUSY, ALERT, GOOD, PURPLE, CYAN };

static Paint paint_for(const char *name) {
  if (strcmp(name, "accent") == 0) return Paint::ACCENT;
  if (strcmp(name, "busy") == 0) return Paint::BUSY;
  if (strcmp(name, "alert") == 0) return Paint::ALERT;
  if (strcmp(name, "good") == 0) return Paint::GOOD;
  if (strcmp(name, "purple") == 0) return Paint::PURPLE;
  if (strcmp(name, "cyan") == 0) return Paint::CYAN;
  return Paint::NORMAL;
}

static tessera::Tone tone_of(Paint p) {
  switch (p) {
    case Paint::ACCENT: return tessera::Tone::ACCENT;
    case Paint::BUSY: return tessera::Tone::BUSY;
    case Paint::ALERT: return tessera::Tone::ALERT;
    case Paint::GOOD: return tessera::Tone::GOOD;
    default: return tessera::Tone::NORMAL;
  }
}

static theme::Role role_of(Paint p, theme::Role normal) {
  return p == Paint::PURPLE ? theme::MARK_PURPLE : p == Paint::CYAN ? theme::MARK_BLUE : normal;
}

static void paint_label(lv_obj_t *label, Paint p, theme::Role normal) {
  ui::set_tone(label, tone_of(p), role_of(p, normal));
}

static void paint_block(lv_obj_t *block, Paint p, theme::Role normal) {
  ui::set_fill(block, tone_of(p), role_of(p, normal));
}

// Both tiles: a script tile only sends a tap; a switch tile (follows == true) also follows the state of its entity.
class ButtonTile : public tessera::Tile {
 public:
  explicit ButtonTile(bool follows) : follows_(follows) {}

  void create(const tessera::TileContext &c) override {
    entity_ = c.entity ? c.entity : "";
    tone_ = paint_for(c.options["color"] | "normal");
    border_tone_ = paint_for(c.options["border"] | "none");
    has_border_ = strcmp(c.options["border"] | "none", "none") != 0;
    show_name_ = c.options["show_name"] | false;
    const std::string name = c.name ? c.name : "";
    const int width = c.width, height = c.height;

    // The frame: a block in the border's colour with a card-coloured block set in by the border's width, both made
    // before the icon so the icon is drawn on top.
    if (has_border_) {
      const char *w = c.options["border_width"] | "thin";
      const int t = ui::px(strcmp(w, "thick") == 0 ? 5 : strcmp(w, "normal") == 0 ? 3 : 2);
      outer_ = ui::block(c.parent, theme::CARD);
      lv_obj_set_size(outer_, width, height);
      lv_obj_set_pos(outer_, 0, 0);
      lv_obj_t *inner = ui::block(c.parent, theme::CARD);
      lv_obj_set_size(inner, width - 2 * t, height - 2 * t);
      lv_obj_set_pos(inner, t, t);
    }

    const Font face = strcmp(c.options["icon_size"] | "normal", "small") == 0 ? Font::ICON_SMALL : Font::ICON;
    icon_ = ui::label(c.parent, face, theme::INK);
    ui::set_text(icon_, ui::icon(icon_code(c.options["icon"] | (follows_ ? "lightbulb" : "plus"))));
    lv_obj_set_width(icon_, width);
    lv_obj_set_style_text_align(icon_, LV_TEXT_ALIGN_CENTER, 0);
    const int ih = ui::line_height(face);

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
    apply();
  }

  void on_state(JsonObjectConst data) override {
    if (!follows_) return;
    const char *s = data["state"] | "";
    on_ = strcmp(s, "on") == 0;
    gone_ = strcmp(s, "unavailable") == 0 || strcmp(s, "unknown") == 0;
    apply();
  }

  void on_tap() override {
    if (entity_.empty()) return;
    if (!follows_) {
      tessera::action("script.turn_on", entity_);
    } else if (entity_.rfind("light.", 0) == 0) {
      tessera::action("light.toggle", entity_);
    } else if (entity_.rfind("input_boolean.", 0) == 0) {
      tessera::action("input_boolean.toggle", entity_);
    } else if (entity_.rfind("fan.", 0) == 0) {
      tessera::action("fan.toggle", entity_);
    } else {
      tessera::action("switch.toggle", entity_);
    }
  }

  void on_theme() override { apply(); }

 private:
  // Colours from the current state: a script tile always shows its colours, a switch tile only while it is on.
  void apply() {
    const bool lit = !follows_ || on_;
    if (entity_.empty() || gone_) {
      paint_label(icon_, Paint::NORMAL, theme::MUTED);
    } else {
      paint_label(icon_, lit ? tone_ : Paint::NORMAL, theme::INK);
    }
    if (outer_) paint_block(outer_, lit ? border_tone_ : Paint::NORMAL, theme::TRACK);
    if (name_) ui::set_color(name_, theme::MUTED);
  }

  std::string entity_;
  bool follows_;
  bool show_name_ = false, has_border_ = false, on_ = false, gone_ = false;
  Paint tone_ = Paint::NORMAL, border_tone_ = Paint::NORMAL;
  lv_obj_t *icon_{}, *name_{}, *outer_{};
};

void IconButton::setup() {
  add_tile("button", [this]() { return new ButtonTile(false); });
  add_tile("switch", [this]() { return new ButtonTile(true); });
  add_tile("light", [this]() { return new ButtonTile(true); });
}

}  // namespace esphome::icon_button
