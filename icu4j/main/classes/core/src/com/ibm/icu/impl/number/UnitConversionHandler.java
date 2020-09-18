// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
package com.ibm.icu.impl.number;

import com.ibm.icu.impl.units.ComplexUnitsConverter;
import com.ibm.icu.impl.units.MeasureUnitImpl;
import com.ibm.icu.impl.units.UnitsData;
import com.ibm.icu.util.Measure;
import com.ibm.icu.util.MeasureUnit;

import java.util.ArrayList;
import java.util.List;

/**
 * A MicroPropsGenerator which converts a measurement from a simple MeasureUnit
 * to a Mixed MeasureUnit.
 */
public class UnitConversionHandler implements MicroPropsGenerator {

    private final MicroPropsGenerator fParent;
    private MeasureUnit fOutputUnit;
    private ComplexUnitsConverter fComplexUnitConverter;

    /**
     * Constructor.
     *
     * @param outputUnit Specifies both the input and output MeasureUnit: if it is a
     *     MIXED unit, the input MeasureUnit will be just the biggest unit of
     *     the sequence.
     * @param parent The parent MicroPropsGenerator.
     */
    public UnitConversionHandler(MeasureUnit outputUnit, MicroPropsGenerator parent) {
        // TODO: port C++ changes from commit c84ded050a (PR #1322) to Java
        this.fOutputUnit = outputUnit;
        this.fParent = parent;

        List<MeasureUnit> singleUnits = outputUnit.splitToSingleUnits();

        assert outputUnit.getComplexity() == MeasureUnit.Complexity.MIXED;
        assert singleUnits.size() > 1;

        MeasureUnitImpl outputUnitImpl = MeasureUnitImpl.forIdentifier(outputUnit.getIdentifier());
        // TODO(icu-units#97): The input unit should be the largest unit, not the first unit, in the identifier.
        this.fComplexUnitConverter =
                new ComplexUnitsConverter(
                        new MeasureUnitImpl(outputUnitImpl.getSingleUnits().get(0)),
                        outputUnitImpl,
                        new UnitsData().getConversionRates());
    }

    /**
     * Obtains the appropriate output values from the Unit Converter.
     */
    @Override
    public MicroProps processQuantity(DecimalQuantity quantity) {
        MicroProps result = this.fParent.processQuantity(quantity);

        quantity.roundToInfinity(); // Enables toDouble
        List<Measure> measures = this.fComplexUnitConverter.convert(quantity.toBigDecimal());

        result.outputUnit = this.fOutputUnit;
        UsagePrefsHandler.mixedMeasuresToMicros(measures, quantity, result);

        return result;
    }
}
