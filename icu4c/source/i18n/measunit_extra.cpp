// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// Extra functions for MeasureUnit not needed for all clients.
// Separate .o file so that it can be removed for modularity.

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

// Allow implicit conversion from char16_t* to UnicodeString for this file:
// Helpful in toString methods and elsewhere.
#define UNISTR_FROM_STRING_EXPLICIT

#include "cstring.h"
#include "uassert.h"
#include "ucln_in.h"
#include "umutex.h"
#include "unicode/errorcode.h"
#include "unicode/localpointer.h"
#include "unicode/measunit.h"
#include "unicode/ucharstrie.h"
#include "unicode/ucharstriebuilder.h"

#include "cstr.h"

U_NAMESPACE_BEGIN


namespace {

// This is to ensure we only insert positive integers into the trie
constexpr int32_t kSIPrefixOffset = 64;

constexpr int32_t kCompoundPartOffset = 128;

enum CompoundPart {
    COMPOUND_PART_PER = kCompoundPartOffset,
    COMPOUND_PART_TIMES,
    COMPOUND_PART_PLUS,
};

constexpr int32_t kPowerPartOffset = 256;

enum PowerPart {
    POWER_PART_P2 = kPowerPartOffset + 2,
    POWER_PART_P3,
    POWER_PART_P4,
    POWER_PART_P5,
    POWER_PART_P6,
    POWER_PART_P7,
    POWER_PART_P8,
    POWER_PART_P9,
    POWER_PART_P10,
    POWER_PART_P11,
    POWER_PART_P12,
    POWER_PART_P13,
    POWER_PART_P14,
    POWER_PART_P15,
};

constexpr int32_t kSimpleUnitOffset = 512;

const struct SIPrefixStrings {
    const char* const string;
    UMeasureSIPrefix value;
} gSIPrefixStrings[] = {
    { "yotta", UMEASURE_SI_PREFIX_YOTTA },
    { "zetta", UMEASURE_SI_PREFIX_ZETTA },
    { "exa", UMEASURE_SI_PREFIX_EXA },
    { "peta", UMEASURE_SI_PREFIX_PETA },
    { "tera", UMEASURE_SI_PREFIX_TERA },
    { "giga", UMEASURE_SI_PREFIX_GIGA },
    { "mega", UMEASURE_SI_PREFIX_MEGA },
    { "kilo", UMEASURE_SI_PREFIX_KILO },
    { "hecto", UMEASURE_SI_PREFIX_HECTO },
    { "deka", UMEASURE_SI_PREFIX_DEKA },
    { "deci", UMEASURE_SI_PREFIX_DECI },
    { "centi", UMEASURE_SI_PREFIX_CENTI },
    { "milli", UMEASURE_SI_PREFIX_MILLI },
    { "micro", UMEASURE_SI_PREFIX_MICRO },
    { "nano", UMEASURE_SI_PREFIX_NANO },
    { "pico", UMEASURE_SI_PREFIX_PICO },
    { "femto", UMEASURE_SI_PREFIX_FEMTO },
    { "atto", UMEASURE_SI_PREFIX_ATTO },
    { "zepto", UMEASURE_SI_PREFIX_ZEPTO },
    { "yocto", UMEASURE_SI_PREFIX_YOCTO },
};

// TODO(ICU-20920): Get this list from data
const char16_t* const gSimpleUnits[] = {
    u"one", // note: expected to be index 0
    u"100kilometer",
    u"acre",
    u"ampere",
    u"arc-minute",
    u"arc-second",
    u"astronomical-unit",
    u"atmosphere",
    u"bar",
    u"barrel",
    u"bit",
    u"british-thermal-unit",
    u"bushel",
    u"byte",
    u"calorie",
    u"carat",
    u"celsius",
    u"century",
    u"cup",
    u"cup-metric",
    u"dalton",
    u"day",
    u"day-person",
    u"decade",
    u"degree",
    u"dot", // (as in "dot-per-inch")
    u"dunam",
    u"earth-mass",
    u"electronvolt",
    u"em",
    u"fahrenheit",
    u"fathom",
    u"fluid-ounce",
    u"fluid-ounce-imperial",
    u"foodcalorie",
    u"foot",
    u"furlong",
    u"g-force",
    u"gallon",
    u"gallon-imperial",
    u"generic", // (i.e., "temperature-generic")
    u"gram",
    u"hectare", // (note: other "are" derivatives are uncommon)
    u"hertz",
    u"horsepower",
    u"hour",
    u"inch",
    u"inch-hg",
    u"joule",
    u"karat",
    u"kelvin",
    u"knot",
    u"light-year",
    u"liter",
    u"lux",
    u"meter",
    u"meter-of-mercury", // (not "millimeter-of-mercury")
    u"metric-ton",
    u"mile",
    u"mile-scandinavian",
    u"minute",
    u"mole",
    u"month",
    u"month-person",
    u"nautical-mile",
    u"newton",
    u"ohm",
    u"ounce",
    u"ounce-troy",
    u"parsec",
    u"pascal",
    u"percent",
    u"permille",
    u"permillion",
    u"permyriad",
    u"pint",
    u"pint-metric",
    u"pixel",
    u"point",
    u"pound",
    u"pound-force",
    u"quart",
    u"radian",
    u"revolution",
    u"second",
    u"solar-luminosity",
    u"solar-mass",
    u"solar-radius",
    u"stone",
    u"tablespoon",
    u"teaspoon",
    u"therm-us",
    u"ton",
    u"volt",
    u"watt",
    u"week",
    u"week-person",
    u"yard",
    u"year",
    u"year-person",
};

icu::UInitOnce gUnitExtrasInitOnce = U_INITONCE_INITIALIZER;

char16_t* kSerializedUnitExtrasStemTrie = nullptr;

UBool U_CALLCONV cleanupUnitExtras() {
    uprv_free(kSerializedUnitExtrasStemTrie);
    kSerializedUnitExtrasStemTrie = nullptr;
    gUnitExtrasInitOnce.reset();
    return TRUE;
}

void U_CALLCONV initUnitExtras(UErrorCode& status) {
    ucln_i18n_registerCleanup(UCLN_I18N_UNIT_EXTRAS, cleanupUnitExtras);

    UCharsTrieBuilder b(status);
    if (U_FAILURE(status)) { return; }

    // Add SI prefixes
    for (const auto& siPrefixInfo : gSIPrefixStrings) {
        UnicodeString uSIPrefix(siPrefixInfo.string, -1, US_INV);
        b.add(uSIPrefix, siPrefixInfo.value + kSIPrefixOffset, status);
    }
    if (U_FAILURE(status)) { return; }

    // Add syntax parts (compound, power prefixes)
    b.add(u"-per-", COMPOUND_PART_PER, status);
    b.add(u"-", COMPOUND_PART_TIMES, status);
    b.add(u"+", COMPOUND_PART_PLUS, status);
    b.add(u"square-", POWER_PART_P2, status);
    b.add(u"cubic-", POWER_PART_P3, status);
    b.add(u"p2-", POWER_PART_P2, status);
    b.add(u"p3-", POWER_PART_P3, status);
    b.add(u"p4-", POWER_PART_P4, status);
    b.add(u"p5-", POWER_PART_P5, status);
    b.add(u"p6-", POWER_PART_P6, status);
    b.add(u"p7-", POWER_PART_P7, status);
    b.add(u"p8-", POWER_PART_P8, status);
    b.add(u"p9-", POWER_PART_P9, status);
    b.add(u"p10-", POWER_PART_P10, status);
    b.add(u"p11-", POWER_PART_P11, status);
    b.add(u"p12-", POWER_PART_P12, status);
    b.add(u"p13-", POWER_PART_P13, status);
    b.add(u"p14-", POWER_PART_P14, status);
    b.add(u"p15-", POWER_PART_P15, status);
    if (U_FAILURE(status)) { return; }

    // Add sanctioned simple units by offset
    int32_t simpleUnitOffset = kSimpleUnitOffset;
    for (auto simpleUnit : gSimpleUnits) {
        b.add(simpleUnit, simpleUnitOffset++, status);
    }

    // Build the CharsTrie
    // TODO: Use SLOW or FAST here?
    UnicodeString result;
    b.buildUnicodeString(USTRINGTRIE_BUILD_FAST, result, status);
    if (U_FAILURE(status)) { return; }

    // Copy the result into the global constant pointer
    size_t numBytes = result.length() * sizeof(char16_t);
    kSerializedUnitExtrasStemTrie = static_cast<char16_t*>(uprv_malloc(numBytes));
    uprv_memcpy(kSerializedUnitExtrasStemTrie, result.getBuffer(), numBytes);
}

class Token {
public:
    Token(int32_t match) : fMatch(match) {}

