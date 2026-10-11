#include "debug_controller.h"

#include <escher/palette.h>
#include <ion.h>
#include <ion/events.h>
#include <ion/external_apps.h>
#include <string.h>

#include "../apps_container.h"
#include "../salty_version.h"
#include "../shortcuts.h"
#include "../theme_manager.h"

#if PLATFORM_DEVICE
// Defined by the linker script, see userland_shared.ld
extern char _external_apps_RAM_start;
extern char _external_apps_RAM_end;
#endif

namespace SaltyDebug {

using Kind = DebugView::Row::Kind;
using namespace Ion::Device::Board::Config;

/* Colors ------------------------------------------------------------------ */

// Fixed colors (not the theme palette) so that the screen stays readable
// whatever the theme being inspected.
static const KDColor k_text = KDColorBlack;
static const KDColor k_good = KDColor::RGB24(0x1b7a2f);
static const KDColor k_bad = KDColor::RGB24(0xc0262d);
static const KDColor k_dim = KDColor::RGB24(0x707070);
static const KDColor k_sectionBackground = KDColor::RGB24(0x1f3a6e);
static const KDColor k_headerBackground = KDColor::RGB24(0xd9d9d9);

/* Names ------------------------------------------------------------------- */

// Order = Escher::Palette::s_paletteSlots (palette.txt slot order)
static const char* const k_paletteSlotNames[] = {
    "YellowDark",   "YellowLightBattery", "YellowLight",     "PurpleBright",
    "PurpleDark",   "GrayWhite",          "GrayBright",      "GrayMiddle",
    "GrayDarkMiddle", "GrayDark",         "GrayVeryDark",    "GrayDarkest",
    "Select",       "SelectDark",         "WallScreen",      "WallScreenDark",
    "PopUpTitleBackground", "LowBattery", "Red",             "RedLight",
    "Magenta",      "MagentaLight",       "Turquoise",       "TurquoiseLight",
    "Pink",         "PinkLight",          "Blue",            "BlueLight",
    "Orange",       "OrangeLight",        "Green",           "GreenLight",
    "Brown",        "Purple",             "BlueishGray",     "Cyan",
    "Violet",       "VioletLight",        "Mint",            "MintLight",
    "TextColor",    "TextColorHover",     "WallpaperColor"};
static_assert(sizeof(k_paletteSlotNames) / sizeof(k_paletteSlotNames[0]) ==
                  Escher::Palette::numberOfPaletteSlots(),
              "Update k_paletteSlotNames when palette slots change");

// Order = ThemeManager::iconIndexForApp()
static const char* const k_iconNames[] = {
    "Calculation", "Python",     "Distributions", "Elements",
    "Finance",     "Grapher",    "Inference",     "Regression",
    "Sequences",   "Settings",   "Solver",        "Statistics"};
static_assert(sizeof(k_iconNames) / sizeof(k_iconNames[0]) == ThemeIconCount,
              "Update k_iconNames when the number of theme icons changes");

/* Shortcut key names come from the macros of shortcuts.h, so they follow any
 * compile-time remapping: "Ion::Keyboard::Key::Alpha" -> "Alpha". */
#define SALTY_STRINGIFY_(x) #x
#define SALTY_STRINGIFY(x) SALTY_STRINGIFY_(x)

static const char* ShortKeyName(const char* qualifiedName) {
  const char* last = qualifiedName;
  for (const char* c = qualifiedName; *c != 0; c++) {
    if (*c == ':') {
      last = c + 1;
    }
  }
  return last;
}

/* Tiny text formatter (there is no snprintf in liba). A Line with a null
 * buffer silently drops everything, used when the page is full. */
class Line {
 public:
  Line(char* buffer, int capacity)
      : m_buffer(buffer), m_capacity(capacity), m_length(0) {
    if (m_buffer != nullptr) {
      m_buffer[0] = 0;
    }
  }
  Line& str(const char* s) {
    while (s != nullptr && *s != 0) {
      push(*s++);
    }
    return *this;
  }
  Line& dec(uint32_t value) {
    char digits[10];
    int n = 0;
    do {
      digits[n++] = '0' + value % 10;
      value /= 10;
    } while (value != 0);
    while (n > 0) {
      push(digits[--n]);
    }
    return *this;
  }
  Line& hex(uint32_t value, int minDigits = 1) {
    char digits[8];
    int n = 0;
    do {
      digits[n++] = "0123456789ABCDEF"[value & 0xF];
      value >>= 4;
    } while (value != 0);
    for (int i = n; i < minDigits; i++) {
      push('0');
    }
    while (n > 0) {
      push(digits[--n]);
    }
    return *this;
  }
  // "812 B" or "12.3 KiB"
  Line& size(uint32_t bytes) {
    if (bytes < 1024) {
      return dec(bytes).str(" B");
    }
    return dec(bytes / 1024).str(".").dec((bytes % 1024) * 10 / 1024).str(
        " KiB");
  }
  Line& yesNo(bool value) { return str(value ? "yes" : "no"); }

