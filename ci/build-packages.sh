#!/usr/bin/env bash
#
# Run the repo's build and collect every .eap into releases/.
# The build command and extra env come from .acap.json.

set -euo pipefail

cd "$(dirname "$0")/.."

CONFIG=.acap.json
cfg() { jq -r "$1" "$CONFIG"; }

COMMAND=$(cfg '.build.command')

while IFS=$'\t' read -r key value; do
	[ -n "$key" ] || continue
	value=${value//\$\{VERSION\}/${VERSION:-}}
	export "$key=$value"
	echo "env $key=$value"
done < <(cfg '.build.env | to_entries[]? | [.key, .value] | @tsv')

rm -rf releases
mkdir -p releases

echo "== $COMMAND"
eval "$COMMAND"

# Repos drop packages in the root, build/, build_<arch>/ or releases/ depending
# on the repo, so collect from anywhere except releases/ itself.
found=0
while IFS= read -r package; do
	[ -n "$package" ] || continue
	mv "$package" releases/
	found=$((found + 1))
done < <(find . -name '*.eap' -not -path './releases/*' -not -path './.git/*')

[ "$found" -gt 0 ] || {
	echo "no .eap produced" >&2
	exit 1
}

echo "collected $found package(s):"
ls -lh releases/