    enum Type {
        TYPE_UNDEFINED,
        TYPE_SI_PREFIX,
        TYPE_COMPOUND_PART,
        TYPE_POWER_PART,
        TYPE_ONE,
        TYPE_SIMPLE_UNIT,
    };

    Type getType() const {
        if (fMatch <= 0) {
            UPRV_UNREACHABLE;
        } else if (fMatch < kCompoundPartOffset) {
            return TYPE_SI_PREFIX;
        } else if (fMatch < kPowerPartOffset) {
            return TYPE_COMPOUND_PART;
        } else if (fMatch < kSimpleUnitOffset) {
            return TYPE_POWER_PART;
        } else if (fMatch == kSimpleUnitOffset) {
            return TYPE_ONE;
        } else {
            return TYPE_SIMPLE_UNIT;
        }
    }

    UMeasureSIPrefix getSIPrefix() const {
        U_ASSERT(getType() == TYPE_SI_PREFIX);
        return static_cast<UMeasureSIPrefix>(fMatch - kSIPrefixOffset);
    }

    int32_t getMatch() const {
        U_ASSERT(getType() == TYPE_COMPOUND_PART);
        return fMatch;
    }

    int8_t getPower() const {
        U_ASSERT(getType() == TYPE_POWER_PART);
        return static_cast<int8_t>(fMatch - kPowerPartOffset);
    }

