#!/usr/bin/env bash
#
# Decide which version this repo should build, from .acap.json.
# Writes build/release/version/upstream to GITHUB_OUTPUT under CI, and always
# prints the decision so it can be run locally to preview.
#
# Policies:
#   mirror  the ACAP version follows the upstream version exactly.
#   patch   upstream is tracked through a pin; our own last digit is bumped.

set -euo pipefail

cd "$(dirname "$0")/.."

CONFIG=.acap.json
[ -f "$CONFIG" ] || {
	echo "missing $CONFIG" >&2
	exit 1
}

cfg() { jq -r "$1" "$CONFIG"; }

POLICY=$(cfg '.versionPolicy')
UPSTREAM_TYPE=$(cfg '.upstream.type')
EVENT_NAME=${EVENT_NAME:-manual}
INPUT_VERSION=${INPUT_VERSION:-}
INPUT_FORCE=${INPUT_FORCE:-false}

current_version() {
	local manifest conf
	manifest=$(find . -path '*/app/manifest.json' -not -path './node_modules/*' | sort | head -1)
	if [ -n "$manifest" ]; then
		jq -r '.acapPackageConf.setup.version' "$manifest"
		return
	fi
	conf=$(find . -path '*/app/package.conf' | sort | head -1)
	[ -n "$conf" ] && sed -n 's/^VERSION=//p' "$conf" | head -1
}

# Current value of the first pin, used by "patch" to detect upstream movement.
pin_value() {
	local file arg gomod module
	file=$(cfg '.pins[0].file // empty')
	arg=$(cfg '.pins[0].arg // empty')
	if [ -n "$file" ] && [ -n "$arg" ] && [ -f "$file" ]; then
		sed -n "s/^ARG ${arg}=//p" "$file" | head -1
		return
	fi
	gomod=$(cfg '.upstream.goMod // empty')
	module=$(cfg '.upstream.module // empty')
	if [ -n "$gomod" ] && [ -f "$gomod" ]; then
		# The module may appear as "require mod ver" or as "mod ver" inside a
		# require block, so take the field after the module name wherever it is.
		awk -v m="$module" '{ for (i = 1; i < NF; i++) if ($i == m) { print $(i + 1); exit } }' "$gomod"
	fi
}

# FFmpeg and openvpn3 publish no releases, and their tag lists contain names
# that are not versions, hence the explicit pattern per repo.
upstream_version() {
	case "$UPSTREAM_TYPE" in
	github-release)
		local tag
		tag=$(gh api "repos/$(cfg '.upstream.repo')/releases/latest" --jq '.tag_name')
		[ "$(cfg '.upstream.stripV // false')" = true ] && tag=${tag#v}
		printf '%s\n' "$tag"
		;;
	github-tag)
		gh api "repos/$(cfg '.upstream.repo')/tags?per_page=100" --paginate --jq '.[].name' |
			grep -E "$(cfg '.upstream.tagPattern')" |
			sed "s|^$(cfg '.upstream.strip // empty')||" |
			sort -V | tail -1
		;;
	go-module)
		curl -fsSL "https://proxy.golang.org/$(cfg '.upstream.module')/@latest" | jq -r '.Version'
		;;
	script)
		bash "$(cfg '.upstream.script')"
		;;
	*)
		echo ''
		;;
	esac
}

bump_patch() {
	local major minor patch
	IFS='.' read -r major minor patch <<<"$1"
	printf '%s.%s.%s\n' "${major:-0}" "${minor:-0}" "$((${patch:-0} + 1))"
}

# True when $1 is a strictly higher version than $2.
version_gt() {
	[ "$1" != "$2" ] && [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -1)" = "$1" ]
}

CURRENT=$(current_version)
UPSTREAM=$(upstream_version || true)

BUILD=false
RELEASE=false
TARGET="$CURRENT"

if [ -n "$INPUT_VERSION" ]; then
	TARGET=${INPUT_VERSION#v}
	BUILD=true
	RELEASE=true
elif [ "$POLICY" = mirror ]; then
	if [ -n "$UPSTREAM" ] && [ "$UPSTREAM" != "$CURRENT" ]; then
		if version_gt "$UPSTREAM" "$CURRENT"; then
			# Upstream is ahead: adopt its version.
			TARGET="$UPSTREAM"
			BUILD=true
			RELEASE=true
		elif [ "$UPSTREAM" != "$(pin_value)" ]; then
			# Our line already ran past upstream, so keep moving forward on it
			# rather than emitting a lower version that clashes with old tags.
			TARGET=$(bump_patch "$CURRENT")
			BUILD=true
			RELEASE=true
		fi
	fi
elif [ "$POLICY" = patch ]; then
	if [ -n "$UPSTREAM" ] && [ "$UPSTREAM" != "$(pin_value)" ]; then
		TARGET=$(bump_patch "$CURRENT")
		BUILD=true
		RELEASE=true
	fi
fi

# Pull requests build for validation but never release.
if [ "$EVENT_NAME" = pull_request ]; then
	BUILD=true
	RELEASE=false
fi

if [ "$INPUT_FORCE" = true ]; then
	BUILD=true
	RELEASE=true
fi

cat <<EOF
policy   : $POLICY
current  : $CURRENT
upstream : ${UPSTREAM:-n/a}
target   : $TARGET
build    : $BUILD
release  : $RELEASE
EOF

if [ -n "${GITHUB_OUTPUT:-}" ]; then
	{
		echo "build=$BUILD"
		echo "release=$RELEASE"
		echo "version=$TARGET"
		echo "upstream=$UPSTREAM"
	} >>"$GITHUB_OUTPUT"
fi
