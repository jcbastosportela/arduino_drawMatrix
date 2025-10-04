#!/usr/bin/env python3
'''
HTML Decompression Tool for Arduino/ESP8266 Projects

Filename: /home/portela/projects/Arduino/tools/decompressHtml.py
Path: /home/portela/projects/Arduino/tools
Created Date: Saturday, October 4th 2025, 3:33:03 pm
Author: Joao Carlos Bastos Portela

Copyright (c) 2025 Your Company

This tool extracts and decompresses gzip-compressed HTML data stored as
uint8_t arrays in C++ source files (commonly used in ESP8266/ESP32 projects
for storing web content in PROGMEM).
'''

import argparse
import gzip
import os
import sys
import tempfile
from pathlib import Path


def extract_byte_array_from_cpp(file_path):
    """
    Extract byte array data from C++ file between curly braces.

    Args:
        file_path (str): Path to the C++ source file

    Returns:
        list: List of byte values extracted from the array

    Raises:
        ValueError: If no valid byte array is found
        FileNotFoundError: If the input file doesn't exist
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except FileNotFoundError:
        raise FileNotFoundError(f"Input file not found: {file_path}")
    except Exception as e:
        raise ValueError(f"Error reading file {file_path}: {e}")

    # Find the data between the curly braces
    start = content.find('{')
    end = content.rfind('}')

    if start == -1 or end == -1 or start >= end:
        raise ValueError("No valid byte array found in file (missing curly braces)")

    data_str = content[start + 1:end]

    # Parse the comma-separated values
    try:
        byte_values = []
        for value in data_str.split(','):
            value = value.strip()
            if value:  # Skip empty values
                byte_values.append(int(value))

        if not byte_values:
            raise ValueError("No valid byte values found in array")

        return byte_values

    except ValueError as e:
        raise ValueError(f"Error parsing byte values: {e}")


def decompress_gzip_data(byte_values):
    """
    Decompress gzip-compressed byte data.

    Args:
        byte_values (list): List of byte values to decompress

    Returns:
        str: Decompressed content as string

    Raises:
        ValueError: If decompression fails
    """
    try:
        # Convert to bytes
        compressed_data = bytes(byte_values)

        # Check for gzip magic number
        if len(compressed_data) < 2 or compressed_data[:2] != b'\x1f\x8b':
            raise ValueError("Data doesn't appear to be gzip compressed (missing magic number)")

        # Decompress
        decompressed_data = gzip.decompress(compressed_data)

        # Try to decode as UTF-8
        return decompressed_data.decode('utf-8')

    except gzip.BadGzipFile as e:
        raise ValueError(f"Invalid gzip data: {e}")
    except UnicodeDecodeError as e:
        raise ValueError(f"Decompressed data is not valid UTF-8: {e}")
    except Exception as e:
        raise ValueError(f"Decompression failed: {e}")


def save_decompressed_content(content, output_path, file_type='html'):
    """
    Save decompressed content to file.

    Args:
        content (str): Content to save
        output_path (str): Output file path
        file_type (str): File type for extension if not specified
    """
    # Add extension if not present
    if not os.path.splitext(output_path)[1]:
        output_path += f'.{file_type}'

    # Create output directory if it doesn't exist
    os.makedirs(os.path.dirname(output_path) or '.', exist_ok=True)

    try:
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Decompressed content saved to: {output_path}")

    except Exception as e:
        raise ValueError(f"Error saving file {output_path}: {e}")


def detect_content_type(content):
    """
    Detect the type of decompressed content.

    Args:
        content (str): Content to analyze

    Returns:
        str: Detected content type
    """
    content_lower = content.lower().strip()

    if content_lower.startswith('<!doctype html') or content_lower.startswith('<html'):
        return 'html'
    elif content_lower.startswith('<?xml'):
        return 'xml'
    elif content_lower.startswith('{') or content_lower.startswith('['):
        return 'json'
    elif 'body{' in content_lower or 'html{' in content_lower:
        return 'css'
    elif 'function' in content_lower or 'var ' in content_lower:
        return 'js'
    else:
        return 'txt'


def main():
    """Main function to handle command line arguments and orchestrate the decompression."""
    parser = argparse.ArgumentParser(
        description='Decompress gzip-compressed HTML/data from C++ byte arrays',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s elop.cpp                           # Decompress to elop_decompressed.html
  %(prog)s elop.cpp -o web_interface.html     # Specify output filename
  %(prog)s elop.cpp -o /tmp/                  # Output to directory
  %(prog)s elop.cpp --stats                   # Show compression statistics
        '''
    )

    parser.add_argument('input_file',
                       help='C++ source file containing compressed byte array')
    parser.add_argument('-o', '--output',
                       help='Output file path (default: input_name_decompressed.ext)')
    parser.add_argument('--stats', action='store_true',
                       help='Show compression statistics')
    parser.add_argument('--temp', action='store_true',
                       help='Keep temporary compressed file for inspection')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Enable verbose output')

    args = parser.parse_args()

    try:
        # Validate input file
        if not os.path.isfile(args.input_file):
            print(f"Error: Input file '{args.input_file}' not found", file=sys.stderr)
            sys.exit(1)

        if args.verbose:
            print(f"Processing file: {args.input_file}")

        # Extract byte array from C++ file
        byte_values = extract_byte_array_from_cpp(args.input_file)

        if args.verbose:
            print(f"Extracted {len(byte_values)} bytes from array")

        # Decompress the data
        decompressed_content = decompress_gzip_data(byte_values)

        # Detect content type
        content_type = detect_content_type(decompressed_content)

        if args.verbose:
            print(f"Detected content type: {content_type}")

        # Determine output path
        if args.output:
            if os.path.isdir(args.output):
                # Output is a directory
                input_name = os.path.splitext(os.path.basename(args.input_file))[0]
                output_path = os.path.join(args.output, f"{input_name}_decompressed.{content_type}")
            else:
                # Output is a specific file
                output_path = args.output
        else:
            # Default output path
            input_path = Path(args.input_file)
            output_path = input_path.parent / f"{input_path.stem}_decompressed.{content_type}"

        # Save decompressed content
        save_decompressed_content(decompressed_content, str(output_path), content_type)

        # Show statistics if requested
        if args.stats:
            compressed_size = len(byte_values)
            decompressed_size = len(decompressed_content.encode('utf-8'))
            compression_ratio = decompressed_size / compressed_size if compressed_size > 0 else 0

            print(f"\nCompression Statistics:")
            print(f"  Compressed size:   {compressed_size:,} bytes")
            print(f"  Decompressed size: {decompressed_size:,} bytes")
            print(f"  Compression ratio: {compression_ratio:.1f}x")
            print(f"  Space saved:       {((decompressed_size - compressed_size) / decompressed_size * 100):.1f}%")

        # Save temporary compressed file if requested
        if args.temp:
            temp_path = f"{os.path.splitext(output_path)[0]}.gz"
            with open(temp_path, 'wb') as f:
                f.write(bytes(byte_values))
            print(f"Temporary compressed file saved to: {temp_path}")

        print("Decompression completed successfully!")

    except KeyboardInterrupt:
        print("\nOperation cancelled by user", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()