    int32_t getSimpleUnitIndex() const {
        U_ASSERT(getType() == TYPE_SIMPLE_UNIT);
        return fMatch - kSimpleUnitOffset;
    }

private:
    int32_t fMatch;
};

struct SingleUnit {
    int8_t power = 1;
    UMeasureSIPrefix siPrefix = UMEASURE_SI_PREFIX_ONE;
    int32_t simpleUnitIndex = 0;
    StringPiece id = "one";

    void appendTo(CharString& builder, UErrorCode& status) const {
        if (simpleUnitIndex == 0) {
            // Don't propagate SI prefixes and powers on one
            builder.append("one", status);
            return;
        }
        int8_t posPower = power < 0 ? -power : power;
        if (posPower == 0) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
        } else if (posPower == 1) {
            // no-op
        } else if (posPower == 2) {
            builder.append("square-", status);
        } else if (posPower == 3) {
            builder.append("cubic-", status);
        } else if (posPower < 10) {
            builder.append('p', status);
            builder.append(posPower + '0', status);
            builder.append('-', status);
        } else if (posPower <= 15) {
            builder.append("p1", status);
            builder.append('0' + (posPower % 10), status);
            builder.append('-', status);
        } else {
            status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        if (U_FAILURE(status)) {
            return;
        }

        if (siPrefix != UMEASURE_SI_PREFIX_ONE) {
            for (const auto& siPrefixInfo : gSIPrefixStrings) {
                if (siPrefixInfo.value == siPrefix) {
                    builder.append(siPrefixInfo.string, status);
                    break;
                }
            }
        }
        if (U_FAILURE(status)) {
            return;
        }

        builder.append(id, status);
    }

    char* build(UErrorCode& status) {
        if (U_FAILURE(status)) {
            return nullptr;
        }
        CharString builder;
        if (power < 0) {
            builder.append("one-per-", status);
        }
        appendTo(builder, status);
        return builder.cloneData(status);
    }
};

class CompoundUnit {
public:
    typedef MaybeStackVector<SingleUnit, 3> SingleUnitList;

    void append(SingleUnit&& singleUnit, UErrorCode& status) {
        if (singleUnit.power >= 0) {
            appendImpl(numerator, std::move(singleUnit), status);
        } else {
            appendImpl(denominator, std::move(singleUnit), status);
        }
    }

    void takeReciprocal() {
        auto temp = std::move(numerator);
        numerator = std::move(denominator);
        denominator = std::move(temp);
    }

    void appendTo(CharString& builder, UErrorCode& status) const {
        if (numerator.length() == 0) {
            builder.append("one", status);
        } else {
            appendToImpl(numerator, builder, status);
        }
        if (denominator.length() > 0) {
            builder.append("-per-", status);
            appendToImpl(denominator, builder, status);
        }
    }

