#include "calculation.h"

#include <poincare/cas.h>
#include <poincare/exception_checkpoint.h>
#include <poincare/helpers/expression_equal_sign.h>
#include <poincare/k_layout.h>
#include <poincare/src/expression/dependency.h>
#include <poincare/src/expression/projection.h>
#include <shared/global_store.h>
#include <string.h>

#include "app.h"
#include "global_preferences.h"

using namespace Poincare;
using namespace Shared;

namespace Calculation {

/* With CAS, some outputs have dependencies. For instance a^2*a^5/a^5 gives
 * dep(a^2,{nonNull(a)}). When all the dependencies can be written as a readable
 * condition, the output can be displayed without them (a^2) along with their
 * conditions (a≠0):
 *   nonNull(a) or a^0  ->  a≠0
 *   realPos(a)         ->  a≥0
 *   real(a)            ->  a∈ℝ
 *   realInteger(a)     ->  a∈ℤ
 * Any other dependency (f(a), 0×f(a)...) cannot be written as a condition: the
 * output is then displayed with its dependencies, as dep(...). */

enum class ConditionType : uint8_t {
  NonZero,      // a≠0
  NonNegative,  // a≥0
  Positive,     // a>0 (a≠0 and a≥0 merged)
  Real,         // a∈ℝ
  Integer       // a∈ℤ
};

struct Condition {
  ConditionType type;
  // Points inside the tree of the expression the conditions come from
  const Internal::Tree* operand;
};

/* The conditions are stored on the stack. An output that has more
 * dependencies is displayed with its dependencies. */
constexpr static int k_maxNumberOfConditions = 16;

static int IndexOfCondition(const Condition* conditions, int numberOfConditions,
                            ConditionType type, const Internal::Tree* operand) {
  for (int i = 0; i < numberOfConditions; i++) {
    if (conditions[i].type == type &&
        conditions[i].operand->treeIsIdenticalTo(operand)) {
      return i;
    }
  }
  return -1;
}

static void RemoveCondition(Condition* conditions, int* numberOfConditions,
                            int index) {
  for (int i = index; i < *numberOfConditions - 1; i++) {
    conditions[i] = conditions[i + 1];
  }
  (*numberOfConditions)--;
}

/* Fill [conditions] (of size k_maxNumberOfConditions) with the conditions
 * equivalent to the dependencies of [e] and return their number. Return 0 if
 * [e] has no dependencies or if one of them cannot be written as a condition.
 * The conditions are cleaned up: no duplicates, a≠0 and a≥0 are merged into
 * a>0, and a∈ℝ is dropped when a≥0 or a>0 already implies it. */
static int ConditionsFromDependencies(const UserExpression& e,
                                      Condition* conditions) {
  if (e.isUninitialized()) {
    return 0;
  }
  const Internal::Tree* tree = e.tree();
  if (!tree->isDep()) {
    return 0;
  }
  const Internal::Tree* dependencies = Internal::Dependency::Dependencies(tree);
  int numberOfConditions = 0;
  for (const Internal::Tree* dependency : dependencies->children()) {
    ConditionType type;
    if (dependency->isNonNull() ||
        (dependency->isPow() && dependency->child(1)->isZero())) {
      // a^0 is undefined only if a is null
      type = ConditionType::NonZero;
    } else if (dependency->isRealPos()) {
      type = ConditionType::NonNegative;
    } else if (dependency->isReal()) {
      type = ConditionType::Real;
    } else if (dependency->isRealInteger()) {
      type = ConditionType::Integer;
    } else {
      return 0;
    }
    const Internal::Tree* operand = dependency->child(0);
    if (IndexOfCondition(conditions, numberOfConditions, type, operand) >= 0) {
      continue;
    }
    if (numberOfConditions == k_maxNumberOfConditions) {
      return 0;
    }
    conditions[numberOfConditions++] = {type, operand};
  }
  // a≠0 and a≥0 -> a>0
  for (int i = 0; i < numberOfConditions; i++) {
    if (conditions[i].type != ConditionType::NonNegative) {
      continue;
    }
    int nonZeroIndex =
        IndexOfCondition(conditions, numberOfConditions, ConditionType::NonZero,
                         conditions[i].operand);
    if (nonZeroIndex >= 0) {
      conditions[i].type = ConditionType::Positive;
      RemoveCondition(conditions, &numberOfConditions, nonZeroIndex);
      if (nonZeroIndex < i) {
        i--;
      }
    }
  }
  // a∈ℝ is implied by a≥0 and a>0
  for (int i = 0; i < numberOfConditions; i++) {
    if (conditions[i].type != ConditionType::Real) {
      continue;
    }
    if (IndexOfCondition(conditions, numberOfConditions,
                         ConditionType::NonNegative,
                         conditions[i].operand) >= 0 ||
        IndexOfCondition(conditions, numberOfConditions,
                         ConditionType::Positive, conditions[i].operand) >= 0) {
      RemoveCondition(conditions, &numberOfConditions, i);
      i--;
    }
  }
  return numberOfConditions;
}

static bool HasConditions(const UserExpression& e) {
  Condition conditions[k_maxNumberOfConditions];
  return ConditionsFromDependencies(e, conditions) > 0;
}

// Return the main expression, without the dependencies that are conditions
static UserExpression WithoutConditions(const UserExpression& e) {
  return HasConditions(e) ? e.cloneChildAtIndex(0) : e;
}

static Layout LayoutOfExpression(UserExpression e) {
  return e.createLayout(Preferences::PrintFloatMode::Decimal,
                        PrintFloat::k_maxNumberOfSignificantDigits,
                        GlobalContextAccessor::Context(), OMG::Base::Decimal,
                        App::k_useTenPower, false, false,
                        PrintFloat::DecimalModeThreshold::Small);
}

static Layout LayoutOfCondition(const Condition& condition) {
  UserExpression e;
  switch (condition.type) {
    case ConditionType::NonZero:
      e = UserExpression::Create(KNotEqual(KA, 0_e), {.KA = condition.operand});
      break;
    case ConditionType::NonNegative:
      e = UserExpression::Create(KSuperiorEqual(KA, 0_e),
                                 {.KA = condition.operand});
      break;
    case ConditionType::Positive:
      e = UserExpression::Create(KSuperior(KA, 0_e), {.KA = condition.operand});
      break;
    case ConditionType::Real:
    case ConditionType::Integer: {
      /* There is no expression for "a belongs to a set": concatenate the
       * layouts, with the same separators as the comparisons. */
      Layout separator = KRackL(KOperatorSeparatorL);
      Layout layout = Layout::Concatenate(
          LayoutOfExpression(UserExpression::Builder(condition.operand)),
          separator);
      layout = Layout::Concatenate(layout, Layout::String("∈"));
      layout = Layout::Concatenate(layout, separator);
      return Layout::Concatenate(
          layout,
          Layout::String(condition.type == ConditionType::Real ? "ℝ" : "ℤ"));
    }
  }
  return LayoutOfExpression(e);
}

bool Calculation::operator==(const Calculation& c) {
  /* Some calculations can make appear trigonometric functions in their exact
   * output. Their argument will be different with the angle unit preferences
   * but both input and approximate output will be the same.
   * For example, i^(sqrt(3)) = cos(sqrt(3)*pi/2)+i*sin(sqrt(3)*pi/2) if angle
   * unit is radian and i^(sqrt(3)) = cos(sqrt(3)*90+i*sin(sqrt(3)*90) in
   * degree. */
  return cumulatedTreeSizes() == c.cumulatedTreeSizes() &&
         memcmp(m_trees, c.m_trees, cumulatedTreeSizes());
}

Calculation* Calculation::next() const {
  return reinterpret_cast<Calculation*>(
      const_cast<char*>(reinterpret_cast<const char*>(this) +
                        sizeof(Calculation) + cumulatedTreeSizes()));
}

UserExpression Calculation::input() {
  UserExpression e = UserExpression::Builder(inputTree());
  return e;
}

UserExpression Calculation::exactOutput() {
  UserExpression e = UserExpression::Builder(exactOutputTree());
  return e;
}

UserExpression Calculation::approximateOutput() {
  UserExpression e = UserExpression::Builder(approximatedOutputTree());
  return e;
}

Layout Calculation::createInputLayout() {
  ExceptionCheckpoint ecp;
  if (ExceptionRun(ecp)) {
    UserExpression e = input();
    if (!e.isUninitialized()) {
      return e.createLayout(Preferences::PrintFloatMode::Decimal,
                            PrintFloat::k_maxNumberOfSignificantDigits,
                            GlobalContextAccessor::Context(),
                            OMG::Base::Decimal, App::k_useTenPower, false,
                            true);
    }
  }
  return Layout();
}

Layout Calculation::createExactOutputLayout(bool* couldNotCreateExactLayout,
                                            bool withoutConditions) {
  ExceptionCheckpoint ecp;
  if (ExceptionRun(ecp)) {
    UserExpression e = exactOutput();
    if (!e.isUninitialized() &&
        (withoutConditions ||
         m_displayOutput == DisplayOutput::ConditionsToggle)) {
      e = WithoutConditions(e);
    }
    if (!e.isUninitialized()) {
      return e.createLayout(Preferences::PrintFloatMode::Decimal,
                            PrintFloat::k_maxNumberOfSignificantDigits,
                            GlobalContextAccessor::Context(),
                            OMG::Base::Decimal, App::k_useTenPower, false,
                            false, PrintFloat::DecimalModeThreshold::Small);
    }
  }
  *couldNotCreateExactLayout = true;
  return Layout();
}

Layout Calculation::createApproximateOutputLayout(
    bool* couldNotCreateApproximateLayout, bool forEditing,
    bool withoutConditions) {
  ExceptionCheckpoint ecp;
  if (ExceptionRun(ecp)) {
    UserExpression e = approximateOutput();
    if (!e.isUninitialized() &&
        (withoutConditions ||
         m_displayOutput == DisplayOutput::ConditionsToggle)) {
      e = WithoutConditions(e);
    }
    if (!e.isUninitialized()) {
      return e.createLayout(
          m_calculationPreferences.displayMode,
          forEditing ? PrintFloat::k_maxNumberOfSignificantDigits
                     : m_calculationPreferences.numberOfSignificantDigits,
          GlobalContextAccessor::Context(), OMG::Base::Decimal,
          App::k_useTenPower, false, false,
          PrintFloat::DecimalModeThreshold::Small);
    }
  }
  *couldNotCreateApproximateLayout = true;
  return Layout();
}

Layout Calculation::createConditionsLayout() {
  ExceptionCheckpoint ecp;
  if (ExceptionRun(ecp)) {
    UserExpression output = exactOutput();
    Condition conditions[k_maxNumberOfConditions];
    int numberOfConditions = ConditionsFromDependencies(output, conditions);
    if (numberOfConditions > 0) {
      // nonNull(a), realPos(b) -> a≠0 and b≥0
      Layout result = LayoutOfCondition(conditions[0]);
      for (int i = 1; i < numberOfConditions; i++) {
        result = Layout::Concatenate(result, Layout::String(" and "));
        result = Layout::Concatenate(result, LayoutOfCondition(conditions[i]));
      }
      return result;
    }
  }
  return Layout();
}

bool Calculation::shouldDisplayConditions() {
  if (!HasConditions(exactOutput())) {
    return false;
  }
  bool couldNotCreateLayout = false;
  Layout exact = createExactOutputLayout(&couldNotCreateLayout, true);
  Layout approximate =
      createApproximateOutputLayout(&couldNotCreateLayout, false, true);
  return !couldNotCreateLayout && !exact.isUninitialized() &&
         !approximate.isUninitialized() && exact.isIdenticalTo(approximate);
}

KDCoordinate Calculation::height(bool expanded) {
  KDCoordinate h = expanded ? m_expandedHeight : m_height;
  assert(h >= 0);
  return h;
}

void Calculation::computeDisplayOutput() {
  if (m_displayOutput != DisplayOutput::Unknown) {
    return;
  }
  if (!m_reductionFailure && shouldDisplayConditions()) {
    m_displayOutput = DisplayOutput::ConditionsToggle;
    return;
  }
  m_displayOutput =
      ComputeDisplayOutput(input(), exactOutput(), approximateOutput());
  /* Hide the exact output if it comes from a reduction failure and if the
   * approximate output is available */
  if (m_reductionFailure && CanDisplayApproximate(m_displayOutput)) {
    forceDisplayOutput(DisplayOutput::ApproximateOnly);
  }
  if (!CanDisplayExact(m_displayOutput) &&
      approximateOutput().isUndefinedOrNonReal()) {
    /* While we have access to the approximate expression, prevent displaying
     * "≈undef". */
    m_equalSign = EqualSign::Hidden;
  }
  assert(m_displayOutput != DisplayOutput::Unknown);
}

void Calculation::setHeights(KDCoordinate height, KDCoordinate expandedHeight) {
  m_height = height;
  m_expandedHeight = expandedHeight;
}

static bool ShouldOnlyDisplayExactOutput(const UserExpression& input) {
  /* If the input is a "store in a function", do not display the approximate
   * result. This prevents x->f(x) from displaying x = undef. */
  assert(!input.isUninitialized());
  return input.isStore() && input.cloneChildAtIndex(1).isUserFunction();
}

Calculation::OutputLayouts Calculation::layoutCalculation(
    KDFont::Size font, KDCoordinate maxVisibleWidth,
    bool canChangeDisplayOutput) {
  /* This processing has do be done in a certain order (1. display output, 2.
   * output layouts, 3. equal sign). */
  computeDisplayOutput();
  OutputLayouts outputLayouts =
      createOutputLayouts(canChangeDisplayOutput, maxVisibleWidth, font);
  computeEqualSign(outputLayouts);

  if (m_displayOutput == DisplayOutput::ExactHidden) {
    /* With ExactHidden, exact output layout was needed to compute equal sign.
     * Remove it afterward. */
    outputLayouts.exact = Layout();
  }
  assert(CanDisplayExact(m_displayOutput) ||
         outputLayouts.exact.isUninitialized());

  return outputLayouts;
}

Calculation::OutputLayouts Calculation::createOutputLayouts(
    bool canChangeDisplayOutput, KDCoordinate maxVisibleWidth,
    KDFont::Size font) {
  assert(m_displayOutput != DisplayOutput::Unknown);

  if (m_displayOutput == DisplayOutput::ConditionsToggle) {
    /* The conditions (a≠0) are displayed in the center layout and the output
     * without its dependencies (a^2) in the right layout. */
    bool couldNotCreateOutputLayout = false;
    Layout output = createApproximateOutputLayout(&couldNotCreateOutputLayout);
    Layout conditions = createConditionsLayout();
    if (couldNotCreateOutputLayout) {
      ExceptionCheckpoint::Raise();
    }
    if (conditions.isUninitialized()) {
      if (!canChangeDisplayOutput) {
        ExceptionCheckpoint::Raise();
      }
      forceDisplayOutput(DisplayOutput::ApproximateOnly);
      return {Layout(), output};
    }
    return {conditions, output};
  }

  // Create the exact output layout
  Layout exactOutput = Layout();
  // DisplayOutput::ExactHidden temporarily bypasses CanDisplayExact condition
  if (CanDisplayExact(m_displayOutput) ||
      m_displayOutput == DisplayOutput::ExactHidden) {
    bool couldNotCreateExactLayout = false;
    exactOutput = createExactOutputLayout(&couldNotCreateExactLayout);
    if (m_displayOutput == DisplayOutput::ExactHidden) {
      // exactOutput will only be used internally, no need to handle failure.
    } else if (couldNotCreateExactLayout) {
      if (canChangeDisplayOutput &&
          m_displayOutput != DisplayOutput::ExactOnly) {
        forceDisplayOutput(DisplayOutput::ApproximateOnly);
      } else {
        /* We should only display the exact result, but we cannot create it
         * -> raise an exception. */
        ExceptionCheckpoint::Raise();
      }
    } else if (canChangeDisplayOutput &&
               CanDisplayApproximate(m_displayOutput)) {
      KDCoordinate exactOutputWidth = exactOutput->layoutSize(font).width();
      assert(m_displayOutput == DisplayOutput::ExactAndApproximate ||
             m_displayOutput == DisplayOutput::ExactAndApproximateToggle);
      if (exactOutputWidth > k_maxExactLayoutWidth ||
          exactOutput.longestIntegerSize() > k_maxNumberDigitsInExactLayout) {
        forceDisplayOutput(DisplayOutput::ExactHidden);
      } else if (m_displayOutput == DisplayOutput::ExactAndApproximate &&
                 exactOutputWidth > maxVisibleWidth) {
        forceDisplayOutput(DisplayOutput::ExactAndApproximateToggle);
      }
    }
  }

  // Create the approximate output layout
  Layout approximateOutput = Layout();
  if (CanDisplayApproximate(m_displayOutput)) {
    bool couldNotCreateApproximateLayout = false;
    approximateOutput =
        createApproximateOutputLayout(&couldNotCreateApproximateLayout);
    if (couldNotCreateApproximateLayout) {
      if (canChangeDisplayOutput && CanDisplayExact(m_displayOutput)) {
        /* Set display the output to ApproximateOnly, make room in the pool by
         * erasing the exact layout, retry to create the approximate layout. */
        forceDisplayOutput(DisplayOutput::ApproximateOnly);
        exactOutput = Layout();
        couldNotCreateApproximateLayout = false;
        approximateOutput =
            createApproximateOutputLayout(&couldNotCreateApproximateLayout);
        if (couldNotCreateApproximateLayout) {
          ExceptionCheckpoint::Raise();
        }
      } else {
        ExceptionCheckpoint::Raise();
      }
    }
  }

  // Hide exact output layout if identical to approximate or input
  if (CanDisplayExact(m_displayOutput) &&
      CanDisplayApproximate(m_displayOutput)) {
    if (exactOutput.isIdenticalTo(approximateOutput)) {
      forceDisplayOutput(DisplayOutput::ApproximateIsIdenticalToExact);
    } else if ((m_displayOutput != DisplayOutput::ExactAndApproximateToggle) &&
               exactOutput.isIdenticalTo(createInputLayout())) {
      forceDisplayOutput(DisplayOutput::ExactAndApproximateToggle);
    }
  }

  return {exactOutput, approximateOutput};
}

Calculation::DisplayOutput Calculation::ComputeDisplayOutput(
    const UserExpression& input, const UserExpression& exactOutput,
    const UserExpression& approximateOutput) {
  using enum Calculation::Calculation::DisplayOutput;
  if (input.isUninitialized() || exactOutput.isUninitialized() ||
      ShouldOnlyDisplayExactOutput(input) ||
      exactOutput.isUndefinedOrNonReal()) {
    return ExactOnly;
  }
  if (approximateOutput.isUndefinedOrNonReal() ||
      // Other conditions are factorized in CAS
      CAS::ShouldOnlyDisplayApproximation(input, exactOutput, approximateOutput,
                                          GlobalContextAccessor::Context())) {
    return ExactHidden;
  }
  SimpleDeepSearchParameters matchParameters{
      .context = GlobalContextAccessor::Context()};
  if (input.isIdenticalTo(exactOutput) ||
      input.deepSearch(&Expression::isApproximate, matchParameters) ||
      exactOutput.deepSearch(&Expression::isApproximate, matchParameters) ||
      input.deepSearch(&Expression::isPercent, matchParameters)) {
    return ExactAndApproximateToggle;
  }
  return ExactAndApproximate;
}

Calculation::EqualSign Calculation::ComputeEqualSignFromOutputs(
    const OutputLayouts& outputLayouts, Poincare::ComplexFormat complexFormat,
    Poincare::AngleUnit angleUnit) {
  /* Displaying the right equal symbol is less important than displaying a
   * result, so we do not want computeEqualSign to create a pool failure that
   * would prevent from displaying a result that we managed to compute. We thus
   * encapsulate the method in an exception checkpoint: if there was not enough
   * memory on the pool to compute the equal sign, just return
   * EqualSign::Approximation. We can safely use an exception checkpoint here
   * because we are sure of not modifying any pre-existing node in the pool. We
   * are sure there cannot be a Store in the exactOutput. */
  ExceptionCheckpoint ecp;
  if (ExceptionRun(ecp)) {
    /* The output Layouts are converted back to Expressions so that they can be
     * compared */
    assert(!outputLayouts.exact.isUninitialized());
    assert(!outputLayouts.approximate.isUninitialized());
    UserExpression exactDisplayOutput =
        UserExpression::Parse(outputLayouts.exact.cloneWithoutMargins(),
                              GlobalContextAccessor::Context());
    UserExpression approximateDisplayOutput =
        UserExpression::Parse(outputLayouts.approximate.cloneWithoutMargins(),
                              GlobalContextAccessor::Context());
    // Parsing the layout into an expression is supposed to work here
    assert(!exactDisplayOutput.isUninitialized() &&
           !approximateDisplayOutput.isUninitialized());
    return Poincare::IsCalculationOutputStrictEquality(
               exactDisplayOutput, approximateDisplayOutput,
               SharedGlobalPrefs().numberOfSignificantDigits(),
               SimplificationParameters{
                   .user = {.complexFormat = complexFormat,
                            .angleUnit = angleUnit},
                   .allowedDimensions = AllowedDimensions::Any()})
               ? EqualSign::Equal
               : EqualSign::Approximation;
  } else {
    return EqualSign::Approximation;
  }
}

void Calculation::computeEqualSign(const OutputLayouts& outputLayouts) {
  assert(m_displayOutput != DisplayOutput::Unknown);
  if (m_equalSign != EqualSign::Unknown) {
    return;
  }
  if (m_displayOutput == DisplayOutput::ExactOnly) {
    m_equalSign = EqualSign::Hidden;
  } else if (m_displayOutput == DisplayOutput::ApproximateOnly) {
    m_equalSign = EqualSign::Approximation;
  } else if (m_displayOutput == DisplayOutput::ApproximateIsIdenticalToExact) {
    m_equalSign = EqualSign::Equal;
  } else if (m_displayOutput == DisplayOutput::ConditionsToggle) {
    /* The center layout is the conditions, not an exact output: there is
     * nothing to compare. The sign is replaced when displayed. */
    m_equalSign = EqualSign::Equal;
  } else if (outputLayouts.exact.isUninitialized()) {
    /* Exact layout may have failed to be computed in createOutputLayouts. */
    assert(m_displayOutput == DisplayOutput::ExactHidden);
    m_equalSign = EqualSign::Approximation;
  } else {
    m_equalSign = ComputeEqualSignFromOutputs(
        outputLayouts, m_calculationPreferences.complexFormat,
        m_calculationPreferences.angleUnit);
  }
  assert(m_equalSign != EqualSign::Unknown);
}

void Calculation::fillExpressionsForAdditionalResults(
    UserExpression* input, UserExpression* exactOutput,
    UserExpression* approximateOutput) {
  *input = this->input();
  *approximateOutput = this->approximateOutput();
  assert(m_displayOutput != DisplayOutput::Unknown);
  *exactOutput = CanDisplayExact(m_displayOutput) ? this->exactOutput()
                                                  : *approximateOutput;
}

AdditionalResultsType Calculation::additionalResultsType() {
  if (m_additionalResultsType.isUninitialized()) {
    if (m_reductionFailure) {
      // Hide the additional result if the calculation had a reduction failure
      m_additionalResultsType = AdditionalResultsType{.empty = true};
    } else {
      UserExpression i, a, e;
      fillExpressionsForAdditionalResults(&i, &e, &a);
      m_additionalResultsType =
          AdditionalResultsType::AdditionalResultsForExpressions(
              i, e, a, m_calculationPreferences,
              GlobalContextAccessor::Context());
    }
  }
  assert(!m_additionalResultsType.isUninitialized());
  return m_additionalResultsType;
}

}  // namespace Calculation
