#!/usr/bin/env bash

set -euo pipefail

for required_command in git rsync ssh; do
    if ! command -v "${required_command}" >/dev/null 2>&1; then
        echo "Required command not found: ${required_command}" >&2
        exit 1
    fi
done

readonly repository_root="$(git rev-parse --show-toplevel)"
readonly build_directory="${HOVER_BUILD_DIRECTORY:-${repository_root}/build/development}"
readonly deck_host="${HOVER_DECK_HOST:-steamdeck}"
readonly deck_root="${HOVER_DECK_ROOT:-/run/media/deck/SR01T/development/codename_hover}"
readonly game_executable="${build_directory}/codename_hover"
readonly shader_directory="${build_directory}/shaders"

if [[ ! -x "${game_executable}" ]]; then
    echo "Game executable not found or not executable: ${game_executable}" >&2
    echo "Build it first with: cmake --build --preset development" >&2
    exit 1
fi

if [[ ! -d "${shader_directory}" ]]; then
    echo "Compiled shader directory not found: ${shader_directory}" >&2
    echo "Build it first with: cmake --build --preset development" >&2
    exit 1
fi

ssh_options=()
if [[ -n "${HOVER_DECK_SSH_CONFIG:-}" ]]; then
    ssh_options=(-F "${HOVER_DECK_SSH_CONFIG}")
fi
readonly -a ssh_command=(ssh "${ssh_options[@]}")

printf -v rsync_remote_shell '%q ' "${ssh_command[@]}"
readonly rsync_remote_shell

echo "Creating Deck deployment directory: ${deck_root}"
"${ssh_command[@]}" "${deck_host}" mkdir -p -- "${deck_root}"

echo "Deploying executable and shaders to ${deck_host}:${deck_root}"
rsync --archive --human-readable --itemize-changes \
    --rsh="${rsync_remote_shell}" \
    "${game_executable}" "${shader_directory}" "${deck_host}:${deck_root}/"

echo "Verifying the deployed executable with a headless scenario listing"
"${ssh_command[@]}" "${deck_host}" "${deck_root}/codename_hover" --list-scenarios

echo "Deck deployment complete."
echo "Launch on the Deck with:"
echo "  ${deck_root}/codename_hover --scenario oval"
