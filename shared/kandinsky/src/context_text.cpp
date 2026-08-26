#include <assert.h>
#include <kandinsky/context.h>
#include <kandinsky/font.h>
#include <omg/utf8_decoder.h>

#include <cmath>

constexpr static int k_tabCharacterWidth = 4;

KDPoint KDContext::alignAndDrawSingleLineString(const char* text, KDPoint p,
                                                KDSize frame,
                                                float horizontalAlignment,
                                                KDGlyph::Style style,
                                                int maxLength) {
  KDSize textSize = KDFont::Font(style.font)->stringSize(text, maxLength);
  assert(textSize.width() <= frame.width() &&
         textSize.height() <= frame.height());
  KDPoint origin(p.x() + std::round(horizontalAlignment *
                                    (frame.width() - textSize.width())),
                 p.y());
  return drawString(text, origin, style, maxLength);
}

KDPoint KDContext::alignAndDrawString(const char* text, KDPoint p, KDSize frame,
                                      KDGlyph::Format format, int maxLength,
                                      KDCoordinate lineSpacing) {
  assert(format.horizontalAlignment >= 0.0f &&
         format.horizontalAlignment <= 1.0f &&
         format.verticalAlignment >= 0.0f && format.verticalAlignment <= 1.0f);
  /* Align vertically
   * Then split lines and horizontal-align each independently */
  KDSize textSize =
      KDFont::Font(format.style.font)->stringSize(text, maxLength, lineSpacing);
  assert(textSize.width() <= frame.width() &&
         textSize.height() <= frame.height());
  // We ceil vertical alignment to prefer shifting down than up.
  KDPoint origin(p.x(),
                 p.y() + std::ceil(format.verticalAlignment *
                                   (frame.height() - textSize.height())));
  KDSize lineFrame =
      KDSize(frame.width(), KDFont::GlyphHeight(format.style.font));

  UTF8Decoder decoder(text);
  const char* startLine = text;
  CodePoint codePoint = decoder.nextCodePoint();
  const char* codePointPointer = decoder.stringPosition();
  while (codePoint != UCodePointNull &&
         (maxLength < 0 || codePointPointer < text + maxLength)) {
    if (codePoint == UCodePointLineFeed) {
      alignAndDrawSingleLineString(startLine, origin, lineFrame,
                                   format.horizontalAlignment, format.style,
                                   codePointPointer - startLine - 1);
      startLine = codePointPointer;
      origin =
          KDPoint(origin.x(), origin.y() + lineFrame.height() + lineSpacing);
    }
    codePoint = decoder.nextCodePoint();
    codePointPointer = decoder.stringPosition();
  }
  // Last line
  return alignAndDrawSingleLineString(startLine, origin, frame,
                                      format.horizontalAlignment, format.style,
                                      maxLength + text - startLine);
}

