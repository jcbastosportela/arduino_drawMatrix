#!/usr/bin/env python3
"""
Serial.print to Logger Migration Script
Automatically converts Serial.print/println calls to LOG_* macros
and removes the old Serial calls.
"""

import re
import sys
import argparse
from pathlib import Path
from typing import List, Tuple, Optional

# Mapping of keywords to log levels
LOG_LEVEL_KEYWORDS = {
    'ERROR': ['error', 'failed', 'fail', 'cannot', 'unable', 'invalid'],
    'WARNING': ['warning', 'warn', 'retry', 'retrying', 'timeout', 'reconnect'],
    'INFO': ['connected', 'started', 'initialized', 'online', 'success', 'complete', 'ready', 'available'],
    'DEBUG': ['click', 'press', 'button', 'activity', 'detected', 'playing', 'stopping', 'folder', 'track'],
    'TRACE': ['dump', 'hook'],
}

# Module name mapping based on context/file
MODULE_MAPPING = {
    'DrawMatrix.ino': {
        'wifi': 'WIFI',
        'ntp': 'NTP',
        'mdns': 'MDNS',
        'server': 'SERVER',
        'ota': 'OTA',
        'alarm': 'ALARM',
        'button': 'BUTTON',
        'display': 'DISPLAY',
        'music': 'MUSIC',
        'default': 'MAIN'
    },
    'ServerSys.cpp': {
        'alarm': 'ALARM',
        'matrix': 'MATRIX',
        'littlefs': 'FS',
        'default': 'SERVER'
    },
    'MusicPlayer.cpp': {
        'dfplayer': 'MUSIC',
        'sd': 'MUSIC',
        'littlefs': 'FS',
        'default': 'MUSIC'
    }
}


def infer_log_level(message: str) -> str:
    """Infer appropriate log level based on message content."""
    msg_lower = message.lower()

    for level, keywords in LOG_LEVEL_KEYWORDS.items():
        for keyword in keywords:
            if keyword in msg_lower:
                return level

    # Default to INFO if no keywords match
    return 'INFO'


def infer_module(message: str, filename: str, context: str = '') -> str:
    """Infer module name based on message content and filename."""
    msg_lower = message.lower() + ' ' + context.lower()

    if filename in MODULE_MAPPING:
        for keyword, module in MODULE_MAPPING[filename].items():
            if keyword != 'default' and keyword in msg_lower:
                return module
        return MODULE_MAPPING[filename]['default']

    return 'MAIN'


def parse_serial_printf(line: str) -> Optional[Tuple[str, str, List[str]]]:
    """Parse Serial.printf() call and extract format string and args."""
    # Match Serial.printf("format", args...)
    match = re.search(r'Serial\.printf\s*\(\s*"([^"]*)"(?:\s*,\s*(.+?))?\s*\)', line)
    if match:
        format_str = match.group(1)
        args_str = match.group(2) if match.group(2) else ''
        args = [arg.strip() for arg in args_str.split(',')] if args_str else []
        return (format_str, args_str, args)
    return None


def parse_serial_print(line: str) -> Optional[str]:
    """Parse Serial.print/println and extract the message."""
    # Handle Serial.println("message")
    match = re.search(r'Serial\.println\s*\(\s*"([^"]*)"\s*\)', line)
    if match:
        return match.group(1)

    # Handle Serial.print("message")
    match = re.search(r'Serial\.print\s*\(\s*"([^"]*)"\s*\)', line)
    if match:
        return match.group(1)

    # Handle Serial.println(variable) - concatenation
    match = re.search(r'Serial\.println\s*\((.+?)\)', line)
    if match:
        content = match.group(1).strip()
        # If it's a simple string concatenation, try to extract
        if '+' in content:
            # Simple heuristic: extract first string literal
            str_match = re.search(r'"([^"]*)"', content)
            if str_match:
                return str_match.group(1)
        return None  # Too complex, skip

    return None