 private:
  void push(char c) {
    if (m_buffer != nullptr && m_length < m_capacity - 1) {
      m_buffer[m_length++] = c;
      m_buffer[m_length] = 0;
    }
  }
  char* m_buffer;
  int m_capacity;
  int m_length;
};

static Line NewLine(DebugView* view, Kind kind, KDColor color = k_text) {
  DebugView::Row* row = view->addRow(kind, color);
  if (row == nullptr) {
    return Line(nullptr, 0);
  }
  return Line(row->text, DebugView::k_textLength);
}

static void Section(DebugView* view, const char* title) {
  NewLine(view, Kind::Section).str(title);
}

static void NameToText(const ThemeEntry* e, char* out /* ThemeNameLength+1 */) {
  memcpy(out, e->name, ThemeNameLength);
  out[ThemeNameLength] = 0;
}

static uint32_t ThemeDataSize(const ThemeEntry* e) {
  uint32_t total = e->wallpaperSize + e->paletteSize;
  for (uint32_t i = 0; i < ThemeIconCount; i++) {
    total += e->iconSizes[i];
  }
  return total;
}

static uint32_t NumberOfIcons(const ThemeEntry* e) {
  uint32_t count = 0;
  if (e->isIcons) {
    for (uint32_t i = 0; i < ThemeIconCount; i++) {
      if (e->iconOffsets[i] != 0) {
        count++;
      }
    }
  }
  return count;
}

static bool HasPalette(const ThemeEntry* e) {
  return e->isPalette && e->paletteOffset != 0;
}

/* DebugView --------------------------------------------------------------- */

KDCoordinate DebugView::RowHeight() {
  return KDFont::GlyphHeight(KDFont::Size::Small);
}

DebugView::Row* DebugView::addRow(Row::Kind kind, KDColor color) {
  if (m_numberOfRows >= k_maxRows) {
    return nullptr;
  }
  Row* row = &m_rows[m_numberOfRows++];
  row->kind = kind;
  row->stripCount = 0;
  row->color = color;
  row->strip = nullptr;
  row->text[0] = 0;
  return row;
}

int DebugView::numberOfVisibleRows() const {
  // The first line is the pinned header
  int rows = bounds().height() / RowHeight() - 1;
  return rows > 1 ? rows : 1;
}

void DebugView::scrollBy(int rows) {
  int maxScroll = m_numberOfRows - numberOfVisibleRows();
  if (maxScroll < 0) {
    maxScroll = 0;
  }
  int scroll = m_scroll + rows;
  scroll = scroll < 0 ? 0 : (scroll > maxScroll ? maxScroll : scroll);
  if (scroll != m_scroll) {
    m_scroll = scroll;
    markWholeFrameAsDirty();
  }
}

void DebugView::drawRow(KDContext* ctx, const Row& row, KDCoordinate y) const {
  const KDCoordinate width = bounds().width() - k_scrollBarWidth;
  const KDCoordinate height = RowHeight();
  KDGlyph::Style style;
  style.font = KDFont::Size::Small;
  style.glyphColor = row.color;
  style.backgroundColor = KDColorWhite;

  switch (row.kind) {
    case Row::Kind::Section:
      style.glyphColor = KDColorWhite;
      style.backgroundColor = k_sectionBackground;
      ctx->fillRect(KDRect(0, y, width, height), k_sectionBackground);
      ctx->drawString(row.text, KDPoint(4, y), style);
      break;
    case Row::Kind::Swatch: {
      // 12x12 swatch with a black border, then the text
      ctx->fillRect(KDRect(4, y + 1, 12, 12), KDColorBlack);
      ctx->fillRect(KDRect(5, y + 2, 10, 10), row.color);
      style.glyphColor = k_text;
      ctx->drawString(row.text, KDPoint(22, y), style);
      break;
    }
    case Row::Kind::Strip:
      for (int i = 0; i < row.stripCount; i++) {
        ctx->fillRect(KDRect(4 + i * 14, y + 1, 13, 12),
                      KDColor::RGB24(row.strip[i]));
      }
      break;
    case Row::Kind::Text:
      ctx->drawString(row.text, KDPoint(4, y), style);
      break;
  }
}

void DebugView::drawRect(KDContext* ctx, KDRect rect) const {
  ctx->fillRect(rect, KDColorWhite);
  const KDCoordinate width = bounds().width();
  const KDCoordinate height = RowHeight();

  // Pinned header
  KDGlyph::Style headerStyle;
  headerStyle.font = KDFont::Size::Small;
  headerStyle.glyphColor = k_text;
  headerStyle.backgroundColor = k_headerBackground;
  ctx->fillRect(KDRect(0, 0, width, height), k_headerBackground);
  ctx->drawString(m_header, KDPoint(4, 0), headerStyle);

  const int visibleRows = numberOfVisibleRows();
  for (int i = 0; i < visibleRows; i++) {
    int index = m_scroll + i;
    if (index >= m_numberOfRows) {
      break;
    }
    KDCoordinate y = (i + 1) * height;
    if (y + height <= rect.top() || y >= rect.bottom()) {
      continue;
    }
    drawRow(ctx, m_rows[index], y);
  }

  // Scroll bar
  if (m_numberOfRows > visibleRows) {
    const KDCoordinate trackTop = height;
    const KDCoordinate trackHeight = bounds().height() - height;
    KDCoordinate thumbHeight = trackHeight * visibleRows / m_numberOfRows;
    if (thumbHeight < 6) {
      thumbHeight = 6;
    }
    KDCoordinate thumbTop =
        trackTop + (trackHeight - thumbHeight) * m_scroll /
                       (m_numberOfRows - visibleRows);
    ctx->fillRect(KDRect(width - k_scrollBarWidth, thumbTop, k_scrollBarWidth,
                         thumbHeight),
                  k_dim);
  }
}

/* DebugController --------------------------------------------------------- */

DebugController::DebugController() : ViewController(nullptr), m_page(0) {}

int DebugController::numberOfPages() const {
  return 1 + ThemeManager::themeCount();
}

void DebugController::viewWillAppear() {
  m_page = 0;
  buildPage();
}

void DebugController::setPage(int page) {
  int pages = numberOfPages();
  m_page = (page % pages + pages) % pages;
  buildPage();
  m_view.redraw();
}

bool DebugController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Left) {
    setPage(m_page - 1);
    return true;
  }
  if (event == Ion::Events::Right) {
    setPage(m_page + 1);
    return true;
  }
  if (event == Ion::Events::Up) {
    m_view.scrollBy(-1);
    return true;
  }
  if (event == Ion::Events::Down) {
    m_view.scrollBy(1);
    return true;
  }
  if (event == Ion::Events::ShiftUp) {
    m_view.scrollBy(-m_view.numberOfVisibleRows());
    return true;
  }
  if (event == Ion::Events::ShiftDown) {
    m_view.scrollBy(m_view.numberOfVisibleRows());
    return true;
  }
  if ((event == Ion::Events::OK || event == Ion::Events::EXE) &&
      ThemeManager::isValid()) {
    // Jump to the page of the active theme
    setPage(1 + ThemeManager::selectedTheme());
    return true;
  }
  return false;
}