KDPoint KDContext::drawString(const char* text, KDPoint p, KDGlyph::Style style,
                              int maxByteLength) {
  KDPoint position = p;
  KDCoordinate glyphHeight = KDFont::GlyphHeight(style.font);
  static KDFont::RenderPalette palette =
      KDFont::Palette(KDColorBlack, KDColorBlack);
  if (palette.from() != style.backgroundColor ||
      palette.to() != style.glyphColor) {
    palette = KDFont::Palette(style.glyphColor, style.backgroundColor);
  }
  KDFont::GlyphBuffer glyphBuffer;

  UTF8Decoder decoder(text);
  const char* codePointPointer = decoder.stringPosition();
  CodePoint codePoint = decoder.nextCodePoint();
  while (codePoint != UCodePointNull &&
         (maxByteLength < 0 || codePointPointer < text + maxByteLength)) {
    codePointPointer = decoder.stringPosition();
    if (codePoint == UCodePointLineFeed) {
      assert(position.y() < KDCOORDINATE_MAX - glyphHeight);
      position = KDPoint(origin().x(), position.y() + glyphHeight);
      if (origin().y() + position.y() > clippingRect().bottom()) {
        break;
      }
      codePoint = decoder.nextCodePoint();
    } else if (codePoint == UCodePointCarriageReturn) {
      // Ignore '\r' that are added for compatibility
      codePoint = decoder.nextCodePoint();
    } else if (codePoint == UCodePointTabulation) {
      position = position.translatedBy(
          KDPoint(k_tabCharacterWidth * KDFont::GlyphMaxWidth(style.font), 0));
      codePoint = decoder.nextCodePoint();
    } else if (codePoint.isCombining()) {
      /* Ignore combining codepoints at the start of a line that
       * unsanitized calls (from micropython for instance) may
       * contain. */
      codePoint = decoder.nextCodePoint();
    } else {
      assert(!codePoint.isCombining());
      KDCoordinate width = KDFont::GlyphWidth(style.font, codePoint);
      if (origin().x() + position.x() + width > clippingRect().left() &&
          origin().x() + position.x() <= clippingRect().right()) {
        KDFont::Font(style.font)
            ->setGlyphGrayscalesForCodePoint(codePoint, &glyphBuffer);
        codePoint = decoder.nextCodePoint();
        while (codePoint.isCombining()) {
#if KANDINSKY_FONT_VARIABLE_WIDTH
          if (KDFont::GlyphWidth(style.font, codePoint) != width) {
            /* Hack: i is finer than an accent in Scandium, remove the dot
             * before stamping the acute accent. */
            glyphBuffer.grayscaleBuffer()[2] = 0;
          }
#else
          assert(KDFont::GlyphWidth(style.font, codePoint) == width);
#endif
          KDFont::Font(style.font)
              ->accumulateGlyphGrayscalesForCodePoint(codePoint, &glyphBuffer);
          codePointPointer = decoder.stringPosition();
          codePoint = decoder.nextCodePoint();
        }
        KDFont::Font(style.font)->colorizeGlyphBuffer(&palette, &glyphBuffer);
#if KANDINSKY_FONT_VARIABLE_WIDTH
        /* Glyph data are all defined with a GlyphMaxWidth stride. To
         * crop the background to the actual glyph width without using
         * variable strides, we crop the context. */
        KDRect savedClippingRect = clippingRect();
        setClippingRect(
            KDRect(savedClippingRect.origin(),
                   KDSize(std::min<KDCoordinate>(
                              savedClippingRect.width(),
                              position.x() - savedClippingRect.x() + width),
                          savedClippingRect.height())));
#endif
        /* Push the character on the screen
         * It's OK to trash the content of the color buffer since we'll re-fetch
         * it for the next char anyway */
        fillRectWithPixels(
            KDRect(position,
                   KDSize(KDFont::GlyphMaxWidth(style.font), glyphHeight)),
            glyphBuffer.colorBuffer(), glyphBuffer.colorBuffer());
#if KANDINSKY_FONT_VARIABLE_WIDTH
        setClippingRect(savedClippingRect);
#endif
      } else {
        codePoint = decoder.nextCodePoint();
        while (codePoint.isCombining()) {
          codePointPointer = decoder.stringPosition();
          codePoint = decoder.nextCodePoint();
        }
      }
      position = position.translatedBy(KDPoint(width, 0));
      if (origin().x() + position.x() > clippingRect().right()) {
        // fast forward until line feed
        while (codePoint != UCodePointLineFeed && codePoint != UCodePointNull) {
          codePoint = decoder.nextCodePoint();
        }
      }
    }
  }
  return position;
}

