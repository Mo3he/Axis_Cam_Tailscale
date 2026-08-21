#!/usr/bin/env bash
#
# Generate release notes for a draft release.
#
# Usage: ci/release-notes.sh <version> [upstream-version] > notes.md

set -euo pipefail

cd "$(dirname "$0")/.."

VERSION=${1:?version required}
UPSTREAM=${2:-}

CONFIG=.acap.json
cfg() { jq -r "$1" "$CONFIG"; }

FRIENDLY=$(cfg '.friendlyName')
UPSTREAM_NAME=$(cfg '.upstream.name // .upstream.repo // .upstream.module // empty')
CHANGES_URL=$(cfg '.upstream.changesUrl // empty')
CHANGES_URL=${CHANGES_URL//\$\{UPSTREAM\}/$UPSTREAM}

# Previous tag, so the compare link points somewhere useful.
PREVIOUS=$(git tag --list 'v*' --sort=-v:refname | grep -v "^v${VERSION}$" | head -1 || true)

printf '%s %s\n\n' "$FRIENDLY" "$VERSION"

if [ -n "$UPSTREAM" ] && [ -n "$UPSTREAM_NAME" ]; then
	printf 'Packages **%s `%s`**.\n\n' "$UPSTREAM_NAME" "$UPSTREAM"
fi

if [ -n "$CHANGES_URL" ]; then
	printf '### Upstream changes\n\n%s\n\n' "$CHANGES_URL"
fi

if [ -f CHANGELOG.md ]; then
	# Pull just this version's section out of the changelog.
	section=$(awk -v v="$VERSION" '
		$0 ~ "^## \\[?" v "\\]?" { found = 1; next }
		found && /^## / { exit }
		found { print }
	' CHANGELOG.md | sed '/^[[:space:]]*$/d')
	if [ -n "$section" ]; then
		printf '### Changes\n\n%s\n\n' "$section"
	fi
fi

printf '### Packages\n\n'
printf 'Install the `signed_*.eap` matching your device architecture.\n\n'

# Only explain the unsigned variants when this release actually ships them.
unsigned_note=''
if compgen -G 'releases/*_acap3.eap' >/dev/null 2>&1; then
	unsigned_note='`_acap3`'
fi
if compgen -G 'releases/*_root.eap' >/dev/null 2>&1; then
	[ -n "$unsigned_note" ] && unsigned_note="${unsigned_note} and "
	unsigned_note="${unsigned_note}\`_root\`"
fi
if [ -n "$unsigned_note" ]; then
	printf 'Packages ending %s are published unsigned by design:\n' "$unsigned_note"
	printf 'they use manifest schema 1.x, which the Axis signing service does not accept.\n\n'
fi

if [ -n "$PREVIOUS" ] && [ -n "${GITHUB_REPOSITORY:-}" ]; then
	printf '**Full changelog**: https://github.com/%s/compare/%s...v%s\n' \
		"$GITHUB_REPOSITORY" "$PREVIOUS" "$VERSION"
fi