void DebugController::buildPage() {
  m_view.clearRows();
  if (m_page == 0) {
    buildGlobalPage();
  } else {
    buildThemePage(m_page - 1);
  }
}

void DebugController::buildGlobalPage() {
  DebugView* v = &m_view;
  const int pages = numberOfPages();
  Line(v->header(), DebugView::k_textLength)
      .str("Salty-OS debug  1/")
      .dec(pages)
      .str("  <> pages  ^v scroll");

  Section(v, "Salty-OS");
  NewLine(v, Kind::Text).str("Version: ").str(SALTY_OS_VERSION);
  NewLine(v, Kind::Text)
      .str("Based on Epsilon: ")
      .str(SALTY_OS_EPSILON_BASE_VERSION);
  NewLine(v, Kind::Text).str("Reported version: ").str(Ion::softwareVersion());
  NewLine(v, Kind::Text).str("Commit: ").str(Ion::commitHash());
  NewLine(v, Kind::Text).str("Built: ").str(__DATE__);
  NewLine(v, Kind::Text).str("PCB: ").str(Ion::pcbVersion());

  Section(v, "Theme Area");
  const uint32_t* raw = reinterpret_cast<const uint32_t*>(ThemeAreaStart);
  const bool valid = ThemeManager::isValid();
  NewLine(v, Kind::Text, valid ? k_good : k_bad)
      .str("Status: ")
      .str(valid ? "VALID" : "INVALID (no theme loaded)");
  NewLine(v, Kind::Text).str("Address: 0x").hex(ThemeAreaStart, 8);
  NewLine(v, Kind::Text).str("Capacity: ").size(ThemeAreaSize);
  NewLine(v, Kind::Text, raw[0] == ThemeAreaHeaderMagic ? k_text : k_bad)
      .str("Magic: 0x")
      .hex(raw[0], 8)
      .str(" (expected 0x")
      .hex(ThemeAreaHeaderMagic, 8)
      .str(")");
  NewLine(v, Kind::Text, raw[1] == ThemeAreaHeaderVersion ? k_text : k_bad)
      .str("Format: v")
      .dec(raw[1])
      .str(" (expected v")
      .dec(ThemeAreaHeaderVersion)
      .str(")");
  if (valid) {
    const int flashed = ThemeManager::themeCount() - 1;
    NewLine(v, Kind::Text)
        .str("Themes: ")
        .dec(flashed)
        .str(" flashed (max ")
        .dec(ThemeCount)
        .str(") + Default");
    const int selected = ThemeManager::selectedTheme();
    Line selectedLine = NewLine(v, Kind::Text);
    selectedLine.str("Selected: ").dec(selected).str(" ");
    if (selected == 0) {
      selectedLine.str("Default");
    } else {
      char name[ThemeNameLength + 1];
      NameToText(ThemeManager::entry(selected - 1), name);
      selectedLine.str(name);
    }
    NewLine(v, Kind::Text)
        .str("Icon slots: ")
        .dec(ThemeIconCount)
        .str("  Palette slots: ")
        .dec(Escher::Palette::numberOfPaletteSlots());
  }

  Section(v, "Memory");
#if PLATFORM_DEVICE
  const uint32_t externalRAM = static_cast<uint32_t>(
      &_external_apps_RAM_end - &_external_apps_RAM_start);
  NewLine(v, Kind::Text)
      .str("Ext. apps RAM: ")
      .dec(externalRAM)
      .str(" B (")
      .size(externalRAM)
      .str(")");
#else
  NewLine(v, Kind::Text, k_dim).str("Ext. apps RAM: n/a (simulator)");
#endif
  NewLine(v, Kind::Text)
      .str("External apps: ")
      .dec(Ion::ExternalApps::numberOfApps(false))
      .str(AppsContainer::sharedAppsContainer()->externalAppsHidden()
               ? " (hidden)"
               : " (shown)");

  Section(v, "Shortcuts");
  NewLine(v, Kind::Text)
      .str("Hold ")
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_PREFIX_KEY_1)))
      .str(" + ")
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_PREFIX_KEY_2)));
  NewLine(v, Kind::Text)
      .str("  Previous theme: ")
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_PREVIOUS_THEME)));
  NewLine(v, Kind::Text)
      .str("  Next theme: ")
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_NEXT_THEME)));
  NewLine(v, Kind::Text)
      .str("  Toggle ext. apps: ")
      .str(ShortKeyName(
          SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_TOGGLE_EXTERNAL_APPS)));
  NewLine(v, Kind::Text)
      .str("  Debug screen: ")
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_OPEN_DEBUG)));

  if (valid) {
    Section(v, "All themes (OK = go to active)");
    NewLine(v, Kind::Text, k_dim).str("00 Default (builtin)");
    for (int t = 0; t < ThemeManager::themeCount() - 1; t++) {
      const ThemeEntry* e = ThemeManager::entry(t);
      if (e == nullptr) {
        break;
      }
      char name[ThemeNameLength + 1];
      NameToText(e, name);
      Line line = NewLine(v, Kind::Text);
      if (t + 1 < 10) {
        line.str("0");
      }
      line.dec(t + 1)
          .str(" ")
          .str(name)
          .str(e->isWallpaper ? "  W" : "  -")
          .str(HasPalette(e) ? "P" : "-")
          .str("I")
          .dec(NumberOfIcons(e))
          .str("  ")
          .size(ThemeDataSize(e));
    }
  }
}