KDPoint KDContext::drawStringTransparent(const char* text, KDPoint p,
                                         KDGlyph::Style style,
                                         int maxByteLength) {
  KDPoint position = p;
  KDCoordinate glyphHeight = KDFont::GlyphHeight(style.font);
  KDFont::GlyphBuffer glyphBuffer;

  UTF8Decoder decoder(text);
  const char* codePointPointer = decoder.stringPosition();
  CodePoint codePoint = decoder.nextCodePoint();
  while (codePoint != UCodePointNull &&
         (maxByteLength < 0 || codePointPointer < text + maxByteLength)) {
    codePointPointer = decoder.stringPosition();
    if (codePoint == UCodePointLineFeed) {
      assert(position.y() < KDCOORDINATE_MAX - glyphHeight);
      position = KDPoint(origin().x(), position.y() + glyphHeight);
      if (origin().y() + position.y() > clippingRect().bottom()) {
        break;
      }
      codePoint = decoder.nextCodePoint();
    } else if (codePoint == UCodePointCarriageReturn) {
      codePoint = decoder.nextCodePoint();
    } else if (codePoint == UCodePointTabulation) {
      position = position.translatedBy(
          KDPoint(k_tabCharacterWidth * KDFont::GlyphMaxWidth(style.font), 0));
      codePoint = decoder.nextCodePoint();
    } else if (codePoint.isCombining()) {
      codePoint = decoder.nextCodePoint();
    } else {
      assert(!codePoint.isCombining());
      KDCoordinate width = KDFont::GlyphWidth(style.font, codePoint);
      if (origin().x() + position.x() + width > clippingRect().left() &&
          origin().x() + position.x() <= clippingRect().right()) {
        KDFont::Font(style.font)
            ->setGlyphGrayscalesForCodePoint(codePoint, &glyphBuffer);
        codePoint = decoder.nextCodePoint();
        while (codePoint.isCombining()) {
#if KANDINSKY_FONT_VARIABLE_WIDTH
          if (KDFont::GlyphWidth(style.font, codePoint) != width) {
            glyphBuffer.grayscaleBuffer()[2] = 0;
          }
#else
          assert(KDFont::GlyphWidth(style.font, codePoint) == width);
#endif
          KDFont::Font(style.font)
              ->accumulateGlyphGrayscalesForCodePoint(codePoint, &glyphBuffer);
          codePointPointer = decoder.stringPosition();
          codePoint = decoder.nextCodePoint();
        }

        /* Build alpha mask from grayscale buffer and blend glyph color
         * with existing pixels. */
        int glyphWidth = KDFont::GlyphMaxWidth(style.font);
        int totalPixels = glyphWidth * glyphHeight;
        static uint8_t maskBuffer[KDFont::k_maxGlyphPixelCount];
        static KDColor workBuffer[KDFont::k_maxGlyphPixelCount];
        uint8_t* grayscale = glyphBuffer.grayscaleBuffer();
        uint8_t grayscaleMask = (0xFF >> (8 - k_grayscaleBitsPerPixel));

        int pixelIndex = totalPixels - 1;
        int grayscaleByteIndex = pixelIndex * k_grayscaleBitsPerPixel / 8;
        while (pixelIndex >= 0) {
          uint8_t grayscaleByte = grayscale[grayscaleByteIndex--];
          for (int j = 0; j < 8 / k_grayscaleBitsPerPixel && pixelIndex >= 0; j++) {
            uint8_t g = grayscaleByte & grayscaleMask;
            grayscaleByte >>= k_grayscaleBitsPerPixel;
            /* blendRectWithMask expects mask to indicate how much the
             * existing pixel should be preserved. We want the opposite
             * (how much the glyph color should be applied), so invert. */
            uint8_t alpha = (g * 255 + grayscaleMask / 2) / grayscaleMask;
            maskBuffer[pixelIndex--] = UINT8_MAX - alpha;
          }
        }

        blendRectWithMask(
            KDRect(position, KDSize(KDFont::GlyphMaxWidth(style.font), glyphHeight)),
            style.glyphColor, maskBuffer, workBuffer);
#if KANDINSKY_FONT_VARIABLE_WIDTH
        /* No clipping restore needed here */
#endif
      } else {
        codePoint = decoder.nextCodePoint();
        while (codePoint.isCombining()) {
          codePointPointer = decoder.stringPosition();
          codePoint = decoder.nextCodePoint();
        }
      }
      position = position.translatedBy(KDPoint(width, 0));
      if (origin().x() + position.x() > clippingRect().right()) {
        while (codePoint != UCodePointLineFeed && codePoint != UCodePointNull) {
          codePoint = decoder.nextCodePoint();
        }
      }
    }
  }
  return position;
}

KDPoint KDContext::alignAndDrawStringTransparent(const char* text, KDPoint p, KDSize frame,
                                                 KDGlyph::Format format, int maxLength,
                                                 KDCoordinate lineSpacing) {
  assert(format.horizontalAlignment >= 0.0f &&
         format.horizontalAlignment <= 1.0f &&
         format.verticalAlignment >= 0.0f && format.verticalAlignment <= 1.0f);
  KDSize textSize =
      KDFont::Font(format.style.font)->stringSize(text, maxLength, lineSpacing);
  assert(textSize.width() <= frame.width() && textSize.height() <= frame.height());
  KDPoint originPoint(p.x(),
                 p.y() + std::ceil(format.verticalAlignment *
                                   (frame.height() - textSize.height())));
  KDSize lineFrame = KDSize(frame.width(), KDFont::GlyphHeight(format.style.font));

  UTF8Decoder decoder(text);
  const char* startLine = text;
  CodePoint codePoint = decoder.nextCodePoint();
  const char* codePointPointer = decoder.stringPosition();
  while (codePoint != UCodePointNull &&
         (maxLength < 0 || codePointPointer < text + maxLength)) {
    if (codePoint == UCodePointLineFeed) {
      // Align and draw a single line
      KDSize textSizeLine = KDFont::Font(format.style.font)->stringSize(startLine, codePointPointer - startLine);
      KDPoint lineOrigin(originPoint.x() + std::round(format.horizontalAlignment * (frame.width() - textSizeLine.width())), originPoint.y());
      drawStringTransparent(startLine, lineOrigin, format.style, codePointPointer - startLine - 1);
      startLine = codePointPointer;
      originPoint = KDPoint(originPoint.x(), originPoint.y() + lineFrame.height() + lineSpacing);
    }
    codePoint = decoder.nextCodePoint();
    codePointPointer = decoder.stringPosition();
  }
  // Last line
  KDSize lastLineSize = KDFont::Font(format.style.font)->stringSize(startLine, maxLength + text - startLine, lineSpacing);
  KDPoint lastOrigin(originPoint.x() + std::round(format.horizontalAlignment * (frame.width() - lastLineSize.width())), originPoint.y());
  return drawStringTransparent(startLine, lastOrigin, format.style, maxLength + text - startLine);
}
