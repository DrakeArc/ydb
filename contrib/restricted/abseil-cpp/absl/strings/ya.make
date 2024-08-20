# Generated by devtools/yamaker.

LIBRARY()

LICENSE(Apache-2.0)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

VERSION(20240722.0)

PEERDIR(
    contrib/restricted/abseil-cpp/absl/base
    contrib/restricted/abseil-cpp/absl/numeric
)

ADDINCL(
    GLOBAL contrib/restricted/abseil-cpp
)

NO_COMPILER_WARNINGS()

NO_UTIL()

SRCDIR(contrib/restricted/abseil-cpp/absl)

SRCS(
    crc/crc32c.cc
    crc/internal/cpu_detect.cc
    crc/internal/crc.cc
    crc/internal/crc_cord_state.cc
    crc/internal/crc_memcpy_fallback.cc
    crc/internal/crc_memcpy_x86_arm_combined.cc
    crc/internal/crc_non_temporal_memcpy.cc
    crc/internal/crc_x86_arm_combined.cc
    status/statusor.cc
    strings/ascii.cc
    strings/charconv.cc
    strings/cord.cc
    strings/cord_analysis.cc
    strings/cord_buffer.cc
    strings/escaping.cc
    strings/internal/charconv_bigint.cc
    strings/internal/charconv_parse.cc
    strings/internal/cord_internal.cc
    strings/internal/cord_rep_btree.cc
    strings/internal/cord_rep_btree_navigator.cc
    strings/internal/cord_rep_btree_reader.cc
    strings/internal/cord_rep_consume.cc
    strings/internal/cord_rep_crc.cc
    strings/internal/cordz_functions.cc
    strings/internal/cordz_handle.cc
    strings/internal/cordz_info.cc
    strings/internal/cordz_sample_token.cc
    strings/internal/damerau_levenshtein_distance.cc
    strings/internal/escaping.cc
    strings/internal/memutil.cc
    strings/internal/ostringstream.cc
    strings/internal/str_format/arg.cc
    strings/internal/str_format/bind.cc
    strings/internal/str_format/extension.cc
    strings/internal/str_format/float_conversion.cc
    strings/internal/str_format/output.cc
    strings/internal/str_format/parser.cc
    strings/internal/stringify_sink.cc
    strings/internal/utf8.cc
    strings/match.cc
    strings/numbers.cc
    strings/str_cat.cc
    strings/str_replace.cc
    strings/str_split.cc
    strings/string_view.cc
    strings/substitute.cc
)

END()