void DebugController::buildThemePage(int theme) {
  DebugView* v = &m_view;
  const int count = ThemeManager::themeCount();
  const bool isDefault = theme == 0;
  const ThemeEntry* e = isDefault ? nullptr : ThemeManager::entry(theme - 1);
  const bool active = ThemeManager::selectedTheme() == theme;

  char name[ThemeNameLength + 1];
  if (isDefault || e == nullptr) {
    memcpy(name, "Default", 8);
  } else {
    NameToText(e, name);
  }

  Line(v->header(), DebugView::k_textLength)
      .str("Theme ")
      .dec(theme + 1)
      .str("/")
      .dec(count)
      .str(": ")
      .str(name)
      .str(active ? " [ACTIVE]" : "");

  Section(v, "General");
  NewLine(v, Kind::Text).str("Name: ").str(name);
  Line indexLine = NewLine(v, Kind::Text);
  indexLine.str("Index: ").dec(theme);
  if (isDefault) {
    indexLine.str(" (virtual, not flashed)");
  } else {
    indexLine.str(" (flashed slot ").dec(theme - 1).str(")");
  }
  NewLine(v, Kind::Text, active ? k_good : k_dim)
      .str("Active: ")
      .yesNo(active);

  if (e == nullptr) {
    NewLine(v, Kind::Text, k_dim).str("Builtin Epsilon look:");
    NewLine(v, Kind::Text, k_dim).str("no wallpaper, palette or icons,");
    NewLine(v, Kind::Text, k_dim).str("compiled-in defaults are used.");
    return;
  }

  NewLine(v, Kind::Text).str("Circle level: ").dec(e->defaultCircleLevel);
  NewLine(v, Kind::Text)
      .str("App name hover background: ")
      .yesNo(e->isAppNameBackgroundHover);
  NewLine(v, Kind::Text)
      .str("Flags: wallpaper=")
      .dec(e->isWallpaper)
      .str(" palette=")
      .dec(e->isPalette)
      .str(" icons=")
      .dec(e->isIcons);
  NewLine(v, Kind::Text).str("Total data: ").size(ThemeDataSize(e));

  Section(v, "Wallpaper");
  ThemeManager::WallpaperInfo info;
  if (ThemeManager::wallpaperInfo(e, &info)) {
    NewLine(v, Kind::Text, k_good)
        .str("Size: ")
        .dec(info.width)
        .str("x")
        .dec(info.height)
        .str(" px");
    NewLine(v, Kind::Text)
        .str("Chunks: ")
        .dec(info.chunkCount)
        .str(" x ")
        .dec(info.chunkRows)
        .str(" rows");
    NewLine(v, Kind::Text)
        .str("Compressed: ")
        .size(e->wallpaperSize)
        .str(" / raw ")
        .size(info.width * info.height * sizeof(KDColor));
    NewLine(v, Kind::Text).str("Offset: 0x").hex(e->wallpaperOffset);
  } else {
    NewLine(v, Kind::Text, k_dim).str("None");
  }

  Section(v, "Palette");
  if (HasPalette(e)) {
    const uint32_t* colors =
        reinterpret_cast<const uint32_t*>(ThemeAreaStart + e->paletteOffset);
    const uint32_t colorCount = e->paletteSize / sizeof(uint32_t);
    const uint32_t slots = Escher::Palette::numberOfPaletteSlots();
    NewLine(v, Kind::Text, colorCount == slots ? k_good : k_bad)
        .str("Colors: ")
        .dec(colorCount)
        .str(" / ")
        .dec(slots)
        .str(" slots (")
        .size(e->paletteSize)
        .str(")");
    NewLine(v, Kind::Text).str("Offset: 0x").hex(e->paletteOffset);
    const uint32_t shown = colorCount < slots ? colorCount : slots;
    // Overview: all colors as swatches
    for (uint32_t first = 0; first < shown;
         first += DebugView::k_stripSwatchesPerRow) {
      DebugView::Row* row = v->addRow(Kind::Strip, k_text);
      if (row == nullptr) {
        break;
      }
      uint32_t n = shown - first;
      row->stripCount = n < DebugView::k_stripSwatchesPerRow
                            ? n
                            : DebugView::k_stripSwatchesPerRow;
      row->strip = colors + first;
    }
    // Detail: slot name and value
    for (uint32_t i = 0; i < shown; i++) {
      DebugView::Row* row = v->addRow(Kind::Swatch, KDColor::RGB24(colors[i]));
      if (row == nullptr) {
        break;
      }
      Line line(row->text, DebugView::k_textLength);
      if (i < 10) {
        line.str("0");
      }
      line.dec(i).str(" ").str(k_paletteSlotNames[i]).str("  #").hex(
          colors[i] & 0xFFFFFF, 6);
    }
  } else {
    NewLine(v, Kind::Text, k_dim).str("None (Escher defaults)");
  }

  Section(v, "Icons");
  const uint32_t iconCount = NumberOfIcons(e);
  NewLine(v, Kind::Text, iconCount > 0 ? k_good : k_dim)
      .str("Present: ")
      .dec(iconCount)
      .str(" / ")
      .dec(ThemeIconCount);
  for (uint32_t i = 0; i < ThemeIconCount; i++) {
    const bool present = e->isIcons && e->iconOffsets[i] != 0;
    Line line = NewLine(v, Kind::Text, present ? k_text : k_dim);
    if (i < 10) {
      line.str("0");
    }
    line.dec(i).str(" ").str(k_iconNames[i]);
    if (present) {
      line.str(": ").size(e->iconSizes[i]).str("  @0x").hex(e->iconOffsets[i]);
    } else {
      line.str(": -");
    }
  }
}

}  // namespace SaltyDebug
