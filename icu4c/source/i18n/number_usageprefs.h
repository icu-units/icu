// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#if !UCONFIG_NO_FORMATTING
#ifndef __NUMBER_USAGEPREFS_H__
#define __NUMBER_USAGEPREFS_H__

#include "cmemory.h"
#include "number_types.h"
#include "unitsrouter_stub.h"

U_NAMESPACE_BEGIN namespace number {
namespace impl {

/**
 * A MicroPropsGenerator which uses UnitsRouter to produce output converted to a
 * MeasureUnit appropriate for a particular localized usage: see
 * NumberFormatterSettings::usage().
 */
class U_I18N_API UsagePrefsHandler : public MicroPropsGenerator, public UMemory {
  public:
    UsagePrefsHandler(const Locale &locale, const MeasureUnit inputUnit, const StringPiece usage,
                      const MicroPropsGenerator *parent, UErrorCode &status);

    /**
     * Obtains the appropriate output value, MeasurementUnit and
     * rounding/precision behaviour from the UnitsRouter.
     */
    void processQuantity(DecimalQuantity &quantity, MicroProps &micros,
                         UErrorCode &status) const U_OVERRIDE;

    /**
     * Returns the list of possible output units, i.e. the full set of
     * preferences, for the localized, usage-specific unit preferences.
     */
    MaybeStackVector<MeasureUnit> getOutputUnits() const {
        return fUnitsRouter.getOutputUnits();
    }

  private:
    // TODO(units): use UnitsRouter, throw away StubUnitsRouter.
    StubUnitsRouter fUnitsRouter;
    const MicroPropsGenerator *fParent;
};

} // namespace impl
} // namespace number
U_NAMESPACE_END

#endif // __NUMBER_USAGEPREFS_H__
#endif /* #if !UCONFIG_NO_FORMATTING */