def convert_to_log_macro(serial_call: str, filename: str, line_num: int, context: str = '') -> Optional[str]:
    """Convert a Serial.print* call to appropriate LOG_* macro."""

    # Handle Serial.printf
    printf_result = parse_serial_printf(serial_call)
    if printf_result:
        format_str, args_str, args = printf_result
        level = infer_log_level(format_str)
        module = infer_module(format_str, filename, context)

        # Remove trailing \n from format string if present
        format_str = format_str.rstrip('\\n')

        if args:
            return f'LOG_{level}("{module}", "{format_str}", {args_str})'
        else:
            return f'LOG_{level}("{module}", "{format_str}")'

    # Handle Serial.print/println with string literal
    message = parse_serial_print(serial_call)
    if message:
        level = infer_log_level(message)
        module = infer_module(message, filename, context)
        return f'LOG_{level}("{module}", "{message}")'

    # Too complex to auto-convert
    return None


def process_file(filepath: Path, dry_run: bool = False) -> Tuple[int, int, List[str]]:
    """Process a single file and convert Serial.print calls to LOG_* macros."""

    print(f"\n{'[DRY RUN] ' if dry_run else ''}Processing: {filepath}")

    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    modified_lines = []
    conversions = 0
    skipped = 0
    skipped_lines = []

    i = 0
    while i < len(lines):
        line = lines[i]
        original_line = line

        # Check if line contains Serial.print/printf
        if 'Serial.print' in line:
            # Get context from previous lines for better module inference
            context = ' '.join(lines[max(0, i-3):i])

            # Try to convert
            converted = convert_to_log_macro(line.strip(), filepath.name, i + 1, context)

            if converted:
                # Preserve indentation
                indent = len(line) - len(line.lstrip())
                modified_lines.append(' ' * indent + converted + '\n')
                conversions += 1
                print(f"  Line {i+1}: {line.strip()}")
                print(f"       ->  {converted}")
            else:
                # Keep original line but mark as skipped
                modified_lines.append(line)
                skipped += 1
                skipped_lines.append(f"Line {i+1}: {line.strip()}")
                print(f"  [SKIP] Line {i+1}: {line.strip()}")
        else:
            modified_lines.append(line)

        i += 1

    # Write back if not dry run
    if not dry_run and conversions > 0:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(modified_lines)
        print(f"  ✓ Converted {conversions} lines, skipped {skipped} lines")
    elif dry_run:
        print(f"  [DRY RUN] Would convert {conversions} lines, skip {skipped} lines")

    return conversions, skipped, skipped_lines


def main():
    parser = argparse.ArgumentParser(
        description='Migrate Serial.print/println calls to Logger macros'
    )
    parser.add_argument(
        'files',
        nargs='+',
        type=Path,
        help='Files to process (.ino, .cpp, .hpp)'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Preview changes without modifying files'
    )

    args = parser.parse_args()

    total_conversions = 0
    total_skipped = 0
    all_skipped_lines = []

    for filepath in args.files:
        if not filepath.exists():
            print(f"Error: File not found: {filepath}")
            continue

        conversions, skipped, skipped_lines = process_file(filepath, args.dry_run)
        total_conversions += conversions
        total_skipped += skipped

        if skipped_lines:
            all_skipped_lines.append(f"\n{filepath}:")
            all_skipped_lines.extend(skipped_lines)

    print("\n" + "="*70)
    print(f"Summary:")
    print(f"  Total conversions: {total_conversions}")
    print(f"  Total skipped: {total_skipped}")

    if all_skipped_lines:
        print(f"\nSkipped lines (manual review needed):")
        for line in all_skipped_lines:
            print(f"  {line}")

    if args.dry_run:
        print("\n[DRY RUN] No files were modified. Remove --dry-run to apply changes.")
    else:
        print("\n✓ Migration complete!")


if __name__ == '__main__':
    main()
