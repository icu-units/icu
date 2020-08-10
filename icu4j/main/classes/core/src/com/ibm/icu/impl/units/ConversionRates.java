package com.ibm.icu.impl.units;

import com.ibm.icu.impl.Assert;
import com.ibm.icu.impl.UResource;
import com.ibm.icu.math.BigDecimal;
import com.sun.xml.internal.bind.v2.runtime.unmarshaller.XsiNilLoader;

import java.util.ArrayList;
import java.util.TreeMap;

public class ConversionRates {

    public ConversionRates() {
        mapToBasicUnits = new TreeMap<>();

        /* TODO: Read from the data */
        Assert.assrt(simpleUnits.length == targets.length && targets.length == factors.length && factors.length == offsets.length);
        for (int i = 0; i < simpleUnits.length; i++) {
            mapToBasicUnits.put(simpleUnits[i], new ConversionRate(simpleUnits[i], targets[i], factors[i], offsets[i]));
        }
    }

    public String getBasicUnit(String identifier) {
        /* TODO: implement */
        return identifier;
    }

    public ArrayList<SingleUnitImpl> getBasicUnits(MeasureUnitImpl measureUnitImpl) {
        ArrayList<SingleUnitImpl> result = new ArrayList<>();
        ArrayList<SingleUnitImpl> singleUnits = measureUnitImpl.getSingleUnits();
        for (SingleUnitImpl singleUnit :
                singleUnits) {
            result.addAll(getBasicUnits(singleUnit));
        }

        return result;
    }

    /**
     * @param singleUnit
     * @return The bese units in `SingleUnitImpl`.
     */
    public ArrayList<SingleUnitImpl> getBasicUnits(SingleUnitImpl singleUnit) {
        String target = mapToBasicUnits.get(singleUnit.getSimpleUnit()).getTarget();
        MeasureUnitImpl targetImpl = UnitsParser.parseForIdentifier(target);
        return targetImpl.getSingleUnits();
    }


    /**
     * Map from any simple unit (i.e. "meter", "foot", "inch") to its basic/root conversion rate info.
     */
    private TreeMap<String, ConversionRate> mapToBasicUnits;


    // TODO: read those units from the units.txt
    public static String[] simpleUnits = {
            "100-kilometer",
            "acre",
            "ampere",
            "arc-minute",
            "arc-second",
            "astronomical-unit",
            "atmosphere",
            "bar",
            "barrel",
            "bit",
            "british-thermal-unit",
            "bushel",
            "byte",
            "calorie",
            "candela",
            "carat",
            "celsius",
            "century",
            "cup",
            "cup-metric",
            "dalton",
            "day",
            "day-person",
            "decade",
            "degree",
            "dot",
            "dunam",
            "earth-mass",
            "earth-radius",
            "electronvolt",
            "em",
            "fahrenheit",
            "fathom",
            "fluid-ounce",
            "fluid-ounce-imperial",
            "foodcalorie",
            "foot",
            "furlong",
            "g-force",
            "gallon",
            "gallon-imperial",
            "gram",
            "hectare",
            "hertz",
            "horsepower",
            "hour",
            "inch",
            "item",
            "joule",
            "karat",
            "kelvin",
            "kilogram",
            "knot",
            "light-year",
            "liter",
            "lux",
            "meter",
            "metric-ton",
            "mile",
            "mile-scandinavian",
            "minute",
            "mole",
            "month",
            "month-person",
            "nautical-mile",
            "newton",
            "ofhg",
            "ohm",
            "ounce",
            "ounce-troy",
            "parsec",
            "pascal",
            "percent",
            "permille",
            "permillion",
            "permyriad",
            "pint",
            "pint-metric",
            "pixel",
            "point",
            "portion",
            "pound",
            "pound-force",
            "quart",
            "radian",
            "revolution",
            "second",
            "solar-luminosity",
            "solar-mass",
            "solar-radius",
            "stone",
            "tablespoon",
            "teaspoon",
            "therm-us",
            "ton",
            "volt",
            "watt",
            "week",
            "week-person",
            "yard",
            "year",
            "year-person",
    };

