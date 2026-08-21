#!/usr/bin/env bash
#
# Write a version into every place this repo records it and refresh the
# upstream pins declared in .acap.json.
#
# Usage: ci/apply-version.sh <version> [upstream-version]

set -euo pipefail

cd "$(dirname "$0")/.."

VERSION=${1:?version required}
UPSTREAM=${2:-}

CONFIG=.acap.json
cfg() { jq -r "$1" "$CONFIG"; }

IFS='.' read -r MAJOR MINOR MICRO <<<"$VERSION"

while IFS= read -r manifest; do
	[ -n "$manifest" ] || continue
	tmp=$(mktemp)
	jq --arg v "$VERSION" '.acapPackageConf.setup.version = $v' "$manifest" >"$tmp"
	mv "$tmp" "$manifest"
	echo "version $VERSION -> $manifest"
done < <(find . -path '*/app/manifest.json' -not -path './node_modules/*' | sort)

while IFS= read -r conf; do
	[ -n "$conf" ] || continue
	sed -i.bak -E \
		-e "s/^APPMAJORVERSION=.*/APPMAJORVERSION=${MAJOR}/" \
		-e "s/^APPMINORVERSION=.*/APPMINORVERSION=${MINOR}/" \
		-e "s/^APPMICROVERSION=.*/APPMICROVERSION=${MICRO}/" \
		-e "s/^VERSION=.*/VERSION=${VERSION}/" \
		"$conf"
	rm -f "$conf.bak"
	echo "version $VERSION -> $conf"
done < <(find . -path '*/app/package.conf' | sort)

pin_count=$(cfg '.pins | length')
for ((i = 0; i < pin_count; i++)); do
	file=$(cfg ".pins[$i].file")
	arg=$(cfg ".pins[$i].arg")
	prefix=$(cfg ".pins[$i].prefix // empty")
	sha_url=$(cfg ".pins[$i].sha256Url // empty")
	[ -f "$file" ] || {
		echo "pin target missing: $file" >&2
		continue
	}

	if [ -n "$sha_url" ]; then
		# Checksum pins track the version pin, so the tarball is fetched and
		# hashed rather than substituted.
		url=${sha_url//\$\{VERSION\}/${UPSTREAM:-$VERSION}}
		echo "hashing $url"
		value=$(curl -fsSL "$url" | sha256sum | awk '{print $1}')
	else
		value="${prefix}${UPSTREAM:-$VERSION}"
	fi

	sed -i.bak -E "s|^ARG ${arg}=.*|ARG ${arg}=${value}|" "$file"
	rm -f "$file.bak"
	echo "pin ${arg}=${value} -> $file"
done

module=$(cfg '.upstream.module // empty')
gomod=$(cfg '.upstream.goMod // empty')
if [ -n "$module" ] && [ -n "$UPSTREAM" ] && [ -f "$gomod" ]; then
	(cd "$(dirname "$gomod")" && go get "${module}@${UPSTREAM}" && go mod tidy)
	echo "go module ${module}@${UPSTREAM}"
fi

if [ -f CHANGELOG.md ] && ! grep -qE "^## \[?${VERSION}\]?" CHANGELOG.md; then
	first_heading=$(grep -n -m1 '^## ' CHANGELOG.md | cut -d: -f1 || true)
	tmp=$(mktemp)
	{
		if [ -n "$first_heading" ]; then
			head -n "$((first_heading - 1))" CHANGELOG.md
		else
			cat CHANGELOG.md
			echo
		fi
		echo "## ${VERSION} - $(date +%Y-%m-%d)"
		echo
		if [ -n "$UPSTREAM" ]; then
			echo "- Update to upstream ${UPSTREAM}."
		else
			echo "- Release ${VERSION}."
		fi
		echo
		[ -n "$first_heading" ] && tail -n +"$first_heading" CHANGELOG.md
	} >"$tmp"
	mv "$tmp" CHANGELOG.md
	echo "changelog entry added for $VERSION"
fi