    char* build(UErrorCode& status) {
        if (U_FAILURE(status)) {
            return nullptr;
        }
        CharString builder;
        appendTo(builder, status);
        return builder.cloneData(status);
    }

    const SingleUnitList& getNumeratorUnits() const {
        return numerator;
    }

    const SingleUnitList& getDenominatorUnits() const {
        return denominator;
    }

    bool isSingle() const {
        return numerator.length() + denominator.length() == 1;
    }

    bool isEmpty() const {
        return numerator.length() + denominator.length() == 0;
    }

private:
    SingleUnitList numerator;
    SingleUnitList denominator;

    void appendToImpl(const SingleUnitList& unitList, CharString& builder, UErrorCode& status) const {
        bool first = true;
        int32_t len = unitList.length();
        for (int32_t i = 0; i < len; i++) {
            if (first) {
                first = false;
            } else {
                builder.append('-', status);
            }
            unitList[i]->appendTo(builder, status);
        }
    }

    void appendImpl(SingleUnitList& unitList, SingleUnit&& singleUnit, UErrorCode& status) {
        // Check that the same simple unit doesn't already exist
        for (int32_t i = 0; i < unitList.length(); i++) {
            SingleUnit* candidate = unitList[i];
            if (candidate->simpleUnitIndex == singleUnit.simpleUnitIndex
                    && candidate->siPrefix == singleUnit.siPrefix) {
                candidate->power += singleUnit.power;
                return;
            }
        }
        // Add a new unit
        SingleUnit* destination = unitList.emplaceBack();
        if (!destination) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *destination = std::move(singleUnit);
    }
};

class SequenceUnit {
public:
    typedef MaybeStackVector<CompoundUnit, 3> CompoundUnitList;

    void append(CompoundUnit&& compoundUnit, UErrorCode& status) {
        CompoundUnit* destination = units.emplaceBack();
        if (!destination) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *destination = std::move(compoundUnit);
    }

    void appendTo(CharString& builder, UErrorCode& status) const {
        if (units.length() == 0) {
            builder.append("one", status);
            return;
        }
        bool isFirst = true;
        for (int32_t i = 0; i < units.length(); i++) {
            if (isFirst) {
                isFirst = false;
            } else {
                builder.append('+', status);
            }
            units[i]->appendTo(builder, status);
        }
    }

    char* build(UErrorCode& status) {
        if (U_FAILURE(status)) {
            return nullptr;
        }
        CharString builder;
        appendTo(builder, status);
        return builder.cloneData(status);
    }

    const CompoundUnitList& getUnits() const {
        return units;
    }

private:
    CompoundUnitList units;
};

class Parser {
public:
    static Parser from(StringPiece source, UErrorCode& status) {
        if (U_FAILURE(status)) {
            return Parser();
        }
        umtx_initOnce(gUnitExtrasInitOnce, &initUnitExtras, status);
        if (U_FAILURE(status)) {
            return Parser();
        }
        return Parser(source);
    }

    bool hasNext() const {
        return fIndex < fSource.length();
    }

    SingleUnit getOnlySingleUnit(UErrorCode& status) {
        bool sawPlus;
        SingleUnit retval;
        nextSingleUnit(retval, sawPlus, status);
        if (U_FAILURE(status)) {
            return retval;
        }
        if (sawPlus || hasNext()) {
            // Expected to find only one unit in the string
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return retval;
        }
        return retval;
    }

    void nextCompoundUnit(CompoundUnit& result, UErrorCode& status) {
        bool sawPlus;
        if (U_FAILURE(status)) {
            return;
        }
        while (hasNext()) {
            int32_t previ = fIndex;
            SingleUnit singleUnit;
            nextSingleUnit(singleUnit, sawPlus, status);
            if (U_FAILURE(status)) {
                return;
            }
            if (sawPlus && !result.isEmpty()) {
                fIndex = previ;
                break;
            }
            result.append(std::move(singleUnit), status);
        }
        return;
    }

    CompoundUnit getOnlyCompoundUnit(UErrorCode& status) {
        CompoundUnit retval;
        nextCompoundUnit(retval, status);
        if (U_FAILURE(status)) {
            return retval;
        }
        if (hasNext()) {
            // Expected to find only one unit in the string
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return retval;
        }
        return retval;
    }

