// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html


package com.ibm.icu.impl.number;

import com.ibm.icu.number.NumberFormatter;
import com.ibm.icu.text.PluralRules;
import com.ibm.icu.util.MeasureUnit;
import com.ibm.icu.util.NoUnit;
import com.ibm.icu.util.ULocale;

import java.util.ArrayList;
import java.util.List;

public class LongNameMultiplexer implements MicroPropsGenerator {
    private final MicroPropsGenerator fParent;

    /**
     * Because we only know which LongNameHandler we wish to call after calling
     * earlier MicroPropsGenerators in the chain, LongNameMultiplexer keeps the
     * parent link, while the LongNameHandlers are given no parents.
     */
    private List<LongNameHandler> fLongNameHandlers;
    private List<MixedUnitLongNameHandler> fMixedUnitHandlers;

    // Unowned pointers to instances owned by MaybeStackVectors.
    private List<MicroPropsGenerator> fHandlers;

    // Each MeasureUnit corresponds to the same-index MicroPropsGenerator
    // pointed to in fHandlers.
    private List<MeasureUnit> fMeasureUnits;

    public LongNameMultiplexer(MicroPropsGenerator fParent) {
        this.fParent = fParent;
    }

    // Produces a multiplexer for LongNameHandlers, one for each unit in
    // `units`. An individual unit might be a mixed unit.
    public static LongNameMultiplexer forMeasureUnits(ULocale locale,
                                                      List<MeasureUnit> units,
                                                      NumberFormatter.UnitWidth width,
                                                      PluralRules rules,
                                                      MicroPropsGenerator parent) {
        LongNameMultiplexer result = new LongNameMultiplexer(parent);

        assert (units.size() > 0);

        result.fMeasureUnits = new ArrayList<>();
        result.fHandlers = new ArrayList<>();
        result.fMixedUnitHandlers = new ArrayList<>();
        result.fLongNameHandlers = new ArrayList<>();

        for (int i = 0; i < units.size(); i++) {
            MeasureUnit unit = units.get(i);
            result.fMeasureUnits.add(unit);
            if (unit.getComplexity() == MeasureUnit.Complexity.MIXED) {
                MixedUnitLongNameHandler mlnh = MixedUnitLongNameHandler
                        .forMeasureUnit(locale, unit, width, rules, null);
                result.fMixedUnitHandlers.add(mlnh);
                result.fHandlers.add(mlnh);
            } else {
                LongNameHandler lnh = LongNameHandler
                        .forMeasureUnit(locale, unit, NoUnit.BASE, width, rules, null);
                result.fLongNameHandlers.add(lnh);
                result.fHandlers.add(lnh);
            }
        }

        return result;
    }

    // The output unit must be provided via `micros.outputUnit`, it must match
    // one of the units provided to the factory function.
    @Override
    public MicroProps processQuantity(DecimalQuantity quantity) {

        // We call parent->processQuantity() from the Multiplexer, instead of
        // letting LongNameHandler handle it: we don't know which LongNameHandler to
        // call until we've called the parent!
        MicroProps micros = this.fParent.processQuantity(quantity);

        // Call the correct LongNameHandler based on outputUnit
        for (int i = 0; i < this.fHandlers.size(); i++) {
            if (fMeasureUnits.get(i) == micros.outputUnit) {
                return fHandlers.get(i).processQuantity(quantity);
            }
        }

        throw new AssertionError
                (" We shouldn't receive any outputUnit for which we haven't already got a LongNameHandler");
    }
}
