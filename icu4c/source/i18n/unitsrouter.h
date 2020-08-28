// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#ifndef __UNITSROUTER_H__
#define __UNITSROUTER_H__

#include <limits>

#include "cmemory.h"
#include "complexunitsconverter.h"
#include "measunit_impl.h"
#include "unicode/measunit.h"
#include "unicode/stringpiece.h"
#include "unicode/uobject.h"
#include "unitsdata.h"

U_NAMESPACE_BEGIN

// Forward declarations
class Measure;

namespace units {

struct RouteResult : UMemory {
    // A list of measures: a single measure for single units, multiple measures
    // for mixed units.
    //
    // TODO(icu-units/icu#21): figure out the right mixed unit API.
    MaybeStackVector<Measure> measures;

    // A skeleton string starting with a precision-increment.
    //
    // TODO(hugovdm): generalise? or narrow down to only a precision-increment?
    // or document that other skeleton elements are ignored?
    UnicodeString precision;

    // The output unit for this RouteResult. This may be a MIXED unit - for
    // example: "yard-and-foot-and-inch", for which `measures` will have three
    // elements.
    MeasureUnitImpl outputUnit;

    RouteResult(MaybeStackVector<Measure> measures, UnicodeString precision, MeasureUnitImpl outputUnit)
        : measures(std::move(measures)), precision(std::move(precision)), outputUnit(std::move(outputUnit)) {}
};

/**
 * Contains the complex unit converter and the limit which representing the smallest value that the
 * converter should accept. For example, if the converter is converting to `foot+inch` and the limit
 * equals 3.0, thus means the converter should not convert to a value less than `3.0 feet`.
 *
 * NOTE:
 *    if the limit doest not has a value `i.e. (std::numeric_limits<double>::lowest())`, this mean there
 *    is no limit for the converter.
 */
struct ConverterPreference : UMemory {
    ComplexUnitsConverter converter;
    double limit;
    UnicodeString precision;

    // The output unit for this ConverterPreference. This may be a MIXED unit -
    // for example: "yard-and-foot-and-inch".
    MeasureUnitImpl targetUnit;

    // In case there is no limit, the limit will be -inf.
    ConverterPreference(const MeasureUnitImpl &source, const MeasureUnitImpl &complexTarget,
                        UnicodeString precision, const ConversionRates &ratesInfo, UErrorCode &status)
        : ConverterPreference(source, complexTarget, std::numeric_limits<double>::lowest(), precision,
                              ratesInfo, status) {}

    ConverterPreference(const MeasureUnitImpl &source, const MeasureUnitImpl &complexTarget,
                        double limit, UnicodeString precision, const ConversionRates &ratesInfo,
                        UErrorCode &status)
        : converter(source, complexTarget, ratesInfo, status), limit(limit),
          precision(std::move(precision)), targetUnit(complexTarget.copy(status)) {}
};

/**
 * `UnitsRouter` responsible for converting from a single unit (such as `meter` or `meter-per-second`) to
 * one of the complex units based on the limits.
 * For example:
 *    if the input is `meter` and the output as following
 *    {`foot+inch`, limit: 3.0}
 *    {`inch`     , limit: no value (-inf)}
 *    Thus means if the input in `meter` is greater than or equal to `3.0 feet`, the output will be in
 *    `foot+inch`, otherwise, the output will be in `inch`.
 *
 * NOTE:
 *    the output units and the their limits MUST BE in order, for example, if the output units, from the
 *    previous example, are the following:
 *        {`inch`     , limit: no value (-inf)}
 *        {`foot+inch`, limit: 3.0}
 *     IN THIS CASE THE OUTPUT WILL BE ALWAYS IN `inch`.
 *
 * NOTE:
 *    the output units  and their limits will be extracted from the units preferences database by knowing
 *    the followings:
 *        - input unit
 *        - locale
 *        - usage
 *
 * DESIGN:
 *    `UnitRouter` uses internally `ComplexUnitConverter` in order to convert the input units to the
 *    desired complex units and to check the limit too.
 */
class U_I18N_API UnitsRouter {
  public:
    UnitsRouter(MeasureUnit inputUnit, StringPiece locale, StringPiece usage, UErrorCode &status);

    RouteResult route(double quantity, UErrorCode &status) const;

    /**
     * Returns the list of possible output units, i.e. the full set of
     * preferences, for the localized, usage-specific unit preferences.
     *
     * The returned pointer should be valid for the lifetime of the
     * UnitsRouter instance.
     */
    const MaybeStackVector<MeasureUnit> *getOutputUnits() const;

  private:
    // List of possible output units. TODO: converterPreferences_ now also has
    // this data available. Maybe drop outputUnits_ and have getOutputUnits
    // construct a the list from data in converterPreferences_ instead?
    MaybeStackVector<MeasureUnit> outputUnits_;

    MaybeStackVector<ConverterPreference> converterPreferences_;
};

} // namespace units
U_NAMESPACE_END

#endif //__UNITSROUTER_H__

#endif /* #if !UCONFIG_NO_FORMATTING */