    SequenceUnit getOnlySequenceUnit(UErrorCode& status) {
        SequenceUnit retval;
        while (hasNext()) {
            CompoundUnit compoundUnit;
            nextCompoundUnit(compoundUnit, status);
            if (U_FAILURE(status)) {
                return retval;
            }
            retval.append(std::move(compoundUnit), status);
        }
        return retval;
    }

private:
    int32_t fIndex = 0;
    StringPiece fSource;
    UCharsTrie fTrie;

    bool fAfterPer = false;

    Parser() : fSource(""), fTrie(u"") {}

    Parser(StringPiece source)
        : fSource(source), fTrie(kSerializedUnitExtrasStemTrie) {}

    Token nextToken(UErrorCode& status) {
        fTrie.reset();
        int32_t match = -1;
        int32_t previ = -1;
        do {
            fTrie.next(fSource.data()[fIndex++]);
            if (fTrie.current() == USTRINGTRIE_NO_MATCH) {
                break;
            } else if (fTrie.current() == USTRINGTRIE_NO_VALUE) {
                continue;
            } else if (fTrie.current() == USTRINGTRIE_FINAL_VALUE) {
                match = fTrie.getValue();
                previ = fIndex;
                break;
            } else if (fTrie.current() == USTRINGTRIE_INTERMEDIATE_VALUE) {
                match = fTrie.getValue();
                previ = fIndex;
                continue;
            } else {
                UPRV_UNREACHABLE;
            }
        } while (fIndex < fSource.length());

        if (match < 0) {
            // TODO: Make a new status code?
            status = U_ILLEGAL_ARGUMENT_ERROR;
        } else {
            fIndex = previ;
        }
        return Token(match);
    }

    void nextSingleUnit(SingleUnit& result, bool& sawPlus, UErrorCode& status) {
        sawPlus = false;
        if (U_FAILURE(status)) {
            return;
        }

        if (!hasNext()) {
            // probably "one"
            return;
        }

        // state:
        // 0 = no tokens seen yet (will accept power, SI prefix, or simple unit)
        // 1 = power token seen (will not accept another power token)
        // 2 = SI prefix token seen (will not accept a power or SI prefix token)
        int32_t state = 0;
        int32_t previ = fIndex;

        // Maybe read a compound part
        if (fIndex != 0) {
            Token token = nextToken(status);
            if (U_FAILURE(status)) {
                return;
            }
            if (token.getType() != Token::TYPE_COMPOUND_PART) {
                goto fail;
            }
            switch (token.getMatch()) {
                case COMPOUND_PART_PER:
                    if (fAfterPer) {
                        goto fail;
                    }
                    fAfterPer = true;
                    result.power = -1;
                    break;

                case COMPOUND_PART_TIMES:
                    break;

                case COMPOUND_PART_PLUS:
                    sawPlus = true;
                    fAfterPer = false;
                    break;
            }
            previ = fIndex;
        }

        // Read a unit
        while (hasNext()) {
            Token token = nextToken(status);
            if (U_FAILURE(status)) {
                return;
            }

            switch (token.getType()) {
                case Token::TYPE_POWER_PART:
                    if (state > 0) {
                        goto fail;
                    }
                    result.power *= token.getPower();
                    previ = fIndex;
                    state = 1;
                    break;

                case Token::TYPE_SI_PREFIX:
                    if (state > 1) {
                        goto fail;
                    }
                    result.siPrefix = token.getSIPrefix();
                    previ = fIndex;
                    state = 2;
                    break;

                case Token::TYPE_ONE:
                    // Skip "one" and go to the next unit
                    return nextSingleUnit(result, sawPlus, status);

                case Token::TYPE_SIMPLE_UNIT:
                    result.simpleUnitIndex = token.getSimpleUnitIndex();
                    result.id = fSource.substr(previ, fIndex - previ);
                    return;

                default:
                    goto fail;
            }
        }

        fail:
            // TODO: Make a new status code?
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
    }
};

} // namespace


MeasureUnit MeasureUnit::forIdentifier(StringPiece identifier, UErrorCode& status) {
    return Parser::from(identifier, status).getOnlySequenceUnit(status).build(status);
}

