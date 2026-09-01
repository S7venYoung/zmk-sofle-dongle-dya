#!/usr/bin/env python3
"""Generate compact Kyo-vs-Terry OLED spans from zmk-dongle-fighter-theme."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


CHARACTERS = {
    "left": {
        "name": "Kyo",
        "idle": "win_b",
        "slow": "kick_ch",
        "mid": "oni_yaki_l",
        "fast": "ura_orochi_nagi_d",
    },
    "right": {
        "name": "Terry",
        "idle": "idle",
        "slow": "punch_l",
        "mid": "rising_tackle_l",
        "fast": "power_geyser_d",
    },
}
ACTION_LIMITS = {"idle": 4, "slow": 6, "mid": 8, "fast": 10}


def selected_files(directory: Path, limit: int) -> list[Path]:
    files = sorted(directory.glob("*.bmp"))
    if not files:
        raise SystemExit(f"no bitmap frames in {directory}")
    if len(files) <= limit:
        return files
    return [files[round(index * (len(files) - 1) / (limit - 1))] for index in range(limit)]


def bitmap_spans(path: Path) -> list[tuple[int, int, int]]:
    image = Image.open(path)
    if image.mode != "P":
        image = image.convert("P")
    width = min(image.width, 58)
    height = min(image.height, 50)
    source_x = max(0, (image.width - width) // 2)
    source_y = max(0, image.height - height)
    target_x = 124 - width
    target_y = 62 - height
    pixels = image.load()
    spans: list[tuple[int, int, int]] = []
    for row in range(height):
        y = target_y + row
        xs = []
        for column in range(width):
            shade = pixels[source_x + column, source_y + row]
            x = target_x + column
            if (shade == 3 or (shade == 2 and (x + y) % 2 == 0) or
                    (shade == 1 and x % 2 == 0 and y % 2 == 0)):
                xs.append(x)
        start = previous = None
        for x in xs:
            if start is None:
                start = previous = x
            elif x == previous + 1:
                previous = x
            else:
                spans.append((y, start, previous))
                start = previous = x
        if start is not None:
            spans.append((y, start, previous))
    return spans


def load_assets(root: Path) -> dict[str, dict[str, list[list[tuple[int, int, int]]]]]:
    result = {}
    for side, config in CHARACTERS.items():
        character = config["name"]
        result[side] = {}
        for action, limit in ACTION_LIMITS.items():
            directory = root / character / config[action]
            result[side][action] = [bitmap_spans(path) for path in selected_files(directory, limit)]
    return result


def write_c(output: Path, assets: dict[str, dict[str, list[list[tuple[int, int, int]]]]]) -> None:
    lines = [
        "/* Generated from hitsmaxft/zmk-dongle-fighter-theme Kyo and Terry assets. */",
        "#include <zephyr/sys/util.h>",
        '#include "fight_assets.h"',
        "",
    ]
    for side, actions in assets.items():
        for action, frames in actions.items():
            prefix = f"{side}_{action}"
            for frame_index, spans in enumerate(frames):
                lines.append(f"static const struct fight_span {prefix}_{frame_index}[] = {{")
                lines.extend(f"    {{{y}, {x0}, {x1}}}," for y, x0, x1 in spans)
                lines.append("};")
            lines.append(f"static const struct fight_frame {prefix}_frames[] = {{")
            lines.extend(
                f"    {{{prefix}_{index}, ARRAY_SIZE({prefix}_{index})}},"
                for index in range(len(frames))
            )
            lines.append("};\n")

    for side in ("left", "right"):
        lines.append(f"static const struct fight_frame *const {side}_actions[FIGHT_ACTION_COUNT] = {{")
        for action in ACTION_LIMITS:
            lines.append(f"    [FIGHT_ACTION_{action.upper()}] = {side}_{action}_frames,")
        lines.append("};")
        lines.append(f"static const uint8_t {side}_counts[FIGHT_ACTION_COUNT] = {{")
        for action in ACTION_LIMITS:
            lines.append(
                f"    [FIGHT_ACTION_{action.upper()}] = ARRAY_SIZE({side}_{action}_frames),"
            )
        lines.append("};\n")

    lines.extend(
        [
            "const struct fight_frame *fight_asset_frame(enum fight_side side,",
            "                                            enum fight_action action, uint8_t frame) {",
            "    if (action >= FIGHT_ACTION_COUNT) { action = FIGHT_ACTION_IDLE; }",
            "    const struct fight_frame *const *actions =",
            "        side == FIGHT_SIDE_LEFT ? left_actions : right_actions;",
            "    const uint8_t *counts = side == FIGHT_SIDE_LEFT ? left_counts : right_counts;",
            "    return &actions[action][frame % counts[action]];",
            "}",
            "",
            "uint8_t fight_asset_frame_count(enum fight_side side, enum fight_action action) {",
            "    if (action >= FIGHT_ACTION_COUNT) { return 1; }",
            "    return side == FIGHT_SIDE_LEFT ? left_counts[action] : right_counts[action];",
            "}",
        ]
    )
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bitmap-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assets = load_assets(args.bitmap_root)
    write_c(args.output, assets)
    print({side: {action: len(frames) for action, frames in actions.items()}
           for side, actions in assets.items()})


if __name__ == "__main__":
    main()
