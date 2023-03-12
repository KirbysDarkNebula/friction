#!/bin/sh
set -e

CWD=`pwd`
INKSCAPE=${INKSCAPE:-inkscape}
FORCE_GEN=${FORCE_GEN:-0}
INDEX=${INDEX:-0}
HICOLOR_DIR="${CWD}/src/app/icons/hicolor"
HICOLOR_SVG="${HICOLOR_DIR}/scalable"
HICOLOR_SIZES="
16
22
32
48
64
96
128
192
256
"
HICOLOR_CATS="
actions
apps
"
INDEX_THEME="${HICOLOR_DIR}/index.theme"
INDEX_THEME_HEADER="
[Icon Theme]
Name=hicolor
Comment=hicolor
Example=folder
Inherits=hicolor

# KDE Specific Stuff
DisplayDepth=32
LinkOverlay=link_overlay
LockOverlay=lock_overlay
ZipOverlay=zip_overlay
DesktopDefault=32
DesktopSizes=16,32
ToolbarDefault=32
ToolbarSizes=16,32
MainToolbarDefault=32
MainToolbarSizes=16,32
SmallDefault=16
SmallSizes=16
PanelDefault=32
PanelSizes=16,32
"
INDEX_THEME_DIRS="Directories="
INDEX_THEME_CATS=""

for W in $HICOLOR_SIZES; do
    for C in $HICOLOR_CATS; do
        if [ "${C}" = "actions" ]; then
            ACT="Actions"
        elif [ "${C}" = "apps" ]; then
            ACT="Applications"
        else
            ACT="Unknown"
        fi
        INDEX_THEME_DIRS="${INDEX_THEME_DIRS}${W}x${W}/${C},"
        INDEX_THEME_CATS="${INDEX_THEME_CATS}
[${W}x${W}/${C}]
Context=${ACT}
Size=${W}
Type=Fixed
"
        if [ -d "${HICOLOR_SVG}/${C}" ]; then
            ICONS=`ls ${HICOLOR_SVG}/${C}/ | sed 's#.svg##g'`
        else
            ICONS=""
        fi
        DIR="${HICOLOR_DIR}/${W}x${W}/${C}"
        echo "==> check for ${DIR} ..."
        if [ ! -d "${DIR}" ]; then
            mkdir -p "${DIR}"
        fi
        for ICON in $ICONS; do
            if [ ! -f "${DIR}/${ICON}.png" ] || [ "${FORCE_GEN}" = 1 ]; then
                echo "==> make ${ICON}.png (${W}x${W}) from ${ICON}.svg ..."
                $INKSCAPE \
                --export-background-opacity=0 \
                --export-width=${W} \
                --export-height=${W} \
                --export-type=png \
                --export-filename="${DIR}/${ICON}.png" \
                "${HICOLOR_SVG}/${C}/${ICON}.svg"
            else
                echo "==> ${DIR}/${ICON}.png already exists"
            fi
        done
    done
done

if [ "${INDEX}" = 1 ]; then
    echo "${INDEX_THEME_HEADER}" > "${INDEX_THEME}"
    echo "${INDEX_THEME_DIRS}" >> "${INDEX_THEME}"
    echo "${INDEX_THEME_CATS}" >> "${INDEX_THEME}"
fi