#include "format_context.h"

#include "format_float.inl"

LSTD_BEGIN_NAMESPACE

namespace fmt {

file_scope utf8 DIGITS[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

template <typename UInt>
file_scope utf8 *format_uint_decimal(utf8 *buffer, UInt value, s64 formattedSize, const string &thousandsSep = "") {
    u32 digitIndex = 0;

    buffer += formattedSize;
    while (value >= 100) {
        u32 index = (u32)(value % 100) * 2;
        value /= 100;
        *--buffer = DIGITS[index + 1];
        if (++digitIndex % 3 == 0) {
            buffer -= thousandsSep.Count;
            copy_memory(buffer, thousandsSep.Data, thousandsSep.Count);
        }
        *--buffer = DIGITS[index];
        if (++digitIndex % 3 == 0) {
            buffer -= thousandsSep.Count;
            copy_memory(buffer, thousandsSep.Data, thousandsSep.Count);
        }
    }

    if (value < 10) {
        *--buffer = (utf8) ('0' + value);
        return buffer;
    }

    u32 index = (u32) value * 2;
    *--buffer = DIGITS[index + 1];
    if (++digitIndex % 3 == 0) {
        buffer -= thousandsSep.Count;
        copy_memory(buffer, thousandsSep.Data, thousandsSep.Count);
    }
    *--buffer = DIGITS[index];

    return buffer;
}

template <u32 BASE_BITS, typename UInt>
file_scope utf8 *format_uint_base(utf8 *buffer, UInt value, s64 formattedSize, bool upper = false) {
    buffer += formattedSize;
    do {
        const utf8 *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
        u32 digit = (value & ((1 << BASE_BITS) - 1));
        *--buffer = (utf8)(BASE_BITS < 4 ? (utf8)('0' + digit) : digits[digit]);
    } while ((value >>= BASE_BITS) != 0);
    return buffer;
}

// Writes pad code points and the actual contents with f(),
// _fSize_ needs to be the size of the output from _f_ in code points (in order to calculate padding properly)
template <typename F>
file_scope void write_padded_helper(format_context *f, const format_specs &specs, F &&func, s64 fSize) {
    u32 padding = (u32)(specs.Width > fSize ? specs.Width - fSize : 0);
    if (specs.Align == alignment::RIGHT) {
        For(range(padding)) f->write_no_specs(specs.Fill);
        func();
    } else if (specs.Align == alignment::CENTER) {
        u32 leftPadding = padding / 2;
        For(range(leftPadding)) f->write_no_specs(specs.Fill);
        func();
        For(range(padding - leftPadding)) f->write_no_specs(specs.Fill);
    } else {
        func();
        For(range(padding)) f->write_no_specs(specs.Fill);
    }
}

void format_context_write(io::writer *w, const byte *data, s64 size) {
    auto *f = (format_context *) w;

    if (!f->Specs) {
        f->write_no_specs((const utf8 *) data, size);
        return;
    }

    if (f->Specs->Type) {
        if (f->Specs->Type == 'p') {
            f->write((void *) data);
            return;
        }
        if (f->Specs->Type != 's') {
            f->on_error("Invalid type specifier for a string", f->Parse.It.Data - f->Parse.FormatString.Data - 1);
            return;
        }
    }

    // 'p' wasn't specified, not treating as formatting a pointer
    s64 length = utf8_length((const utf8 *) data, size);

    // Adjust size for specified precision
    if (f->Specs->Precision != -1) {
        assert(f->Specs->Precision >= 0);
        length = f->Specs->Precision;
        size = get_cp_at_index((const utf8 *) data, length, length, true) - (const utf8 *) data;
    }
    write_padded_helper(
        f, *f->Specs, [&]() { f->write_no_specs((const utf8 *) data, size); }, length);
}

void format_context_flush(io::writer *w) {
    auto *f = (format_context *) w;
    f->Out->flush();
}

// @Threadsafety ???
file_scope utf8 U64_FORMAT_BUFFER[::type::numeric_info<u64>::digits10 + 1];

void format_context::write(const void *value) {
    if (Specs && Specs->Type && Specs->Type != 'p') {
        on_error("Invalid type specifier for a pointer", Parse.It.Data - Parse.FormatString.Data - 1);
        return;
    }

    auto uptr = bit_cast<u64>(value);
    u32 numDigits = count_digits<4>(uptr);

    auto f = [&, this]() {
        this->write_no_specs(U'0');
        this->write_no_specs(U'x');

        utf8 formatBuffer[::type::numeric_info<u64>::digits / 4 + 2];
        auto *p = format_uint_base<4>(formatBuffer, uptr, numDigits);
        this->write_no_specs(p, formatBuffer + numDigits - p);
    };

    if (!Specs) {
        f();
        return;
    }

    format_specs specs = *Specs;
    if (specs.Align == alignment::NONE) specs.Align = alignment::RIGHT;
    write_padded_helper(this, specs, f, numDigits + 2);
}

void format_context::write_u64(u64 value, bool negative, format_specs specs) {
    utf8 type = specs.Type;
    if (!type) type = 'd';

    s64 numDigits;
    if (type == 'd' || type == 'n') {
        numDigits = count_digits(value);
    } else if (to_lower(type) == 'b') {
        numDigits = count_digits<1>(value);
    } else if (type == 'o') {
        numDigits = count_digits<3>(value);
    } else if (to_lower(type) == 'x') {
        numDigits = count_digits<4>(value);
    } else if (type == 'c') {
        if (specs.Align == alignment::NUMERIC || specs.Sign != sign::NONE || specs.Hash) {
            on_error("Invalid format specifier(s) for code point - code points can't have numeric alignment, signs or #", Parse.It.Data - Parse.FormatString.Data);
            return;
        }
        auto cp = (utf32) value;
        write_padded_helper(
            this, specs, [&]() { this->write_no_specs(cp); }, get_size_of_cp(cp));
        return;
    } else {
        on_error("Invalid type specifier for an integer", Parse.It.Data - Parse.FormatString.Data - 1);
        return;
    }

    utf8 prefixBuffer[4];
    utf8 *prefixPointer = prefixBuffer;

    if (negative) {
        *prefixPointer++ = '-';
    } else if (specs.Sign == sign::PLUS) {
        *prefixPointer++ = '+';
    } else if (specs.Sign == sign::SPACE) {
        *prefixPointer++ = ' ';
    }

    if ((to_lower(type) == 'x' || to_lower(type) == 'b') && specs.Hash) {
        *prefixPointer++ = '0';
        *prefixPointer++ = type;
    }

    // Octal prefix '0' is counted as a digit,
    // so only add it if precision is not greater than the number of digits.
    if (type == 'o' && specs.Hash) {
        if (specs.Precision == -1 || specs.Precision > numDigits) *prefixPointer++ = '0';
    }

    auto prefix = string(prefixBuffer, prefixPointer - prefixBuffer);

    s64 formattedSize = prefix.Length + numDigits;
    s64 padding = 0;
    if (specs.Align == alignment::NUMERIC) {
        if (specs.Width > formattedSize) {
            padding = specs.Width - formattedSize;
            formattedSize = specs.Width;
        }
    } else if (specs.Precision > (s32) numDigits) {
        formattedSize = (u32) prefix.Length + (u32) specs.Precision;
        padding = (u32) specs.Precision - numDigits;
        specs.Fill = '0';
    }
    if (specs.Align == alignment::NONE) specs.Align = alignment::RIGHT;

    type = (utf8) to_lower(type);
    if (type == 'd') {
        write_padded_helper(
            this, specs,
            [&]() {
                if (prefix.Length) this->write_no_specs(prefix);
                For(range(padding)) this->write_no_specs(specs.Fill);
                auto *p = format_uint_decimal(U64_FORMAT_BUFFER, value, numDigits);
                this->write_no_specs(p, U64_FORMAT_BUFFER + numDigits - p);
            },
            formattedSize);
    } else if (type == 'b') {
        write_padded_helper(
            this, specs,
            [&]() {
                if (prefix.Length) this->write_no_specs(prefix);
                For(range(padding)) this->write_no_specs(specs.Fill);
                auto *p = format_uint_base<1>(U64_FORMAT_BUFFER, value, numDigits);
                this->write_no_specs(p, U64_FORMAT_BUFFER + numDigits - p);
            },
            formattedSize);
    } else if (type == 'o') {
        write_padded_helper(
            this, specs,
            [&]() {
                if (prefix.Length) this->write_no_specs(prefix);
                For(range(padding)) this->write_no_specs(specs.Fill);
                auto *p = format_uint_base<3>(U64_FORMAT_BUFFER, value, numDigits);
                this->write_no_specs(p, U64_FORMAT_BUFFER + numDigits - p);
            },
            formattedSize);
    } else if (type == 'x') {
        write_padded_helper(
            this, specs,
            [&]() {
                if (prefix.Length) this->write_no_specs(prefix);
                For(range(padding)) this->write_no_specs(specs.Fill);
                auto *p = format_uint_base<4>(U64_FORMAT_BUFFER, value, numDigits, is_upper(specs.Type));
                this->write_no_specs(p, U64_FORMAT_BUFFER + numDigits - p);
            },
            formattedSize);
    } else if (type == 'n') {
        formattedSize += ((numDigits - 1) / 3);
        write_padded_helper(
            this, specs,
            [&]() {
                if (prefix.Length) this->write_no_specs(prefix);
                For(range(padding)) this->write_no_specs(specs.Fill);
                auto *p = format_uint_decimal(U64_FORMAT_BUFFER, value, formattedSize, "," /*@Locale*/);
                this->write_no_specs(p, U64_FORMAT_BUFFER + formattedSize - p);
            },
            formattedSize);
    } else {
        assert(false && "Invalid type");  // sanity
    }
}

// Writes a float with given formatting specs
void format_context::write_f64(f64 value, format_specs specs) {
    utf8 type = specs.Type;
    if (type) {
        utf8 lower = (utf8) to_lower(type);
        if (lower != 'g' && lower != 'e' && lower != '%' && lower != 'f' && lower != 'a') {
            on_error("Invalid type specifier for a float", Parse.It.Data - Parse.FormatString.Data - 1);
            return;
        }
    } else {
        type = 'g';
    }

    bool percentage = specs.Type == '%';

    utf32 sign = 0;

    ieee754_f64 bits;
    bits.F = value;

    // Check the sign bit instead of value < 0 since the latter is always false for NaN
    if (bits.ieee.S) {
        sign = '-';
        value = -value;
    } else if (specs.Sign == sign::PLUS) {
        sign = '+';
    } else if (specs.Sign == sign::SPACE) {
        sign = ' ';
    }

    // Handle INF or NAN
    if (bits.ieee.E == 2047) {
        write_padded_helper(
            this, specs,
            [&, this]() {
                if (sign) this->write_no_specs(sign);
                this->write_no_specs((bits.W & ((1ll << 52) - 1)) ? (is_upper(specs.Type) ? "NAN" : "nan")
                                                                  : (is_upper(specs.Type) ? "INF" : "inf"));
                if (percentage) this->write_no_specs(U'%');
            },
            3 + (sign ? 1 : 0) + (percentage ? 1 : 0));
        return;
    }

    if (percentage) {
        value *= 100;
        type = 'f';
    }

    // @Locale The decimal point written in _internal::format_float_ should be locale-dependent.
    // Also if we decide to add a thousands separator we should do it inside _format_float_
    stack_dynamic_buffer<512> formatBuffer;
    defer(free(formatBuffer));

    format_float(
        [](void *user, utf8 *buf, s64 length) {
            auto *fb = (stack_dynamic_buffer<512> *) user;
            fb->Count += length;
            return (utf8 *) fb->Data + fb->Count;
        },
        &formatBuffer, (utf8 *) formatBuffer.Data, type, value, specs.Precision);

    // Note: We set _type_ to 'g' if it's zero, but here we check specs.Type (which we didn't modify)
    // This is because '0' is similar to 'g', except that it prints at least one digit after the decimal point,
    // which we do here (python-like formatting)
    if (!specs.Type) {
        auto *p = formatBuffer.begin(), *end = formatBuffer.end();
        while (p < end && is_digit(*p)) ++p;
        if (p < end && to_lower(*p) != 'e') {
            ++p;
            if (*p == '0') ++p;
            while (p != end && *p >= '1' && *p <= '9') ++p;

            byte *where = p;
            while (p != end && *p == '0') ++p;

            if (p == end || !is_digit(*p)) {
                if (p != end) copy_memory(where, p, (s64)(end - p));
                formatBuffer.Count -= (s64)(p - where);
            }
        } else if (p == end) {
            // There was no dot at all
            append_pointer_and_size(formatBuffer, (byte *) ".0", 2);
        }
    }

    if (percentage) append(formatBuffer, '%');

    if (specs.Align == alignment::NUMERIC) {
        if (sign) {
            write_no_specs(sign);
            sign = 0;
            if (specs.Width) --specs.Width;
        }
        specs.Align = alignment::RIGHT;
    } else if (specs.Align == alignment::NONE) {
        specs.Align = alignment::RIGHT;
    }

    auto formattedSize = formatBuffer.Count + (sign ? 1 : 0);
    write_padded_helper(
        this, specs,
        [&, this]() {
            if (sign) this->write_no_specs(sign);
            this->write_no_specs((utf8 *) formatBuffer.Data, formatBuffer.Count);
        },
        formattedSize);
}

struct width_checker {
    format_context *F;

    template <typename T>
    u32 operator()(T value) {
        if constexpr (::type::is_integer_v<T>) {
            if (sign_bit(value)) {
                F->on_error("Negative width");
                return (u32) -1;
            } else if ((u64) value > ::type::numeric_info<s32>::max()) {
                F->on_error("Width value is too big");
                return (u32) -1;
            }
            return (u32) value;
        } else {
            F->on_error("Width was not an integer");
            return (u32) -1;
        }
    }
};

struct precision_checker {
    format_context *F;

    template <typename T>
    s32 operator()(T value) {
        if constexpr (::type::is_integer_v<T>) {
            if (sign_bit(value)) {
                F->on_error("Negative precision");
                return -1;
            } else if ((u64) value > ::type::numeric_info<s32>::max()) {
                F->on_error("Precision value is too big");
                return -1;
            }
            return (s32) value;
        } else {
            F->on_error("Precision was not an integer");
            return -1;
        }
    }
};

arg format_context::get_arg_from_index(s64 index) {
    if (index < Args.Count) {
        return Args.get_arg(index);
    }
    on_error("Argument index out of range");
    return {};
}

bool format_context::handle_dynamic_specs() {
    assert(Specs);

    if (Specs->WidthIndex != -1) {
        auto width = get_arg_from_index(Specs->WidthIndex);
        if (width.Type != type::NONE) {
            Specs->Width = visit_fmt_arg(width_checker{this}, width);
            if (Specs->Width == (u32) -1) return false;
        }
    }
    if (Specs->PrecisionIndex != -1) {
        auto precision = get_arg_from_index(Specs->PrecisionIndex);
        if (precision.Type != type::NONE) {
            Specs->Precision = visit_fmt_arg(precision_checker{this}, precision);
            if (Specs->Precision == ::type::numeric_info<s32>::min()) return false;
        }
    }

    return true;
}
}  // namespace fmt

LSTD_END_NAMESPACE