UMeasureUnitComplexity MeasureUnit::getComplexity(UErrorCode& status) const {
    const char* id = getIdentifier();
    Parser parser = Parser::from(id, status);
    if (U_FAILURE(status)) {
        return UMEASURE_UNIT_SINGLE;
    }

    CompoundUnit compoundUnit;
    parser.nextCompoundUnit(compoundUnit, status);
    if (parser.hasNext()) {
        return UMEASURE_UNIT_SEQUENCE;
    } else if (compoundUnit.isSingle()) {
        return UMEASURE_UNIT_SINGLE;
    } else {
        return UMEASURE_UNIT_COMPOUND;
    }
}

UMeasureSIPrefix MeasureUnit::getSIPrefix(UErrorCode& status) const {
    const char* id = getIdentifier();
    return Parser::from(id, status).getOnlySingleUnit(status).siPrefix;
}

MeasureUnit MeasureUnit::withSIPrefix(UMeasureSIPrefix prefix, UErrorCode& status) const {
    const char* id = getIdentifier();
    SingleUnit singleUnit = Parser::from(id, status).getOnlySingleUnit(status);
    singleUnit.siPrefix = prefix;
    return singleUnit.build(status);
}

int8_t MeasureUnit::getPower(UErrorCode& status) const {
    const char* id = getIdentifier();
    return Parser::from(id, status).getOnlySingleUnit(status).power;
}

MeasureUnit MeasureUnit::withPower(int8_t power, UErrorCode& status) const {
    const char* id = getIdentifier();
    SingleUnit singleUnit = Parser::from(id, status).getOnlySingleUnit(status);
    singleUnit.power = power;
    return singleUnit.build(status);
}

MeasureUnit MeasureUnit::reciprocal(UErrorCode& status) const {
    const char* id = getIdentifier();
    CompoundUnit compoundUnit = Parser::from(id, status).getOnlyCompoundUnit(status);
    compoundUnit.takeReciprocal();
    return compoundUnit.build(status);
}

MeasureUnit MeasureUnit::product(const MeasureUnit& other, UErrorCode& status) const {
    const char* id = getIdentifier();
    CompoundUnit compoundUnit = Parser::from(id, status).getOnlyCompoundUnit(status);
    if (U_FAILURE(status)) {
        return *this;
    }

    // Append other's first CompoundUnit to compoundUnit, then assert other has only one
    Parser otherParser = Parser::from(other.getIdentifier(), status);
    otherParser.nextCompoundUnit(compoundUnit, status);
    if (U_FAILURE(status)) {
        return *this;
    }
    if (otherParser.hasNext()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return *this;
    }

    return compoundUnit.build(status);
}

LocalArray<MeasureUnit> MeasureUnit::getSingleUnits(UErrorCode& status) const {
    const char* id = getIdentifier();
    CompoundUnit compoundUnit = Parser::from(id, status).getOnlyCompoundUnit(status);
    if (U_FAILURE(status)) {
        return LocalArray<MeasureUnit>();
    }

    const CompoundUnit::SingleUnitList& numerator = compoundUnit.getNumeratorUnits();
    const CompoundUnit::SingleUnitList& denominator = compoundUnit.getDenominatorUnits();
    int32_t count = numerator.length() + denominator.length();
    MeasureUnit* arr = new MeasureUnit[count];

    int32_t i = 0;
    for (int32_t j = 0; j < numerator.length(); j++) {
        arr[i++] = numerator[j]->build(status);
    }
    for (int32_t j = 0; j < denominator.length(); j++) {
        arr[i++] = denominator[j]->build(status);
    }

    return LocalArray<MeasureUnit>::withLength(arr, count);
}

LocalArray<MeasureUnit> MeasureUnit::getCompoundUnits(UErrorCode& status) const {
    const char* id = getIdentifier();
    SequenceUnit sequenceUnit = Parser::from(id, status).getOnlySequenceUnit(status);
    if (U_FAILURE(status)) {
        return LocalArray<MeasureUnit>();
    }

    const SequenceUnit::CompoundUnitList& unitVector = sequenceUnit.getUnits();
    int32_t count = unitVector.length();
    MeasureUnit* arr = new MeasureUnit[count];

    for (int32_t i = 0; i < count; i++) {
        arr[i] = unitVector[i]->build(status);
    }

    return LocalArray<MeasureUnit>::withLength(arr, count);
}


U_NAMESPACE_END

#endif /* !UNCONFIG_NO_FORMATTING */