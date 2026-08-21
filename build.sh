#!/usr/bin/env sh
# Build the Tailscale ACAP variants.
#
#   ./build.sh                 # build every variant
#   ./build.sh aarch64 arm     # build only the named variant folders
#
# Downloads the prebuilt Tailscale binaries, strips them, then builds each
# variant folder that contains an app/ directory. Variant folders map to the
# .eap suffixes used in releases: *_ROOT -> _root, *_acap3 -> _acap3.
#
# Override the container runtime with RUNTIME=docker|podman.
# TAILSCALE_VERSION pins the upstream binaries; it defaults to whatever
# ci/upstream-version.sh resolves.
set -eu

REPO_ROOT=$(cd -P "$(dirname "$0")" && pwd)
cd "$REPO_ROOT"

if [ -z "${RUNTIME:-}" ]; then
	if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
		RUNTIME=docker
	elif command -v podman >/dev/null 2>&1; then
		RUNTIME=podman
	else
		echo 'Error: neither docker nor podman found in PATH' >&2
		exit 1
	fi
fi
echo "==> Using container runtime: ${RUNTIME}"

VERSION="${TAILSCALE_VERSION:-$(sh ci/upstream-version.sh)}"
[ -n "$VERSION" ] || { echo 'Error: could not resolve a Tailscale version' >&2; exit 1; }
echo "==> Tailscale version: ${VERSION}"

# --- fetch and strip upstream binaries ---------------------------------------

BINS="${REPO_ROOT}/tailscale_bins"
rm -rf "$BINS"
mkdir -p "$BINS"

fetch_arch() {
	tgz_arch=$1
	suffix=$2
	echo "==> Downloading tailscale ${VERSION} (${tgz_arch})"
	curl -fsSL "https://pkgs.tailscale.com/stable/tailscale_${VERSION}_${tgz_arch}.tgz" \
		-o "${BINS}/ts_${suffix}.tgz"
	tar -xzf "${BINS}/ts_${suffix}.tgz" -C "$BINS" --strip-components=1
	mv "${BINS}/tailscale" "${BINS}/tailscale_${suffix}"
	mv "${BINS}/tailscaled" "${BINS}/tailscaled_${suffix}"
	rm -f "${BINS}/ts_${suffix}.tgz"
}

fetch_arch arm arm
fetch_arch arm64 arm64

# Stripping is optional: it only shrinks the package, so a missing cross
# binutils on a dev machine must not fail the build.
strip_with() {
	tool=$1
	suffix=$2
	command -v "$tool" >/dev/null 2>&1 || return 0
	"$tool" -s "${BINS}/tailscale_${suffix}" || true
	"$tool" -s "${BINS}/tailscaled_${suffix}" || true
}
strip_with aarch64-linux-gnu-strip arm64
strip_with arm-linux-gnueabihf-strip arm

# --- build variants -----------------------------------------------------------

echo '==> Cleaning old .eap files...'
rm -f "${REPO_ROOT}"/*.eap
rm -rf "${REPO_ROOT}/build"

build_variant() {
	folder=${1%/}
	[ -d "${folder}/app" ] || return 0
	[ "$folder" = common ] && return 0

	# aarch64/arm/aarch64_ROOT/arm_ROOT share sources via common/app; only
	# arm_acap3 carries its own self-contained app tree.
	case "$folder" in
	aarch64 | arm | aarch64_ROOT | arm_ROOT) lib_dir="common/app/lib" ;;
	*) lib_dir="${folder}/app/lib" ;;
	esac
	mkdir -p "$lib_dir"

	case "$folder" in
	arm*) src=arm ;;
	*) src=arm64 ;;
	esac
	cp "${BINS}/tailscale_${src}" "${lib_dir}/tailscale"
	cp "${BINS}/tailscaled_${src}" "${lib_dir}/tailscaled"

	case "$folder" in
	*_ROOT) variant="_root" ;;
	*_acap3) variant="_acap3" ;;
	*) variant="" ;;
	esac

	tag=$(echo "$folder" | tr '[:upper:]' '[:lower:]' | tr '/ ' '__')
	echo "==> Building ${folder}"
	"$RUNTIME" build -f "${folder}/Dockerfile" --tag "$tag" .

	out="${REPO_ROOT}/build/${tag}"
	mkdir -p "$out"
	cid=$("$RUNTIME" create "$tag")
	"$RUNTIME" cp "${cid}:/opt/app" "$out"
	"$RUNTIME" rm "$cid" >/dev/null

	find "$out" -type f -name '*.eap' | while read -r eap; do
		base=$(basename "$eap" .eap)
		mv "$eap" "${REPO_ROOT}/${base}${variant}.eap"
	done
}

if [ "$#" -eq 0 ]; then
	set -- */
fi
for v in "$@"; do
	build_variant "$v"
done

rm -rf "${REPO_ROOT}/build" "$BINS"

echo '==> Done!'
ls -lh "${REPO_ROOT}"/*.eap 2>/dev/null || true
