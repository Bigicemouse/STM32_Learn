#!/usr/bin/env sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
EIDE_YML="$PROJECT_ROOT/.eide/eide.yml"
ENV_INI="$PROJECT_ROOT/.eide/env.ini"
LAUNCH_JSON="$PROJECT_ROOT/.vscode/launch.json"

die() {
    echo "post-install: $*" >&2
    exit 1
}

read_project_name() {
    if [ -f "$EIDE_YML" ]; then
        awk '
            /^[[:space:]]*name:[[:space:]]*/ {
                value = $0
                sub(/^[[:space:]]*name:[[:space:]]*/, "", value)
                gsub(/^[[:space:]"'"'"']+|[[:space:]"'"'"']+$/, "", value)
                print value
                exit
            }
        ' "$EIDE_YML" | tr -d '\r'
    fi
}

PROJECT_NAME=$(read_project_name)

if [ -z "$PROJECT_NAME" ] || [ "$PROJECT_NAME" = "stm32f103_stdperiph_template" ]; then
    PROJECT_NAME=$(basename "$PROJECT_ROOT")
fi

case "$PROJECT_NAME" in
    *[!A-Za-z0-9_]* | "" )
        die "project name '$PROJECT_NAME' is invalid; use only letters, digits, and underscore"
        ;;
esac

update_env_ini() {
    if [ ! -f "$ENV_INI" ]; then
        return
    fi

    if grep -q '^KEIL_OUTPUT_NAME=' "$ENV_INI"; then
        sed -i "s/^KEIL_OUTPUT_NAME=.*/KEIL_OUTPUT_NAME=$PROJECT_NAME/" "$ENV_INI"
    else
        printf '\nKEIL_OUTPUT_NAME=%s\n' "$PROJECT_NAME" >> "$ENV_INI"
    fi
}

update_uvprojx() {
    for file in "$PROJECT_ROOT"/*.uvprojx; do
        [ -e "$file" ] || continue
        sed -i "s#<OutputName>[^<]*</OutputName>#<OutputName>$PROJECT_NAME</OutputName>#" "$file"

        target="$PROJECT_ROOT/$PROJECT_NAME.uvprojx"
        if [ "$file" != "$target" ]; then
            mv -f "$file" "$target"
        fi
    done
}

rename_uvoptx() {
    for file in "$PROJECT_ROOT"/*.uvoptx; do
        [ -e "$file" ] || continue
        target="$PROJECT_ROOT/$PROJECT_NAME.uvoptx"
        if [ "$file" != "$target" ]; then
            mv -f "$file" "$target"
        fi
    done
}

rename_workspace() {
    for file in "$PROJECT_ROOT"/*.code-workspace; do
        [ -e "$file" ] || continue
        target="$PROJECT_ROOT/$PROJECT_NAME.code-workspace"
        if [ "$file" != "$target" ]; then
            mv -f "$file" "$target"
        fi
    done
}

update_launch_json() {
    if [ ! -f "$LAUNCH_JSON" ]; then
        return
    fi

    sed -i "s#\${workspaceFolder}/build/default/[^\"/]*\.axf#\${workspaceFolder}/build/default/$PROJECT_NAME.axf#g" "$LAUNCH_JSON"
}

update_env_ini
update_uvprojx
rename_uvoptx
rename_workspace
update_launch_json

echo "post-install: project name synchronized to '$PROJECT_NAME'"
