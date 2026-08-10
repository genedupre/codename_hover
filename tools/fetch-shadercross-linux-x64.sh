#!/usr/bin/env bash

set -euo pipefail

readonly shadercross_run_id="28236415347"
readonly shadercross_artifact_name="SDL3_shadercross-linux-x64"
readonly shadercross_archive_name="SDL3_shadercross-3.0.0-linux-x64.tar.gz"
readonly shadercross_directory_name="SDL3_shadercross-3.0.0-linux-x64"
readonly expected_archive_sha256="252de380a0a4c6b5479419be3e4f00e419805fc99a41bae840096f0708ce3e15"
readonly expected_executable_sha256="03fbad1d5484253a06451c6d6c630d0624111f9665d35267d6f6f6fc4f9ec92b"

for required_command in gh git sha256sum tar; do
    if ! command -v "${required_command}" >/dev/null 2>&1; then
        echo "Required command not found: ${required_command}" >&2
        exit 1
    fi
done

repository_root="$(git rev-parse --show-toplevel)"
install_parent="${repository_root}/.tools/shadercross"
install_directory="${install_parent}/${shadercross_directory_name}"
installed_executable="${install_directory}/bin/shadercross"

if [[ -f "${installed_executable}" ]]; then
    if ! echo "${expected_executable_sha256}  ${installed_executable}" | sha256sum --check --status; then
        echo "Existing shadercross executable has an unexpected SHA-256." >&2
        echo "Refusing to overwrite ${install_directory}" >&2
        exit 1
    fi

    echo "Pinned shadercross tool is already installed and verified."
    exit 0
fi

if [[ -e "${install_directory}" ]]; then
    echo "A partial shadercross directory already exists: ${install_directory}" >&2
    echo "Move it aside and run this script again." >&2
    exit 1
fi

download_directory="$(mktemp -d /tmp/codename-hover-shadercross.XXXXXX)"
cleanup()
{
    rm -rf -- "${download_directory}"
}
trap cleanup EXIT

gh run download "${shadercross_run_id}" \
    --repo libsdl-org/SDL_shadercross \
    --name "${shadercross_artifact_name}" \
    --dir "${download_directory}"

archive_path="${download_directory}/${shadercross_archive_name}"
echo "${expected_archive_sha256}  ${archive_path}" | sha256sum --check --status

tar -xzf "${archive_path}" -C "${download_directory}"

downloaded_executable="${download_directory}/${shadercross_directory_name}/bin/shadercross"
echo "${expected_executable_sha256}  ${downloaded_executable}" | sha256sum --check --status

mkdir -p "${install_parent}"
mv "${download_directory}/${shadercross_directory_name}" "${install_parent}/"

echo "Installed and verified shadercross at ${install_directory}"
