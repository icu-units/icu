// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <math.h>

#include "cmemory.h"
#include "complexunitsconverter.h"
#include "uarrsort.h"
#include "uassert.h"
#include "unicode/fmtable.h"
#include "unicode/localpointer.h"
#include "unicode/measunit.h"
#include "unicode/measure.h"
#include "unitconverter.h"

U_NAMESPACE_BEGIN
namespace units {

ComplexUnitsConverter::ComplexUnitsConverter(const MeasureUnitImpl &inputUnit,
                                             const MeasureUnitImpl &outputUnits,
                                             const ConversionRates &ratesInfo, UErrorCode &status)
    : units_(outputUnits.extractIndividualUnits(status)) {
    if (U_FAILURE(status)) {
        return;
    }

    if (units_.length() == 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    auto compareUnits = [](const void *context, const void *left, const void *right) {
        UErrorCode status = U_ZERO_ERROR;

        typedef MeasureUnitImpl *MUIPointer;

        const auto *leftPointer = static_cast<const MUIPointer *>(left);
        const MeasureUnitImpl *leftU = *leftPointer;

        const auto *rightPointer = static_cast<const MUIPointer *>(right);
        const MeasureUnitImpl *rightU = *rightPointer;

        // TODO: use the compare function that will be added to UnitConveter.
        UnitConverter fromLeftToRight(*leftU,                                         //
                                      *rightU,                                        //
                                      *static_cast<const ConversionRates *>(context), //
                                      status);
        if (fromLeftToRight.convert(1.0) > 1.0) {
            return -1;
        };

        return 1;
    };

    uprv_sortArray(units_.getAlias(),    //
                   units_.length(),      //
                   units_.elementSize(), //
                   compareUnits,         //
                   &ratesInfo,           //
                   false,                //
                   &status               //
    );

    // In case the `outputUnits` are `UMEASURE_UNIT_MIXED` such as `foot+inch`. In this case we need more
    // converters to convert from the `inputUnit` to the first unit in the `outputUnits`. Then, a
    // converter from the first unit in the `outputUnits` to the second unit and so on.
    //      For Example:
    //          - inputUnit is `meter`
    //          - outputUnits is `foot+inch`
    //              - Therefore, we need to have two converters:
    //                      1. a converter from `meter` to `foot`
    //                      2. a converter from `foot` to `inch`
    //          - Therefore, if the input is `2 meter`:
    //              1. convert `meter` to `foot` --> 2 meter to 6.56168 feet
    //              2. convert the residual of 6.56168 feet (0.56168) to inches, which will be (6.74016
    //              inches)
    //              3. then, the final result will be (6 feet and 6.74016 inches)
    for (int i = 0, n = units_.length(); i < n; i++) {
        if (i == 0) { // first element
            unitConverters_.emplaceBackAndCheckErrorCode(status, inputUnit, *units_[i], ratesInfo,
                                                         status);
        } else {
            unitConverters_.emplaceBackAndCheckErrorCode(status, *units_[i - 1], *units_[i], ratesInfo,
                                                         status);
        }

        if (U_FAILURE(status)) {
            return;
        }
    }
}

UBool ComplexUnitsConverter::greaterThanOrEqual(double quantity, double limit) const {
    U_ASSERT(unitConverters_.length() > 0);

    // First converter converts to the biggest quantity.
    double newQuantity = unitConverters_[0]->convert(quantity);
    return newQuantity >= limit;
}

MaybeStackVector<Measure> ComplexUnitsConverter::convert(double quantity, UErrorCode &status) const {
    MaybeStackVector<Measure> result;

    for (int i = 0, n = unitConverters_.length(); i < n; ++i) {
        quantity = (*unitConverters_[i]).convert(quantity);
        if (i < n - 1) {
            int64_t newQuantity = floor(quantity);
            Formattable formattableNewQuantity(newQuantity);

            // NOTE: Measure would own its MeasureUnit.
            MeasureUnit *type = new MeasureUnit(units_[i]->copy(status).build(status));
            result.emplaceBackAndCheckErrorCode(status, formattableNewQuantity, type, status);

            // Keep the residual of the quantity.
            //   For example: `3.6 feet`, keep only `0.6 feet`
            quantity -= newQuantity;
        } else { // LAST ELEMENT
            Formattable formattableQuantity(quantity);

            // NOTE: Measure would own its MeasureUnit.
            MeasureUnit *type = new MeasureUnit(units_[i]->copy(status).build(status));
            result.emplaceBackAndCheckErrorCode(status, formattableQuantity, type, status);
        }
    }

    return result;
}

} // namespace units
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