    // TODO: read those units from the units.txt
    public static String[] factors = {
            "100000",
            "ft2_to_m2 * 43560",
            "1",
            "1/360*60",
            "1/360*60*60",
            "149597900000",
            "101325",
            "100000",
            "42*gal_to_m3",
            "1",
            "4.184*2267.96185/9",
            "2150.42*in3_to_m3",
            "8",
            "4.184",
            "1",
            "0.0002",
            "1",
            "100",
            "gal_to_m3/16",
            "0.00025",
            "1.49241808560E-10",
            "86400",
            "86400",
            "10",
            "1/360",
            "1",
            "1000",
            "5.9722E+24",
            "6.3781E6",
            "1.602177E-19",
            "1",
            "5/9",
            "ft_to_m * 6",
            "gal_to_m3/128",
            "gal_imp_to_m3/160",
            "4184",
            "ft_to_m",
            "ft_to_m*660",
            "gravity",
            "gal_to_m3",
            "gal_imp_to_m3",
            "0.001",
            "10000",
            "1",
            "ft_to_m * lb_to_kg * gravity * 550",
            "3600",
            "ft_to_m/12",
            "1",
            "1",
            "1/24",
            "1",
            "1",
            "1852/3600",
            "9460730000000000",
            "0.001",
            "1",
            "1",
            "1000",
            "ft_to_m*5280",
            "10000",
            "60",
            "6.02214076E+23",
            "1/12",
            "1/12",
            "1852",
            "1",
            "13595.1*gravity",
            "1",
            "lb_to_kg/16",
            "0.03110348",
            "30856780000000000",
            "1",
            "1/100",
            "1/1000",
            "1/1000000",
            "1/10000",
            "gal_to_m3/8",
            "0.0005",
            "1",
            "ft_to_m/864",
            "1",
            "lb_to_kg",
            "lb_to_kg * gravity",
            "gal_to_m3/4",
            "1/2*PI",
            "1",
            "1",
            "3.828E+26",
            "1.98847E+30",
            "695700000",
            "lb_to_kg*14",
            "gal_to_m3/256",
            "gal_to_m3/16*48",
            "105480400",
            "lb_to_kg*2000",
            "1",
            "1",
            "604800",
            "604800",
            "ft_to_m*3",
            "1",
            "1",
    };

    // TODO: read those units from the units.txt
    public static String[] targets = {
            "meter",
            "square-meter",
            "ampere",
            "revolution",
            "revolution",
            "meter",
            "kilogram-per-meter-square-second",
            "kilogram-per-meter-square-second",
            "cubic-meter",
            "bit",
            "kilogram-square-meter-per-square-second",
            "cubic-meter",
            "bit",
            "kilogram-square-meter-per-square-second",
            "candela",
            "kilogram",
            "kelvin",
            "year",
            "cubic-meter",
            "cubic-meter",
            "kilogram-square-meter-per-square-second",
            "second",
            "second",
            "year",
            "revolution",
            "pixel",
            "square-meter",
            "kilogram",
            "meter",
            "kilogram-square-meter-per-square-second",
            "em",
            "kelvin",
            "meter",
            "cubic-meter",
            "cubic-meter",
            "kilogram-square-meter-per-square-second",
            "meter",
            "meter",
            "meter-per-square-second",
            "cubic-meter",
            "cubic-meter",
            "kilogram",
            "square-meter",
            "revolution-per-second",
            "kilogram-square-meter-per-cubic-second",
            "second",
            "meter",
            "item",
            "kilogram-square-meter-per-square-second",
            "portion",
            "kelvin",
            "kilogram",
            "meter-per-second",
            "meter",
            "cubic-meter",
            "candela-square-meter-per-square-meter",
            "meter",
            "kilogram",
            "meter",
            "meter",
            "second",
            "item",
            "year",
            "year",
            "meter",
            "kilogram-meter-per-square-second",
            "kilogram-per-square-meter-square-second",
            "kilogram-square-meter-per-cubic-second-square-ampere",
            "kilogram",
            "kilogram",
            "meter",
            "kilogram-per-meter-square-second",
            "portion",
            "portion",
            "portion",
            "portion",
            "cubic-meter",
            "cubic-meter",
            "pixel",
            "meter",
            "portion",
            "kilogram",
            "kilogram-meter-per-square-second",
            "cubic-meter",
            "revolution",
            "revolution",
            "second",
            "kilogram-square-meter-per-cubic-second",
            "kilogram",
            "meter",
            "kilogram",
            "cubic-meter",
            "cubic-meter",
            "kilogram-square-meter-per-square-second",
            "kilogram",
            "kilogram-square-meter-per-cubic-second-ampere",
            "kilogram-square-meter-per-cubic-second",
            "second",
            "second",
            "meter",
            "year",
            "year",
    };

    // TODO: read those units from the units.txt
    public static final String[] offsets = {
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "273.15",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "2298.35/9",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
            "1",
    };

}
