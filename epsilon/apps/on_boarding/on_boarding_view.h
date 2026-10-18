#pragma once

#include <assert.h>
#include <escher/view.h>

#include "splash_view.h"
#include "tutorial_view.h"

namespace OnBoarding {

/* Root view of the on boarding app: the splash screen first, then the
 * tutorial. Only one of them is given a frame at a time. */
class OnBoardingView : public Escher::View {
 public:
  SplashView* splashView() { return &m_splashView; }
  TutorialView* tutorialView() { return &m_tutorialView; }
  bool isShowingTutorial() const { return m_showTutorial; }
  void showTutorial() {
    m_showTutorial = true;
    layoutSubviews();
    markWholeFrameAsDirty();
  }

 private:
  int numberOfSubviews() const override { return 2; }
  Escher::View* subviewAtIndex(int index) override {
    assert(index == 0 || index == 1);
    return index == 0 ? static_cast<Escher::View*>(&m_splashView)
                      : static_cast<Escher::View*>(&m_tutorialView);
  }
  void layoutSubviews(bool force = false) override {
    setChildFrame(&m_splashView, m_showTutorial ? KDRectZero : bounds(), force);
    setChildFrame(&m_tutorialView, m_showTutorial ? bounds() : KDRectZero,
                  force);
  }

  SplashView m_splashView;
  TutorialView m_tutorialView;
  bool m_showTutorial = false;
};

}  // namespace OnBoarding
