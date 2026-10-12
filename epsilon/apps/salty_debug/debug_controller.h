#pragma once

#include <escher/view.h>
#include <escher/view_controller.h>
#include <kandinsky/color.h>
#include <stdint.h>

namespace SaltyDebug {

/* A page is a list of rows drawn under a pinned header. Rows are rebuilt each
 * time the page changes (the data comes straight from the Theme Area). */
class DebugView : public Escher::View {
 public:
  constexpr static int k_maxRows = 80;
  constexpr static int k_textLength = 45;  // 44 chars at 7px = 308px
  constexpr static int k_stripSwatchesPerRow = 22;

  struct Row {
    enum class Kind : uint8_t { Text, Section, Swatch, Strip };
    Kind kind;
    uint8_t stripCount;
    KDColor color;        // Text color, or swatch color for Kind::Swatch
    const uint32_t* strip;  // Kind::Strip: raw 0x00RRGGBB values
    char text[k_textLength];
  };

  DebugView() : m_numberOfRows(0), m_scroll(0) { m_header[0] = 0; }

  void drawRect(KDContext* ctx, KDRect rect) const override;

  // Rows
  void clearRows() {
    m_numberOfRows = 0;
    m_scroll = 0;
  }
  Row* addRow(Row::Kind kind, KDColor color);  // nullptr if the page is full
  int numberOfRows() const { return m_numberOfRows; }
  char* header() { return m_header; }

  // Scrolling
  int numberOfVisibleRows() const;
  void scrollBy(int rows);
  void redraw() { markWholeFrameAsDirty(); }

 private:
  constexpr static KDCoordinate k_scrollBarWidth = 3;
  static KDCoordinate RowHeight();
  void drawRow(KDContext* ctx, const Row& row, KDCoordinate y) const;

  int m_numberOfRows;
  int m_scroll;
  char m_header[k_textLength];
  Row m_rows[k_maxRows];
};

class DebugController : public Escher::ViewController {
 public:
  DebugController();
  Escher::View* view() override { return &m_view; }
  void viewWillAppear() override;
  bool handleEvent(Ion::Events::Event event) override;

 private:
  // Page 0 is the global page, page p >= 1 shows theme (p - 1)
  int numberOfPages() const;
  void setPage(int page);
  void buildPage();
  void buildGlobalPage();
  void buildThemePage(int theme);

  DebugView m_view;
  int m_page;
};

}  // namespace SaltyDebug
