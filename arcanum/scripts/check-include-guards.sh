#!/usr/bin/env bash
# Validates that each .h file under arcanum/ has an include guard
# matching its arcanum-relative path, uppercased with `/` and `.`
# replaced by `_`.
#
# Example: ecsl/IR/ECSLDialect.h -> ECSL_IR_ECSLDIALECT_H
#
# Run from the arcanum/ directory.
set -euo pipefail

fail=0
while IFS= read -r -d '' file; do
  expected=$(echo "$file" | tr '[:lower:]/.' '[:upper:]__')
  ifndef=$(grep -m1 '^#ifndef ' "$file" | awk '{print $2}' || true)
  define=$(grep -m1 '^#define ' "$file" | awk '{print $2}' || true)
  if [[ "$ifndef" != "$expected" || "$define" != "$expected" ]]; then
    echo "ERROR: $file: include guard mismatch"
    echo "  expected: $expected"
    echo "  #ifndef:  ${ifndef:-<missing>}"
    echo "  #define:  ${define:-<missing>}"
    fail=1
  fi
done < <(git ls-files -z '*.h')

exit $fail
